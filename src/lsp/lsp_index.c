// SPDX-License-Identifier: MIT
#include "../constants.h"

#include "lsp_index.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LSPIndex *lsp_index_new(void)
{
    return calloc(1, sizeof(LSPIndex));
}

void lsp_index_free(LSPIndex *idx)
{
    if (!idx)
    {
        return;
    }
    LSPRange *c = idx->head;
    while (c)
    {
        LSPRange *n = c->next;
        if (c->hover_text)
        {
            zfree(c->hover_text);
        }
        zfree(c);
        c = n;
    }
    zfree(idx);
}

static void lsp_index_add(LSPIndex *idx, LSPRange *r)
{
    if (!idx->head)
    {
        idx->head = r;
        idx->tail = r;
    }
    else
    {
        idx->tail->next = r;
        idx->tail = r;
    }
}

// Whether the token's text is in the file at the token's line and column. The code that the
// parser generates from a text of its own is not: its positions count from the start of that text.
static int lsp_index_is_written(const LSPIndex *idx, Token t)
{
    if (t.line <= 0 || t.col <= 0 || !t.start)
    {
        return 0;
    }
    if (!idx->source)
    {
        return 1;
    }
    if (t.line > idx->line_count)
    {
        return 0;
    }
    size_t at = idx->line_starts[t.line - 1] + (size_t)(t.col - 1);
    return at + t.len <= idx->source_len && strncmp(idx->source + at, t.start, t.len) == 0;
}

void lsp_index_add_def(LSPIndex *idx, Token t, const char *hover, ASTNode *node)
{
    if (!lsp_index_is_written(idx, t))
    {
        return;
    }
    LSPRange *r = calloc(1, sizeof(LSPRange));
    r->kind = RANGE_DEFINITION;
    r->start_line = t.line - 1;
    r->start_col = t.col - 1;
    r->end_line = t.line - 1;
    r->end_col = (int)(t.col) - 1 + (int)(t.len);
    if (hover)
    {
        r->hover_text = xstrdup(hover);
    }
    r->node = node;

    lsp_index_add(idx, r);
}
static void lsp_index_add_plugin(LSPIndex *idx, ASTNode *node)
{
    if (!node || node->kind != NODE_PLUGIN)
    {
        return;
    }
    LSPRange *r = calloc(1, sizeof(LSPRange));
    r->kind = RANGE_DEFINITION; // DEFINITION range works for hover
    r->start_line = node->plugin_stmt.start_line - 1;
    r->start_col = node->plugin_stmt.start_col - 1;
    r->end_line = node->plugin_stmt.end_line - 1;
    r->end_col = node->plugin_stmt.end_col - 1;
    r->node = node;

    // Default hover text for the plugin itself (fallback)
    r->hover_text = xstrdup(node->plugin_stmt.plugin_name);

    lsp_index_add(idx, r);
}

void lsp_index_add_ref(LSPIndex *idx, Token t, Token def_t, ASTNode *node)
{
    if (!lsp_index_is_written(idx, t) || def_t.line <= 0)
    {
        return;
    }
    LSPRange *r = calloc(1, sizeof(LSPRange));
    r->kind = RANGE_REFERENCE;
    r->start_line = t.line - 1;
    r->start_col = t.col - 1;
    r->end_line = t.line - 1;
    r->end_col = (int)(t.col) - 1 + (int)(t.len);

    r->def_line = def_t.line - 1;
    r->def_col = def_t.col - 1;
    r->node = node;

    lsp_index_add(idx, r);
}

void lsp_index_add_type_ref(LSPIndex *idx, Token t, const char *name)
{
    // The token is where the name is written: a name that is qualified (`m::Box`), mangled or
    // renamed by an import is not that text, and its range would be a wrong one.
    if (!lsp_index_is_written(idx, t) || !name || strlen(name) != t.len ||
        strncmp(t.start, name, t.len) != 0)
    {
        return;
    }
    // One node per name: a file may name the same type many times.
    ASTNode *named = idx->type_names;
    while (named && strcmp(named->var_ref.name, name) != 0)
    {
        named = named->next;
    }
    if (!named)
    {
        named = ast_create(NODE_EXPR_VAR);
        named->token = t;
        named->var_ref.name = xstrdup(name);
        named->next = idx->type_names;
        idx->type_names = named;
    }

    LSPRange *r = calloc(1, sizeof(LSPRange));
    r->kind = RANGE_REFERENCE;
    r->start_line = t.line - 1;
    r->start_col = t.col - 1;
    r->end_line = t.line - 1;
    r->end_col = (int)(t.col) - 1 + (int)(t.len);
    r->def_line = -1;
    r->def_col = -1;
    r->node = named;

    lsp_index_add(idx, r);
}

