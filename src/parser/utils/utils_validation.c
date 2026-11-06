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

void register_type_usage(ParserContext *ctx, const char *name, Token t)
{
    if (ctx->is_speculative)
    {
        return;
    }

    TypeUsage *u = xmalloc(sizeof(TypeUsage));
    u->name = xstrdup(name);
    u->location = t;
    u->next = ctx->pending_type_validations;
    ctx->pending_type_validations = u;
}

int validate_types(ParserContext *ctx)
{
    int errors = 0;
    TypeUsage *u = ctx->pending_type_validations;
    while (u)
    {
        ASTNode *def = find_struct_def(ctx, u->name);
        if (!def)
        {
            Type *alias = find_type_alias(ctx, u->name);
            if (!alias)
            {
                SelectiveImport *si = find_selective_import(ctx, u->name);
                if (!si && !is_extern_symbol(ctx, u->name))
                {
                    if (!is_trait(u->name) && TYPE_UNKNOWN == find_primitive_kind(u->name))
                    {
                        int whitelisted = 0;
                        if (ctx->config->c_type_whitelist)
                        {
                            char **ptr = ctx->config->c_type_whitelist;
                            while (*ptr)
                            {
                                if (strcmp(u->name, *ptr) == 0)
                                {
                                    whitelisted = 1;
                                    break;
                                }
                                ptr++;
                            }
                        }

                        if (whitelisted)
                        {
                            u = u->next;
                            continue;
                        }

                        if (!ctx->config->mode_lsp && !ctx->config->mode_doc)
                        {
                            char msg[MAX_SHORT_MSG_LEN];
                            snprintf(msg, sizeof(msg),
                                     "Unknown type '%s' (assuming external C struct)", u->name);
                            const char *hint = get_closest_type_hint(ctx, u->name);
                            if (hint)
                            {
                                char help[MAX_MANGLED_NAME_LEN];
                                snprintf(help, sizeof(help), "Did you mean '%s'?", hint);
                                zwarn_with_suggestion(u->location, msg, help);
                            }
                            else
                            {
                                zwarn_at(u->location, "%s", msg);
                            }
                        }
                    }
                }
            }
        }
        u = u->next;
    }
    return errors == 0;
}

void propagate_vector_inner_types(ParserContext *ctx)
{
    StructRef *ref = ctx->parsed_structs_list;
    while (ref)
    {
        ASTNode *strct = ref->node;
        if (strct && strct->kind == NODE_STRUCT && strct->type_info &&
            strct->type_info->kind == TYPE_VECTOR && !strct->type_info->inner)
        {
            if (strct->strct.fields && strct->strct.fields->type_info)
            {
                strct->type_info->inner = strct->strct.fields->type_info;
            }
        }
        ref = ref->next;
    }
}

int type_needs_drop(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return 0;
    }
    if (is_owning_closure(t))
    {
        return 1;
    }
    if (t->kind == TYPE_ARRAY && t->array_size > 0)
    {
        return type_needs_drop(ctx, t->inner);
    }
    if (t->kind == TYPE_TUPLE)
    {
        for (int i = 0; i < t->count; i++)
        {
            if (type_needs_drop(ctx, t->args[i]))
            {
                return 1;
            }
        }
        return 0;
    }
    if ((t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->name)
    {
        ASTNode *def = find_struct_def(ctx, type_def_name(t));
        return def && def->type_info && def->type_info->traits.has_drop;
    }
    return 0;
}

// Marks a struct as Drop if one of its fields needs dropping, and an enum if the payload of one of
// its variants does; returns whether it did.
static int propagate_drop_from_fields(ParserContext *ctx, ASTNode *strct)
{
    if (!strct || !strct->type_info || strct->type_info->traits.has_drop)
    {
        return 0;
    }
    if (strct->kind == NODE_ENUM)
    {
        for (ASTNode *v = strct->enm.variants; v; v = v->next)
        {
            if (v->kind == NODE_ENUM_VARIANT && type_needs_drop(ctx, v->variant.payload))
            {
                strct->type_info->traits.has_drop = 1;
                return 1;
            }
        }
        return 0;
    }
    if (strct->kind != NODE_STRUCT)
    {
        return 0;
    }
    for (ASTNode *field = strct->strct.fields; field; field = field->next)
    {
        Type *ft = field->type_info;
        if (ft && (ft->kind == TYPE_VECTOR || type_needs_drop(ctx, ft)))
        {
            strct->type_info->traits.has_drop = 1;
            ft->traits.has_drop = 1;
            return 1;
        }
    }
    return 0;
}

