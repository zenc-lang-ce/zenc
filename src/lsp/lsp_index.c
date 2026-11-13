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

int lsp_index_is_written(const LSPIndex *idx, Token t)
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

// A range over the token `t`, with the name written there.
static LSPRange *lsp_range_at(Token t, RangeType kind)
{
    LSPRange *r = calloc(1, sizeof(LSPRange));
    r->kind = kind;
    r->start_line = t.line - 1;
    r->start_col = t.col - 1;
    r->end_line = t.line - 1;
    r->end_col = (int)(t.col) - 1 + (int)(t.len);
    r->symbol.name = token_strdup(t);
    return r;
}

void lsp_index_add_def(LSPIndex *idx, Token t, DeclarationKind kind, const char *hover,
                       ASTNode *node)
{
    if (!lsp_index_is_written(idx, t))
    {
        return;
    }
    LSPRange *r = lsp_range_at(t, RANGE_DEFINITION);
    r->symbol.kind = kind;
    r->symbol.definition = t;
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
    LSPRange *r = lsp_range_at(t, RANGE_REFERENCE);
    r->symbol.definition = def_t;
    r->node = node;

    lsp_index_add(idx, r);
}

static LSPVisibleType g_visible_type = NULL;

void lsp_index_set_visible_type(LSPVisibleType visible_type)
{
    g_visible_type = visible_type;
}

// The name token of the declaration that `ctx` registered for the type `name` used in `file`;
// TOKEN_UNKNOWN if there is none. The server parses the whole workspace with one context, so other
// files' types of that name are registered too: the one `file` sees (see LSPVisibleType) is the one
// it names, and otherwise its own declaration.
static Token type_declaration(ParserContext *ctx, const char *name, const char *file)
{
    ASTNode *visible = g_visible_type && file ? g_visible_type(file, name) : NULL;
    if (visible)
    {
        return visible->token;
    }
    Token found[3] = {TOKEN_UNKNOWN, TOKEN_UNKNOWN, TOKEN_UNKNOWN};
    ZenSymbol *sym = ctx->global_scope ? symbol_lookup_local(ctx->global_scope, name) : NULL;
    if (sym && (sym->kind == SYM_STRUCT || sym->kind == SYM_ENUM || sym->kind == SYM_ALIAS))
    {
        found[0] = sym->decl_token;
    }
    // A generic struct or enum is a template, not a symbol; nor is a trait.
    GenericTemplate *template = find_generic_template(ctx, name);
    if (template && template->struct_node)
    {
        found[1] = template->struct_node->token;
    }
    ASTNode *trait = find_trait_def(ctx, name);
    if (trait)
    {
        found[2] = trait->token;
    }

    for (int i = 0; i < 3; i++)
    {
        if (found[i].file && file && strcmp(found[i].file, file) == 0)
        {
            return found[i];
        }
    }
    for (int i = 0; i < 3; i++)
    {
        if (found[i].line > 0)
        {
            return found[i];
        }
    }
    return TOKEN_UNKNOWN;
}

void lsp_index_add_type_ref(LSPIndex *idx, ParserContext *ctx, Token t, const char *name)
{
    // The token is where the name is written: a name that is qualified (`m::Box`), mangled or
    // renamed by an import is not that text, and its range would be a wrong one.
    if (!lsp_index_is_written(idx, t) || !name || strlen(name) != t.len ||
        strncmp(t.start, name, t.len) != 0)
    {
        return;
    }
    lsp_index_add_ref(idx, t, type_declaration(ctx, name, t.file), NULL);
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

int lsp_range_names(const LSPRange *r, Token definition)
{
    Token own = r->symbol.definition;
    return own.line > 0 && own.line == definition.line && own.col == definition.col && own.file &&
           definition.file && strcmp(own.file, definition.file) == 0;
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

// The parameters of a function or a lambda, `self` included: they are no nodes of their own.
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
        DeclarationKind kind =
            strcmp(names[i], "self") == 0 ? DECLARATION_RECEIVER : DECLARATION_PARAMETER;
        lsp_index_add_def(idx, tokens[i], kind, hover, NULL);
    }
}