LSPRange *lsp_find_at(LSPIndex *idx, int line, int col)
{
    LSPRange *curr = idx->head;
    LSPRange *best = NULL;

    while (curr)
    {
        if (line >= curr->start_line && line <= curr->end_line)
        {
            if (line == curr->start_line && col < curr->start_col)
            {
                curr = curr->next;
                continue;
            }

            if (line == curr->end_line && col > curr->end_col)
            {
                curr = curr->next;
                continue;
            }

            best = curr;
        }
        curr = curr->next;
    }
    return best;
}

// The declaration as written, from the name up to the body: the parsed types are in their C
// form (int32_t for int, double for f64) and method names are mangled (Rect__area).
int lsp_function_signature(Token name, char *out, size_t size)
{
    const char *p = name.start;
    if (!p || name.kind != TOK_IDENT)
    {
        return 0;
    }
    size_t used = 0;
    str_appendf(out, size, &used, "fn ");
    int depth = 0;
    int pending_space = 0;
    char quote = 0;
    for (; *p && used + 2 < size; p++)
    {
        if (quote)
        {
            if (*p == '\\' && p[1])
            {
                out[used++] = *p++;
            }
            else if (*p == quote)
            {
                quote = 0;
            }
            out[used++] = *p;
            continue;
        }
        if (*p == '/' && p[1] == '/')
        {
            while (p[1] && p[1] != '\n')
            {
                p++;
            }
            pending_space = 1;
            continue;
        }
        if (*p == '/' && p[1] == '*')
        {
            p += 2;
            while (*p && !(*p == '*' && p[1] == '/'))
            {
                p++;
            }
            if (!*p)
            {
                break;
            }
            p++;
            pending_space = 1;
            continue;
        }
        if (depth == 0 && (*p == '{' || *p == ';'))
        {
            break;
        }
        if (isspace((unsigned char)*p))
        {
            pending_space = 1;
            continue;
        }
        if (pending_space && out[used - 1] != '(' && *p != ')' && *p != ',')
        {
            out[used++] = ' ';
        }
        pending_space = 0;
        if (*p == '(' || *p == '[')
        {
            depth++;
        }
        else if (*p == ')' || *p == ']')
        {
            depth--;
        }
        else if (*p == '"' || *p == '\'')
        {
            quote = *p;
        }
        out[used++] = *p;
    }
    out[used] = 0;
    return 1;
}

// The parameters of a function or a lambda, `self` included, are definitions of their own. They
// have no node: rename and references read a definition's name from its node, and the owner's
// would stand for each of its parameters.
static void lsp_index_add_params(ParserContext *ctx, LSPIndex *idx, int count, char **names,
                                 Type **types, Token *tokens)
{
    for (int i = 0; names && tokens && i < count; i++)
    {
        if (!names[i])
        {
            continue;
        }
        char hover[MAX_SHORT_MSG_LEN];
        if (types && types[i] && types[i]->kind != TYPE_UNKNOWN)
        {
            snprintf(hover, sizeof(hover), "%s: %s", names[i], zen_c_type_spelling(ctx, types[i]));
        }
        else
        {
            snprintf(hover, sizeof(hover), "%s", names[i]);
        }
        lsp_index_add_def(idx, tokens[i], hover, NULL);
    }
}

// Walker.

