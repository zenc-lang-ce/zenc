// SPDX-License-Identifier: MIT
#include "plugins/plugin_manager.h"
#include "parser.h"
#include "utils/format_expr.h"
#include "utils/colors.h"
#include "utils/utils.h"
#include "constants.h"
#include "ast/primitives.h"
#include <ctype.h>
#include "analysis/const_fold.h"

// Returns whether it gave a link_name to the type or to a type inside it.
static int sync_type_linkage_depth(ParserContext *ctx, Type *t, int depth)
{
    if (!t)
    {
        return 0;
    }
    // Guard against cyclic / pathologically deep type graphs (fuzz inputs can
    // construct self-referential or deeply nested generic types).
    if (depth > 512)
    {
        return 0;
    }
    int changed = 0;
    if ((t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && !t->link_name && t->name)
    {
        ASTNode *def = find_struct_def(ctx, t->name);
        if (def && def->link_name)
        {
            t->link_name = xstrdup(def->link_name);
            changed = 1;
        }
    }
    if (t->inner)
    {
        changed |= sync_type_linkage_depth(ctx, t->inner, depth + 1);
    }
    for (int i = 0; t->args && i < t->count; i++)
    {
        changed |= sync_type_linkage_depth(ctx, t->args[i], depth + 1);
    }
    return changed;
}

static int sync_type_linkage(ParserContext *ctx, Type *t)
{
    return sync_type_linkage_depth(ctx, t, 0);
}

// Whether t, or the type it points to, is named in C by its @link_name.
static int type_has_link_name(Type *t)
{
    while (t)
    {
        if (t->link_name)
        {
            return 1;
        }
        t = t->inner;
    }
    return 0;
}

static void sync_link_names_recursive_depth(ParserContext *ctx, ASTNode *node, int depth);

// The link names of one node and of everything under it, not of the nodes that follow it.
static void sync_link_names_node(ParserContext *ctx, ASTNode *node, int depth)
{
    if (node->type_info && sync_type_linkage(ctx, node->type_info) && node->kind == NODE_FIELD)
    {
        // Codegen declares a field from the text of its type, written when the field was
        // parsed, before the type had its link_name.
        node->field.type = type_to_c_string(node->type_info, ctx->config->misra_mode);
    }

    // The same for a cast. Its Type can be the one of the variable it initializes
    // (`let p = (T*)q;`), which got the link_name first, so this checks the Type instead.
    if (node->kind == NODE_EXPR_CAST && type_has_link_name(node->type_info))
    {
        node->cast.target_type = type_to_c_string(node->type_info, ctx->config->misra_mode);
    }

    // sizeof(T) is written from its own Type, not from the node's.
    if (node->kind == NODE_EXPR_SIZEOF && node->size_of.target_type_info)
    {
        sync_type_linkage(ctx, node->size_of.target_type_info);
    }

    // The data of a variant, also the parameter of its constructor (the same Type).
    if (node->kind == NODE_ENUM_VARIANT && node->variant.payload)
    {
        sync_type_linkage(ctx, node->variant.payload);
    }

    if (node->kind == NODE_FUNCTION)
    {
        if (node->func.ret_type_info)
        {
            sync_type_linkage(ctx, node->func.ret_type_info);
        }
        if (node->func.arg_types)
        {
            for (int i = 0; i < node->func.count; i++)
            {
                sync_type_linkage(ctx, node->func.arg_types[i]);
            }
        }
    }

    // An imported module's tree is not a child: it hangs from the first import of its file
    // only, so it is walked once.
    if (node->kind == NODE_IMPORT)
    {
        sync_link_names_recursive_depth(ctx, node->import_stmt.module_root, depth + 1);
    }

    ASTChildren children = ast_children(node);
    for (int i = 0; i < children.count; i++)
    {
        sync_link_names_recursive_depth(ctx, *children.slots[i], depth + 1);
    }
}

static void sync_link_names_recursive_depth(ParserContext *ctx, ASTNode *node, int depth)
{
    if (depth > 4096)
    {
        return;
    }

    for (; node; node = node->next)
    {
        sync_link_names_node(ctx, node, depth);
    }
}

static void sync_link_names_recursive(ParserContext *ctx, ASTNode *node)
{
    sync_link_names_recursive_depth(ctx, node, 0);
}

void audit_section_5(ParserContext *ctx, Scope *scope, const char *name, const char *link_name,
                     Token tok)
{
    if (!scope || !name)
    {
        return;
    }
    if (strcmp(name, "it") == 0 || strcmp(name, "self") == 0)
    {
        return;
    }

    if (!ctx->config->misra_mode)
    {
        // Only shadowing is reported: each symbol of this name in the scopes around this one.
        if (!ctx->silent_warnings)
        {
            for (Scope *p = scope->parent; p; p = p->parent)
            {
                for (ZenSymbol *sh = symbol_lookup_local(p, name); sh; sh = sh->next_same_name)
                {
                    warn_shadowing(tok, name);
                }
            }
        }
        return;
    }

    if (ctx->hook_check_standard_macro_name)
    {
        ctx->hook_check_standard_macro_name(tok, name);
    }

    // MISRA also compares the name with every symbol in scope, so it walks them all.
    Scope *p = scope;
    int limit = (p == ctx->global_scope) ? 31 : 63;
    const char *actual_name = link_name ? link_name : name;

    while (p)
    {
        ZenSymbol *sh = p->symbols;
        while (sh)
        {
            if (p != scope && strcmp(sh->name, name) == 0 && !ctx->silent_warnings)
            {
                zerror_at(tok, "MISRA Rule 5.3");
            }

            const char *sh_actual_name = sh->link_name ? sh->link_name : sh->name;
            if (strcmp(sh_actual_name, actual_name) != 0 && ctx->hook_check_identifier_collision)
            {
                ctx->hook_check_identifier_collision(tok, sh_actual_name, actual_name, limit);
            }

            sh = sh->next;
        }
        p = p->parent;
    }
}

void sync_all_link_names(ParserContext *ctx, ASTNode *root)
{
    sync_link_names_recursive(ctx, root);

    // Generic instances (Vec<Point>…) live outside the tree, in their own lists. New ones are
    // added at the head, so only those added since the last pass are walked: the language
    // server parses every file of a workspace with one context, and walking the whole lists for
    // each file made indexing quadratic. A compilation runs this pass once and walks them all.
    for (ASTNode *s = ctx->instantiated_structs; s && s != ctx->link_synced_structs; s = s->next)
    {
        sync_link_names_node(ctx, s, 0);
    }
    for (ASTNode *f = ctx->instantiated_funcs; f && f != ctx->link_synced_funcs; f = f->next)
    {
        sync_link_names_node(ctx, f, 0);
    }
    ctx->link_synced_structs = ctx->instantiated_structs;
    ctx->link_synced_funcs = ctx->instantiated_funcs;
}
