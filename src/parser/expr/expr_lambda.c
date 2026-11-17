// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "expr_internal.h"
#include "ast/ast.h"
#include "ast/primitives.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode *parse_arrow_lambda_single(ParserContext *ctx, Lexer *l, Token param,
                                   int default_capture_mode)
{
    char *param_name = token_strdup(param);
    ASTNode *lambda = ast_create(NODE_LAMBDA);
    lambda->lambda.count = 1;
    lambda->lambda.default_capture_mode = default_capture_mode;
    lambda->lambda.param_names = xmalloc(sizeof(char *));
    lambda->lambda.param_names[0] = param_name;
    lambda->lambda.param_tokens = xmalloc(sizeof(Token));
    lambda->lambda.param_tokens[0] = param;
    lambda->lambda.count = 1;

    // Create Type Info: unknown -> unknown
    Type *t = type_new(TYPE_FUNCTION);
    t->inner = type_new(TYPE_I32);
    t->args = xmalloc(sizeof(Type *));
    t->args[0] = type_new(TYPE_UNKNOWN); // Arg
    t->count = 1;
    lambda->type_info = t;

    // Register parameter in scope for body parsing
    enter_scope(ctx);
    add_symbol_with_token(ctx, param_name, t->args[0], param, 0);

    // Body parsing...
    ASTNode *body_block = NULL;
    if (lexer_peek(l).kind == TOK_LBRACE)
    {
        body_block = parse_block(ctx, l);
    }
    else
    {
        ASTNode *expr = parse_expression(ctx, l);
        ASTNode *ret = ast_create(NODE_RETURN);
        ret->ret.value = expr;
        body_block = ast_create(NODE_BLOCK);
        body_block->block.statements = ret;
    }
    lambda->lambda.body = body_block;

    // Attempt to infer return type from body if it's a simple return
    if (lambda->lambda.body->block.statements &&
        lambda->lambda.body->block.statements->kind == NODE_RETURN &&
        !lambda->lambda.body->block.statements->next)
    {
        ASTNode *ret_val = lambda->lambda.body->block.statements->ret.value;
        if (ret_val->type_info && ret_val->type_info->kind != TYPE_UNKNOWN)
        {
            if (param_name[0] == 'x')
            {
            }
            // Update return type
            if (t->inner)
            {
                zfree(t->inner);
            }
            t->inner = ret_val->type_info;
        }
    }

#if FF_COMPATIBILITY_0_5
    // The body may have given the parameter a type (`x => x * 2`: see parse_expr_prec_impl).
    ZenSymbol *sym = find_symbol_entry(ctx, param_name);
    if (sym && sym->type_info && sym->type_info->kind != TYPE_UNKNOWN)
    {
        t->args[0] = sym->type_info;
    }
    else
    {
        t->args[0] = type_new(TYPE_UNKNOWN);

        if (sym)
        {
            sym->type_info = t->args[0]; // Bind AST node type implicitly!
        }
    }
#endif

    lambda->lambda.lambda_id = ctx->lambda_counter++;
    lambda->lambda.is_expression = 1;
    if (ctx->known_generics_count == 0)
    {
        register_lambda(ctx, lambda);
    }
    analyze_lambda_captures(ctx, lambda);
    check_unused_parameters(ctx, ctx->current_scope, NULL);
    exit_scope(ctx);
    return lambda;
}

ASTNode *parse_arrow_lambda_multi(ParserContext *ctx, Lexer *l, Token *params, Type **param_types,
                                  int count, int default_capture_mode)
{
    char **param_names = xmalloc(sizeof(char *) * (size_t)(count));
    Token *param_tokens = xmalloc(sizeof(Token) * (size_t)(count));
    for (int i = 0; i < count; i++)
    {
        param_names[i] = token_strdup(params[i]);
        param_tokens[i] = params[i];
    }
    ASTNode *lambda = ast_create(NODE_LAMBDA);
    lambda->lambda.param_names = param_names;
    lambda->lambda.param_tokens = param_tokens;
    lambda->lambda.count = count;
    lambda->lambda.default_capture_mode = default_capture_mode;

    // Type Info construction
    Type *t = type_new(TYPE_FUNCTION);
    t->inner = type_new(TYPE_I32);
    t->args = xmalloc(sizeof(Type *) * (size_t)(count));
    t->count = count;

    for (int i = 0; i < count; i++)
    {
        t->args[i] = (param_types && param_types[i]) ? param_types[i] : type_new(TYPE_UNKNOWN);
    }
    lambda->type_info = t;

    // Register parameters in scope for body parsing
    enter_scope(ctx);
    for (int i = 0; i < count; i++)
    {
        if (param_types && param_types[i])
        {
            add_symbol_with_token(ctx, param_names[i], param_types[i], params[i], 0);
        }
        else
        {
            add_symbol_with_token(ctx, param_names[i], t->args[i], params[i], 0);
        }
    }

    // Body parsing...
    ASTNode *body_block = NULL;
    if (lexer_peek(l).kind == TOK_LBRACE)
    {
        body_block = parse_block(ctx, l);
    }
    else
    {
        ASTNode *expr = parse_expression(ctx, l);
        ASTNode *ret = ast_create(NODE_RETURN);
        ret->ret.value = expr;
        body_block = ast_create(NODE_BLOCK);
        body_block->block.statements = ret;
    }
    lambda->lambda.body = body_block;
    lambda->lambda.lambda_id = ctx->lambda_counter++;
    lambda->lambda.is_expression = 1;
    if (ctx->known_generics_count == 0)
    {
        register_lambda(ctx, lambda);
    }
    analyze_lambda_captures(ctx, lambda);
    check_unused_parameters(ctx, ctx->current_scope, NULL);
    exit_scope(ctx);
    return lambda;
}