// Whether the struct or enum `node` (not a template) has an `impl Drop`, its own or, for an
// instance (`Wrap__int32_t`), its template's (`Wrap`).
static int implements_drop(ParserContext *ctx, ASTNode *node)
{
    if (!node || (node->kind == NODE_STRUCT ? node->strct.is_template
                                            : node->kind != NODE_ENUM || node->enm.is_template))
    {
        return 0;
    }
    const char *name = node->kind == NODE_STRUCT ? node->strct.name : node->enm.name;
    const char *tpl = NULL;
    for (Instantiation *inst = ctx->instantiations; inst && !tpl; inst = inst->next)
    {
        if (inst->template_name && strcmp(inst->name, name) == 0)
        {
            tpl = inst->template_name;
        }
    }
    return check_impl(ctx, "Drop", name) || (tpl && check_impl(ctx, "Drop", tpl));
}

// Marks `strct` as Drop if it implements Drop. `impl Drop for X` marks X when it is parsed, which
// misses a struct declared below the impl, and an instance created before a generic impl.
static void seed_drop_from_impl(ParserContext *ctx, ASTNode *strct)
{
    if (strct && strct->type_info && !strct->type_info->traits.has_drop &&
        implements_drop(ctx, strct))
    {
        strct->type_info->traits.has_drop = 1;
    }
}

// type_will_drop, `depth` levels into the declarations (a struct may hold itself through a pointer,
// which is not followed, but the guard keeps any other cycle finite).
static int type_will_drop_at(ParserContext *ctx, Type *t, int depth)
{
    if (!t || depth > 32)
    {
        return 0;
    }
    if (type_needs_drop(ctx, t))
    {
        return 1;
    }
    switch (t->kind)
    {
    case TYPE_TUPLE:
        for (int i = 0; i < t->count; i++)
        {
            if (type_will_drop_at(ctx, t->args[i], depth + 1))
            {
                return 1;
            }
        }
        return 0;
    case TYPE_ARRAY:
        return t->array_size > 0 && type_will_drop_at(ctx, t->inner, depth + 1);
    case TYPE_STRUCT:
    case TYPE_ENUM:
    {
        ASTNode *def = t->name ? find_struct_def(ctx, type_def_name(t)) : NULL;
        if (!def)
        {
            return 0;
        }
        if (implements_drop(ctx, def))
        {
            return 1;
        }
        if (def->kind == NODE_ENUM)
        {
            for (ASTNode *v = def->enm.variants; v; v = v->next)
            {
                if (v->kind == NODE_ENUM_VARIANT &&
                    type_will_drop_at(ctx, v->variant.payload, depth + 1))
                {
                    return 1;
                }
            }
            return 0;
        }
        for (ASTNode *field = def->strct.fields; field; field = field->next)
        {
            if (field->type_info && (field->type_info->kind == TYPE_VECTOR ||
                                     type_will_drop_at(ctx, field->type_info, depth + 1)))
            {
                return 1;
            }
        }
        return 0;
    }
    default:
        return 0;
    }
}

int type_will_drop(ParserContext *ctx, Type *t)
{
    return type_will_drop_at(ctx, t, 0);
}

void propagate_drop_traits(ParserContext *ctx)
{
    for (StructRef *ref = ctx->parsed_structs_list; ref; ref = ref->next)
    {
        seed_drop_from_impl(ctx, ref->node);
    }
    for (StructRef *ref = ctx->parsed_enums_list; ref; ref = ref->next)
    {
        seed_drop_from_impl(ctx, ref->node);
    }
    for (ASTNode *ins = ctx->instantiated_structs; ins; ins = ins->next)
    {
        seed_drop_from_impl(ctx, ins);
    }

    int changed = 1;
    while (changed)
    {
        changed = 0;
        for (StructRef *ref = ctx->parsed_structs_list; ref; ref = ref->next)
        {
            changed |= propagate_drop_from_fields(ctx, ref->node);
        }
        for (StructRef *ref = ctx->parsed_enums_list; ref; ref = ref->next)
        {
            changed |= propagate_drop_from_fields(ctx, ref->node);
        }
        for (ASTNode *ins = ctx->instantiated_structs; ins; ins = ins->next)
        {
            changed |= propagate_drop_from_fields(ctx, ins);
        }
    }
}

