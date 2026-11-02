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
    lambda->lambda.count = 1;

    // Create Type Info: unknown -> unknown
    Type *t = type_new(TYPE_FUNCTION);
    t->inner = type_new(TYPE_INT);
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

    // Update parameter types from symbol table (in case inference happened)
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
    for (int i = 0; i < count; i++)
    {
        param_names[i] = token_strdup(params[i]);
    }
    ASTNode *lambda = ast_create(NODE_LAMBDA);
    lambda->lambda.param_names = param_names;
    lambda->lambda.count = count;
    lambda->lambda.default_capture_mode = default_capture_mode;

    // Type Info construction
    Type *t = type_new(TYPE_FUNCTION);
    t->inner = type_new(TYPE_INT);
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
// gave it, by its Zen C name as in a tuple type (a @link_name only shows in C). A primitive is
// read from the C type codegen writes for it: an integer literal is an `int` until then, and
// `(1, 2)` must be the tuple `(int, int)` is, of `int32_t`s.
static Type *literal_element_type(ParserContext *ctx, ASTNode *element)
{
    Type *parsed = element->type_info;
    if (parsed && (parsed->kind == TYPE_STRUCT || parsed->kind == TYPE_ENUM ||
                   parsed->kind == TYPE_ALIAS || parsed->kind == TYPE_TUPLE))
    {
        return parsed;
    }
    char *inferred = infer_type(ctx, element);
    if (!inferred)
    {
        return type_new(TYPE_INT);
    }
    char *text = xstrdup(inferred);
    int pointers = 0;
    size_t length = strlen(text);
    while (length > 0 && text[length - 1] == '*')
    {
        length--;
        pointers++;
    }
    text[length] = 0;
    TypeKind kind = find_primitive_kind(text);
    Type *t = kind != TYPE_UNKNOWN ? type_new(kind) : type_from_string_helper(ctx, text);
    if (!t)
    {
        t = type_from_name(ctx, text);
    }
    while (pointers-- > 0)
    {
        t = type_new_ptr(t);
    }
    return t;
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

    // While the language server indexes the workspace, the literal is left without a type.
    extern int g_is_indexing;
    if (g_is_indexing)
    {
        return n;
    }

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
            elements[i++] = literal_element_type(ctx, curr);
        }
        n->type_info = type_new_tuple(elements, count);
        register_tuple(ctx, n->type_info);
    }
    return n;
}