// A `let` or `def` named `name` and written at `t`, with its type when it is known. A declaration
// the parser generates under a name that is not the token's (`__it` of a `for ... in`) is none.
static void add_variable_def(ParserContext *ctx, LSPIndex *idx, Token t, const char *name,
                             Type *type, DeclarationKind kind, ASTNode *node)
{
    if (!name || !t.start || strlen(name) != t.len || strncmp(t.start, name, t.len) != 0)
    {
        return;
    }
    const char *keyword = kind == DECLARATION_CONSTANT ? "def" : "let";
    char hover[MAX_SHORT_MSG_LEN];
    if (type && type->kind != TYPE_UNKNOWN)
    {
        snprintf(hover, sizeof(hover), "%s %s: %s", keyword, name, zen_c_type_spelling(ctx, type));
    }
    else
    {
        snprintf(hover, sizeof(hover), "%s %s", keyword, name);
    }
    lsp_index_add_def(idx, t, kind, hover, node);
}

// The enum named `name` as `ctx` registered it: a declared one, an instance, or a template.
static ASTNode *enum_named(ParserContext *ctx, const char *name)
{
    ASTNode *def = name ? find_struct_def(ctx, name) : NULL;
    if (def && def->kind == NODE_ENUM)
    {
        return def;
    }
    GenericTemplate *template = name ? find_generic_template(ctx, name) : NULL;
    return template && template->struct_node && template->struct_node->kind == NODE_ENUM
               ? template->struct_node
               : NULL;
}

