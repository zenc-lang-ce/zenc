// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "ast/ast.h"
#include "analysis/move_check.h"
#include "plugins/plugin_manager.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analysis/const_fold.h"
#include "utils/utils.h"
#include "ast/primitives.h"

void set_def_value(ZenSymbol *s, long long val)
{
    s->is_const_value = 1;
    s->const_int_val = val;
    s->is_def = 1;

    // Auto-infer type for def if unknown: `int`, or `i64` when the
    // value does not fit in 32 bits.
    if (!s->type_info || s->type_info->kind == TYPE_UNKNOWN)
    {
        int is_wide = val < INT32_MIN || val > INT32_MAX;
        if (s->type_info)
        {
            zfree(s->type_info);
        }
        s->type_info = type_new(is_wide ? TYPE_I64 : TYPE_I32);
        s->is_untyped = !is_wide;
        s->type_info->is_const = 1;
    }
}

void read_def_value(ParserContext *ctx, Lexer *l, ASTNode *node, ZenSymbol *symbol, int may_wait)
{
    Type *type_obj = node->type_info;
    if (type_obj && type_obj->kind == TYPE_TUPLE && tuple_literal_follows(l))
    {
        node->var_decl.init_expr = parse_tuple_expression(ctx, l, type_obj, NULL);
        return;
    }
    ASTNode *value = parse_expression(ctx, l);
    node->var_decl.init_expr = value;
    if (!symbol)
    {
        return;
    }

    // Its integer value, for the symbol table. One that names a `def` whose value is not known
    // yet is computed once every value is read (resolve_pending_constants).
    long long val;
    if (eval_const_int_expr(value, ctx, &val))
    {
        set_def_value(symbol, val);
    }
    else if (may_wait)
    {
        wait_for_constant(ctx, value, NULL, symbol, node->token, NULL);
    }
    // Any other value without a type gives the `def` its type, as it gives a `let` its own: a
    // float or a string literal, a struct literal.
    if (symbol->type_info->kind == TYPE_UNKNOWN && value && value->type_info &&
        value->type_info->kind != TYPE_UNKNOWN)
    {
        symbol->type_info = type_clone(value->type_info);
        symbol->type_info->is_const = 1;
    }
}

ASTNode *parse_def(ParserContext *ctx, Lexer *l, int is_export)
{
    lexer_next(l); // eat def
    Token n = lexer_next(l);

    Type *type_obj = NULL;

    if (lexer_peek(l).kind == TOK_COLON)
    {
        lexer_next(l);
        // Hybrid Parse
        type_obj = parse_type_formal(ctx, l);
        if (!type_obj)
        {
            return NULL;
        }
    }

    char *ns = token_strdup(n);
    // The node keeps the type as written (a NODE_CONST is const already); its symbol, a const
    // copy, or an unknown type until the value tells it.
    Type *sym_type = type_obj ? type_clone(type_obj) : type_new(TYPE_UNKNOWN);
    sym_type->is_const = 1;

    // Use is_def flag for manifest constants
    add_symbol_with_token(ctx, ns, sym_type, n, is_export);
    ZenSymbol *sym_entry = find_symbol_entry(ctx, ns);
    if (sym_entry)
    {
        sym_entry->kind = SYM_CONSTANT;
        sym_entry->is_def = 1;
        // is_const_value set only if literal
    }

    if (lexer_peek(l).kind != TOK_OP || !is_token(lexer_peek(l), "="))
    {
        zpanic_at(n, "'def' constants must be initialized");
        return NULL;
    }
    lexer_next(l);

    ASTNode *o = ast_create(NODE_CONST);
    o->token = n;
    o->var_decl.name = ns;
    o->type_info = type_obj;

    // A global's value is read once every declaration is: it may name one declared further down
    // (parse_pending_global_values).
    int is_global = !ctx->current_scope || !ctx->current_scope->parent;
    if (is_global && !ctx->declarations_read)
    {
        wait_for_global_value(ctx, o, NULL, *l);
        if (!skip_expression(l) || lexer_peek(l).kind != TOK_SEMICOLON)
        {
            zpanic_at(lexer_peek(l), "Expected ';' after the value of '%s'", ns);
            return NULL;
        }
    }
    else
    {
        read_def_value(ctx, l, o, sym_entry, 0);
    }

    if (lexer_peek(l).kind == TOK_SEMICOLON)
    {
        lexer_next(l);
    }

    if (is_global)
    {
        collect_declaration(ctx, o);
    }

    return o;
}

ASTNode *parse_type_alias(ParserContext *ctx, Lexer *l, int is_opaque, int is_export)
{
    lexer_next(l); // consume 'type' or 'alias'
    Token n = lexer_next(l);
    if (n.kind != TOK_IDENT)
    {
        zpanic_at(n, "Expected identifier for type alias");
        return NULL;
    }
    check_type_name(n);

    lexer_next(l); // consume '='

    Type *t = parse_type_formal(ctx, l);
    if (!t)
    {
        return NULL;
    }

    if (lexer_peek(l).kind == TOK_SEMICOLON)
    {
        lexer_next(l);
    }

    ASTNode *node = ast_create(NODE_TYPE_ALIAS);
    node->token = n;
    node->type_alias.alias = xmalloc(n.len + 1);
    strncpy(node->type_alias.alias, n.start, n.len);
    node->type_alias.alias[n.len] = 0;
    node->type_info = t;
    node->type_alias.is_opaque = is_opaque;
    node->type_alias.defined_in_file =
        ctx->current_filename ? xstrdup(ctx->current_filename) : NULL;
    node->type_alias.is_export = is_export;

    collect_declaration(ctx, node);

    return node;
}