void fix_type_refs_has_drop(ParserContext *ctx)
{
    if (!ctx)
    {
        return;
    }

    for (StructRef *ref = ctx->parsed_funcs_list; ref; ref = ref->next)
    {
        ASTNode *fn = ref->node;
        if (!fn || fn->kind != NODE_FUNCTION)
        {
            continue;
        }

        for (int i = 0; i < fn->func.count; i++)
        {
            Type *at = fn->func.arg_types[i];
            if (at && at->kind == TYPE_STRUCT && at->name && !at->traits.has_drop)
            {
                ASTNode *def = find_struct_def(ctx, type_def_name(at));
                if (def && def->type_info && def->type_info->traits.has_drop)
                {
                    at->traits.has_drop = 1;
                }
            }
        }

        if (fn->func.ret_type_info && fn->func.ret_type_info->kind == TYPE_STRUCT &&
            fn->func.ret_type_info->name && !fn->func.ret_type_info->traits.has_drop)
        {
            ASTNode *def = find_struct_def(ctx, type_def_name(fn->func.ret_type_info));
            if (def && def->type_info && def->type_info->traits.has_drop)
            {
                fn->func.ret_type_info->traits.has_drop = 1;
            }
        }
    }

    for (StructRef *ref = ctx->parsed_structs_list; ref; ref = ref->next)
    {
        ASTNode *strct = ref->node;
        if (!strct || strct->kind != NODE_STRUCT)
        {
            continue;
        }
        for (ASTNode *field = strct->strct.fields; field; field = field->next)
        {
            Type *ft = field->type_info;
            if (ft && ft->kind == TYPE_STRUCT && ft->name && !ft->traits.has_drop)
            {
                ASTNode *def = find_struct_def(ctx, type_def_name(ft));
                if (def && def->type_info && def->type_info->traits.has_drop)
                {
                    ft->traits.has_drop = 1;
                }
            }
        }
    }

    for (ASTNode *ins = ctx->instantiated_structs; ins; ins = ins->next)
    {
        if (ins->kind != NODE_STRUCT)
        {
            continue;
        }
        for (ASTNode *field = ins->strct.fields; field; field = field->next)
        {
            Type *ft = field->type_info;
            if (ft && ft->kind == TYPE_STRUCT && ft->name && !ft->traits.has_drop)
            {
                ASTNode *def = find_struct_def(ctx, type_def_name(ft));
                if (def && def->type_info && def->type_info->traits.has_drop)
                {
                    ft->traits.has_drop = 1;
                }
            }
        }
    }
}

typedef struct
{
    const char **names;
    int count;
    int cap;
} DefinedFunctions;

// Adds a defined function name; returns 0 if it was already defined.
static int defined_functions_add(DefinedFunctions *set, const char *name)
{
    for (const char **n = set->names; n < set->names + set->count; n++)
    {
        if (strcmp(*n, name) == 0)
        {
            return 0;
        }
    }
    if (set->count == set->cap)
    {
        set->cap = set->cap ? set->cap * 2 : 64;
        set->names = xrealloc(set->names, sizeof(const char *) * (size_t)set->cap);
    }
    *(set->names + set->count) = name;
    set->count++;
    return 1;
}

// A function with a body, emitted unconditionally under its C name.
static void check_function_definition(ASTNode *f, const char *owner, DefinedFunctions *set,
                                      int *errors)
{
    if (f->kind != NODE_FUNCTION || !f->func.body || !f->func.name || f->func.generic_params ||
        f->cfg_condition)
    {
        return;
    }
    if (defined_functions_add(set, f->func.name))
    {
        return;
    }

    if (owner && f->func.method_name)
    {
        zerror_at(f->token, "Redefinition of method '%s::%s'", owner, f->func.method_name);
    }
    else
    {
        zerror_at(f->token, "Redefinition of function '%s'", f->func.name);
    }
    (*errors)++;
}

static void check_redefinitions_in(ASTNode *node, DefinedFunctions *set, int *errors)
{
    // Definitions between `#if` and `#endif` may be alternatives: not checked.
    int preproc_depth = 0;
    for (; node; node = node->next)
    {
        if (node->kind == NODE_PREPROC_DIRECTIVE && node->raw_stmt.content)
        {
            const char *d = node->raw_stmt.content;
            while (*d == '#' || *d == ' ' || *d == '\t')
            {
                d++;
            }
            if (strncmp(d, "if", 2) == 0)
            {
                preproc_depth++;
            }
            else if (strncmp(d, "endif", 5) == 0 && preproc_depth > 0)
            {
                preproc_depth--;
            }
            continue;
        }
        if (node->kind == NODE_IMPORT)
        {
            check_redefinitions_in(node->import_stmt.module_root, set, errors);
            continue;
        }
        if (preproc_depth > 0 || node->cfg_condition)
        {
            continue;
        }
        if (node->kind == NODE_FUNCTION)
        {
            check_function_definition(node, NULL, set, errors);
        }
        else if (node->kind == NODE_IMPL)
        {
            for (ASTNode *m = node->impl.methods; m; m = m->next)
            {
                check_function_definition(m, type_impl_name(node->impl.target), set, errors);
            }
        }
        else if (node->kind == NODE_IMPL_TRAIT)
        {
            for (ASTNode *m = node->impl_trait.methods; m; m = m->next)
            {
                check_function_definition(m, NULL, set, errors);
            }
        }
    }
}

int validate_function_definitions(ParserContext *ctx, ASTNode *root)
{
    // MISRA mode reports duplicate identifiers itself (rules 5.8 and 5.9).
    if (ctx->config->mode_lsp || ctx->config->misra_mode || !root || root->kind != NODE_ROOT)
    {
        return 1;
    }
    DefinedFunctions set = {0};
    int errors = 0;
    check_redefinitions_in(root->root.children, &set, &errors);
    return errors == 0;
}