// The type of an element of a tuple literal. A struct, enum, alias or tuple is the type the parser
// gave it, by its Zen C name as in a tuple type (a @link_name only shows in C). A primitive is a
// type of its own with that kind (and `const`), not the element's: an integer literal is an `i32`,
// and `(1, 2)` must be the tuple `(int, int)` is. An element without a type is an `i32`.
static Type *literal_element_type(ASTNode *element)
{
    Type *parsed = element->type_info;
    if (!parsed || parsed->kind == TYPE_UNKNOWN)
    {
        return type_new(TYPE_I32);
    }
    if (parsed->kind == TYPE_STRUCT || parsed->kind == TYPE_ENUM || parsed->kind == TYPE_ALIAS ||
        parsed->kind == TYPE_TUPLE)
    {
        return parsed;
    }
    if (parsed->inner || parsed->args)
    {
        return type_clone(parsed);
    }
    Type *t = type_new(parsed->kind);
    t->is_const = parsed->is_const;
    return t;
}

int tuple_literal_follows(Lexer *l)
{
    if (lexer_peek(l).kind != TOK_LPAREN)
    {
        return 0;
    }
    Lexer ahead = *l;
    int depth = 0;
    while (1)
    {
        Token t = lexer_next(&ahead);
        if (t.kind == TOK_EOF || t.kind == TOK_SEMICOLON)
        {
            return 0;
        }
        if (t.kind == TOK_LPAREN || t.kind == TOK_LBRACKET || t.kind == TOK_LBRACE)
        {
            depth++;
        }
        else if (t.kind == TOK_RPAREN || t.kind == TOK_RBRACKET || t.kind == TOK_RBRACE)
        {
            if (--depth == 0)
            {
                return 0;
            }
        }
        else if (depth == 1 && t.kind == TOK_COMMA)
        {
            return 1;
        }
    }
}

ASTNode *parse_tuple_expression(ParserContext *ctx, Lexer *l, Type *tuple, ASTNode *first_elem)
{
    Token tk;
    if (!first_elem)
    {
        tk = lexer_next(l);
    }
    else
    {
        tk = first_elem->token;
    }

    ASTNode *head = first_elem, *prev = first_elem;
    int count = (first_elem ? 1 : 0);

    // If first_elem was provided, we might be at a comma or the closing paren
    if (first_elem && lexer_peek(l).kind == TOK_COMMA)
    {
        lexer_next(l); // eat comma
    }

    while (lexer_peek(l).kind != TOK_RPAREN)
    {
        ASTNode *element = parse_expression(ctx, l);
        if (head == NULL)
        {
            head = element;
        }
        else
        {
            prev->next = element;
        }
        prev = element;
        count++;

        if (lexer_peek(l).kind == TOK_COMMA)
        {
            lexer_next(l);
        }
        else
        {
            break;
        }
    }

    if (lexer_next(l).kind != TOK_RPAREN)
    {
        zpanic_at(lexer_peek(l), "Expected ) after tuple literal");
        return NULL;
    }

    ASTNode *n = ast_create(NODE_EXPR_TUPLE_LITERAL);
    n->token = tk;
    n->tuple_literal.elements = head;
    n->tuple_literal.count = count;

    if (tuple)
    {
        n->type_info = tuple;
    }
    else
    {
        Type **elements = xmalloc(sizeof(Type *) * (size_t)(count > 0 ? count : 1));
        int i = 0;
        for (ASTNode *curr = head; curr; curr = curr->next)
        {
            elements[i++] = literal_element_type(curr);
        }
        n->type_info = type_new_tuple(elements, count);
        register_tuple(ctx, n->type_info);
    }
    return n;
}