// The references of the patterns of a `match` on a value of type `subject`: the enum of
// `Color::Red` where it is written, and the variant, of that enum or of the subject's.
static void add_pattern_refs(LSPIndex *idx, ParserContext *ctx, ASTNode *match, Type *subject)
{
    while (subject && subject->kind == TYPE_POINTER)
    {
        subject = subject->inner;
    }
    ASTNode *subject_enum = subject && (subject->kind == TYPE_ENUM || subject->kind == TYPE_STRUCT)
                                ? enum_named(ctx, type_def_name(subject))
                                : NULL;
    for (ASTNode *c = match->match_stmt.cases; c; c = c->next)
    {
        if (c->kind != NODE_MATCH_CASE)
        {
            continue;
        }
        for (MatchPattern *p = c->match_case.patterns; p; p = p->next)
        {
            if (p->kind != PATTERN_NAME)
            {
                continue;
            }
            ASTNode *enm = subject_enum;
            if (p->scope_token.line > 0)
            {
                char *scope = token_strdup(p->scope_token);
                lsp_index_add_type_ref(idx, ctx, p->scope_token, scope);
                enm = enum_named(ctx, scope);
            }
            ASTNode *v = enm ? enm->enm.variants : NULL;
            while (v && !(v->variant.name && strcmp(v->variant.name, p->name.name) == 0))
            {
                v = v->next;
            }
            if (v)
            {
                lsp_index_add_ref(idx, p->name_token, v->token, NULL);
            }
            else if (!enm && subject && subject->kind == TYPE_STRUCT && p->scope_token.line <= 0)
            {
                // `Some(x)` on an Option: the struct's method of that name.
                FuncSig *method = find_func(
                    ctx, mangle_method_symbol(type_def_name(subject), NULL, p->name.name));
                if (method)
                {
                    lsp_index_add_ref(idx, p->name_token, method->decl_token, NULL);
                }
            }
        }
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
            lsp_index_add_def(idx, node->token, DECLARATION_FUNCTION, hover, node);
            lsp_index_add_params(ctx, idx, node->func.count, node->func.param_names,
                                 node->func.arg_types, node->func.param_tokens);
        }
        else if (node->kind == NODE_LAMBDA)
        {
            lsp_index_add_params(ctx, idx, node->lambda.count, node->lambda.param_names,
                                 node->type_info ? node->type_info->args : NULL,
                                 node->lambda.param_tokens);
        }
        else if (node->kind == NODE_VAR_DECL || node->kind == NODE_CONST)
        {
            Type *type = node->var_decl.type_info ? node->var_decl.type_info : node->type_info;
            add_variable_def(ctx, idx, node->token, node->var_decl.name, type,
                             node->kind == NODE_CONST ? DECLARATION_CONSTANT : DECLARATION_VARIABLE,
                             node);
        }
        else if (node->kind == NODE_FOR_RANGE)
        {
            // `for i in 0..n`: no node declares `i`.
            add_variable_def(ctx, idx, node->for_range.var_token, node->for_range.var_name,
                             type_new(TYPE_INT), DECLARATION_VARIABLE, NULL);
        }
        else if (node->kind == NODE_STRUCT)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->strct.name ? node->strct.name : "unknown";
            snprintf(hover, sizeof(hover), "%sstruct %s", node->strct.is_opaque ? "opaque " : "",
                     name);
            lsp_index_add_def(idx, node->token, DECLARATION_STRUCT, hover, node);

            ASTNode *field = node->strct.fields;
            while (field)
            {
                if (field->kind == NODE_FIELD)
                {
                    char fh[MAX_SHORT_MSG_LEN];
                    const char *fname = field->field.name ? field->field.name : "unknown";
                    const char *ftype = zen_c_type_spelling(ctx, field->type_info);
                    snprintf(fh, sizeof(fh), "field %s: %s", fname, ftype);
                    lsp_index_add_def(idx, field->token, DECLARATION_FIELD, fh, field);
                }
                field = field->next;
            }
        }
        else if (node->kind == NODE_ENUM)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->enm.name ? node->enm.name : "unknown";
            snprintf(hover, sizeof(hover), "enum %s", name);
            lsp_index_add_def(idx, node->token, DECLARATION_ENUM, hover, node);

            ASTNode *variant = node->enm.variants;
            while (variant)
            {
                if (variant->kind == NODE_ENUM_VARIANT)
                {
                    // `Shape::Rect(f32, f32)`, as a program writes the variant.
                    char vh[MAX_SHORT_MSG_LEN];
                    const char *vname = variant->variant.name ? variant->variant.name : "unknown";
                    Type *payload = variant->variant.payload;
                    if (!payload)
                    {
                        snprintf(vh, sizeof(vh), "%s::%s", name, vname);
                    }
                    else if (payload->kind == TYPE_TUPLE)
                    {
                        snprintf(vh, sizeof(vh), "%s::%s%s", name, vname,
                                 zen_c_type_spelling(ctx, payload));
                    }
                    else
                    {
                        snprintf(vh, sizeof(vh), "%s::%s(%s)", name, vname,
                                 zen_c_type_spelling(ctx, payload));
                    }
                    lsp_index_add_def(idx, variant->token, DECLARATION_VARIANT, vh, variant);
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
            lsp_index_add_def(idx, node->token, DECLARATION_ALIAS, hover, node);
        }
        else if (node->kind == NODE_TRAIT)
        {
            char hover[MAX_SHORT_MSG_LEN];
            const char *name = node->trait.name ? node->trait.name : "unknown";
            snprintf(hover, sizeof(hover), "trait %s", name);
            lsp_index_add_def(idx, node->token, DECLARATION_TRAIT, hover, node);
        }
        else if (node->kind == NODE_PLUGIN)
        {
            lsp_index_add_plugin(idx, node);
        }

        // Reference logic.
        if (node->definition_token.line > 0)
        {
            // A call through a path (`Id::of(1)`) refers to its function where the path writes
            // it, and `Mode::On` to its variant; the path's start is the type's name.
            Token at = (node->kind == NODE_EXPR_CALL && node->call.member_token.line > 0)
                           ? node->call.member_token
                       : (node->kind == NODE_EXPR_VAR && node->var_ref.variant.token.line > 0)
                           ? node->var_ref.variant.token
                           : node->token;
            lsp_index_add_ref(idx, at, node->definition_token, node);
        }
        if (node->kind == NODE_MATCH && node->match_stmt.expr)
        {
            add_pattern_refs(idx, ctx, node, node->match_stmt.expr->type_info);
        }
        if (node->kind == NODE_EXPR_STRUCT_INIT)
        {
            // `Box { v: 1 }`: its token is where the type's name is written.
            lsp_index_add_type_ref(idx, ctx, node->token, node->struct_init.struct_name);
        }
        else if (node->kind == NODE_IMPL && node->impl.target)
        {
            // `impl Point`: the type's name, where the header writes it.
            lsp_index_add_type_ref(idx, ctx, node->impl.target_token, node->impl.target->name);
        }
        else if (node->kind == NODE_IMPL_TRAIT)
        {
            // `impl Show for Point`: the trait's name and the type's.
            lsp_index_add_type_ref(idx, ctx, node->impl_trait.trait_token,
                                   node->impl_trait.trait_name);
            if (node->impl_trait.target)
            {
                lsp_index_add_type_ref(idx, ctx, node->impl_trait.target_token,
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