static void lsp_walk_node(ParserContext *ctx, LSPIndex *idx, ASTNode *node, int depth)
{
    if (depth > 32)
    {
        return;
    }

    while (node)
    {
        // Definition logic.
        if (node->kind == NODE_FUNCTION)
        {
            char hover[MAX_SHORT_MSG_LEN * 4];
            if (!lsp_function_signature(node->token, hover, sizeof(hover)))
            {
                const char *name = node->func.name ? node->func.name : "unknown";
                const char *ret = node->func.ret_type_info
                                      ? zen_c_type_spelling(ctx, node->func.ret_type_info)
                                      : "void";
                snprintf(hover, sizeof(hover), "fn %s(...) -> %s", name, ret);
            }
            lsp_index_add_def(idx, node->token, hover, node);
            lsp_index_add_params(ctx, idx, node->func.count, node->func.param_names,
                                 node->func.arg_types, node->func.param_tokens);
        }
        else if (node->kind == NODE_LAMBDA)
        {
            lsp_index_add_params(ctx, idx, node->lambda.count, node->lambda.param_names,
                                 node->type_info ? node->type_info->args : NULL,
                                 node->lambda.param_tokens);
        }
        else if (node->kind == NODE_VAR_DECL)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->var_decl.name ? node->var_decl.name : "unknown";
            snprintf(hover, sizeof(hover), "let %s", name);
            lsp_index_add_def(idx, node->token, hover, node);
        }
        else if (node->kind == NODE_CONST)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->var_decl.name ? node->var_decl.name : "unknown";
            snprintf(hover, sizeof(hover), "def %s", name);
            lsp_index_add_def(idx, node->token, hover, node);
        }
        else if (node->kind == NODE_STRUCT)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->strct.name ? node->strct.name : "unknown";
            snprintf(hover, sizeof(hover), "%sstruct %s", node->strct.is_opaque ? "opaque " : "",
                     name);
            lsp_index_add_def(idx, node->token, hover, node);

            ASTNode *field = node->strct.fields;
            while (field)
            {
                if (field->kind == NODE_FIELD)
                {
                    char fh[MAX_SHORT_MSG_LEN];
                    const char *fname = field->field.name ? field->field.name : "unknown";
                    const char *ftype = zen_c_type_spelling(ctx, field->type_info);
                    snprintf(fh, sizeof(fh), "field %s: %s", fname, ftype);
                    lsp_index_add_def(idx, field->token, fh, field);
                }
                field = field->next;
            }
        }
        else if (node->kind == NODE_ENUM)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->enm.name ? node->enm.name : "unknown";
            snprintf(hover, sizeof(hover), "enum %s", name);
            lsp_index_add_def(idx, node->token, hover, node);

            ASTNode *variant = node->enm.variants;
            while (variant)
            {
                if (variant->kind == NODE_ENUM_VARIANT)
                {
                    char vh[MAX_SHORT_MSG_LEN];
                    const char *vname = variant->variant.name ? variant->variant.name : "unknown";
                    snprintf(vh, sizeof(vh), "variant %s", vname);
                    lsp_index_add_def(idx, variant->token, vh, variant);
                }
                variant = variant->next;
            }
        }
        else if (node->kind == NODE_TYPE_ALIAS)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *alias = node->type_alias.alias ? node->type_alias.alias : "unknown";
            const char *orig =
                node->type_info ? zen_c_type_spelling(ctx, node->type_info) : "unknown";
            snprintf(hover, sizeof(hover), "alias %s = %s", alias, orig);
            lsp_index_add_def(idx, node->token, hover, node);
        }
        else if (node->kind == NODE_TRAIT)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->trait.name ? node->trait.name : "unknown";
            snprintf(hover, sizeof(hover), "trait %s", name);
            lsp_index_add_def(idx, node->token, hover, node);
        }
        else if (node->kind == NODE_PLUGIN)
        {
            lsp_index_add_plugin(idx, node);
        }

        // Reference logic.
        if (node->definition_token.line > 0)
        {
            lsp_index_add_ref(idx, node->token, node->definition_token, node);
        }
        else if (node->kind == NODE_EXPR_STRUCT_INIT)
        {
            // `Box { v: 1 }`: its token is where the type's name is written.
            lsp_index_add_type_ref(idx, node->token, node->struct_init.struct_name);
        }
        else if (node->kind == NODE_EXPR_VAR && node->var_ref.variant.owner &&
                 node->var_ref.variant.owner->name)
        {
            // `Mode::On`: its token is where the enum's name is written.
            lsp_index_add_type_ref(idx, node->token, node->var_ref.variant.owner->name);
        }
        else if (node->kind == NODE_IMPL && node->impl.target)
        {
            // `impl Point`: the type's name, where the header writes it.
            lsp_index_add_type_ref(idx, node->impl.target_token, node->impl.target->name);
        }
        else if (node->kind == NODE_IMPL_TRAIT)
        {
            // `impl Show for Point`: the trait's name and the type's.
            lsp_index_add_type_ref(idx, node->impl_trait.trait_token, node->impl_trait.trait_name);
            if (node->impl_trait.target)
            {
                lsp_index_add_type_ref(idx, node->impl_trait.target_token,
                                       node->impl_trait.target->name);
            }
        }

        // Walk every child list. Trait methods are prototypes, kept out of the index.
        if (node->kind != NODE_TRAIT)
        {
            ASTChildren children = ast_children(node);
            for (int i = 0; i < children.count; i++)
            {
                lsp_walk_node(ctx, idx, *children.slots[i], depth + 1);
            }
        }

        // The expressions of an f-string are not children (ast_children() leaves them out).
        if (node->kind == NODE_EXPR_FSTRING)
        {
            for (ASTNode *part = node->fstring.parts; part; part = part->next)
            {
                lsp_walk_node(ctx, idx, part->fstring_part.expr, depth + 1);
            }
        }

        // Move to next sibling without recursion.
        node = node->next;
    }
}

void lsp_build_index(ParserContext *ctx, LSPIndex *idx, ASTNode *root, const char *source)
{
    idx->source = source;
    if (source)
    {
        idx->source_len = strlen(source);
        idx->line_count = 1;
        for (const char *p = source; *p; p++)
        {
            idx->line_count += *p == '\n';
        }
        idx->line_starts = xmalloc(sizeof(size_t) * (size_t)(idx->line_count));
        int line = 0;
        idx->line_starts[line++] = 0;
        for (size_t i = 0; i < idx->source_len; i++)
        {
            if (source[i] == '\n')
            {
                idx->line_starts[line++] = i + 1;
            }
        }
    }
    lsp_walk_node(ctx, idx, root, 0);
}
