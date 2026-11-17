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

void replace_it_with_var(ASTNode *node, char *var_name)
{
    if (!node)
    {
        return;
    }
    if (node->kind == NODE_EXPR_VAR)
    {
        if (strcmp(node->var_ref.name, "it") == 0)
        {
            // Replace 'it' with var_name
            node->var_ref.name = xstrdup(var_name);
        }
        return;
    }

    // A lambda had its captures worked out when it was parsed, while it still said `it`;
    // renaming inside it now would make its body and its captures disagree.
    if (node->kind == NODE_LAMBDA)
    {
        return;
    }

    ASTChildren children = ast_children(node);
    for (int i = 0; i < children.count; i++)
    {
        for (ASTNode *child = *children.slots[i]; child; child = child->next)
        {
            replace_it_with_var(child, var_name);
        }
    }
}

// The value of `let name[: type] = value` at `l` (after the `=`), read as an expression of the
// type written, *type_obj_io (NULL when none): when no type is written, *type_obj_io and *type_io
// become the value's. NULL when it cannot be read.
static ASTNode *read_var_value(ParserContext *ctx, Lexer *l, char **type_io, Type **type_obj_io)
{
    char *type = *type_io;
    Type *type_obj = *type_obj_io;
    ASTNode *init = NULL;
    // Peek for special initializers
    Token next = lexer_peek(l);
    // The `;` after the value is the declaration's.
    if (next.kind == TOK_IDENT && strncmp(next.start, "embed", 5) == 0)
    {
        init = parse_embed(ctx, l);
    }
    else if (type_obj && type_obj->kind == TYPE_TUPLE && tuple_literal_follows(l))
    {
        init = parse_tuple_expression(ctx, l, type_obj, NULL);
    }
    else
    {
        ctx->cg.expected_init_type = type_obj;
        init = parse_expression(ctx, l);
        ctx->cg.expected_init_type = NULL;
    }

    if (init && type)
    {
        char *rhs_type = init->type_info ? type_mangle_spelling(init->type_info) : NULL;

        if (rhs_type && strchr(type, '*') && strchr(rhs_type, '*'))
        {
            // Strip stars to get struct names
            char target_struct[MAX_TYPE_NAME_LEN];
            strcpy(target_struct, type);
            target_struct[strlen(target_struct) - 1] = 0;
            char source_struct[MAX_TYPE_NAME_LEN];
            strcpy(source_struct, rhs_type);
            source_struct[strlen(source_struct) - 1] = 0;

            ASTNode *def = find_struct_def(ctx, source_struct);

            if (def && def->strct.parent && strcmp(def->strct.parent, target_struct) == 0)
            {
                // Create Cast Node
                ASTNode *cast = ast_create(NODE_EXPR_CAST);
                cast->cast.expr = init;
                cast->type_info = type_obj; // Inherit formal type

                init = cast; // Replace init with cast
            }
        }
    }

    // ** Type Inference Logic **
    if (!type && init)
    {
        if (init->type_info)
        {
            // Create new type to avoid inheriting is_const from builtins like true/false
            type_obj = type_new(init->type_info->kind);
            if (init->type_info->name)
            {
                type_obj->name = xstrdup(init->type_info->name);
            }
            if (init->type_info->inner)
            {
                type_obj->inner = init->type_info->inner; // Shallow copy for inner
            }
            if (init->type_info->kind == TYPE_ALIAS)
            {
                type_obj->alias = init->type_info->alias;
            }
            // Copy function type args for lambda/closure support
            if (init->type_info->args && init->type_info->count > 0)
            {
                type_obj->args = init->type_info->args;
                type_obj->count = init->type_info->count;
                type_obj->is_varargs = init->type_info->is_varargs;
            }
            type_obj->array_size = init->type_info->array_size;
            type_obj->is_raw = init->type_info->is_raw;
            type_obj->is_explicit_struct = init->type_info->is_explicit_struct;
            type = type_mangle_spelling(type_obj);
        }
        else if (init->kind == NODE_EXPR_SLICE)
        {
            zpanic_at(init->token, "Slice Node has NO Type Info!");
            return NULL;
        }
        else if (init->kind == NODE_EXPR_STRUCT_INIT)
        {
            type = xstrdup(init->struct_init.struct_name);
            type_obj = type_new(TYPE_STRUCT);
            type_obj->name = xstrdup(type);
        }
    }
    *type_io = type;
    *type_obj_io = type_obj;
    return init;
}

// Checks the value `init` of the variable `name` against its type `type_obj`, and records an
// integer literal as the variable's constant value. 0 when the value does not fit the type.
static int check_var_value(ParserContext *ctx, const char *name, Type *type_obj, ASTNode *init)
{
    if (init && type_obj)
    {
        Type *t = init->type_info;
        if (!t && init->kind == NODE_EXPR_VAR)
        {
            t = find_symbol_type_info(ctx, init->var_ref.name);
        }

        // Parser-level type validation: catches obvious mismatches like assigning
        // string to int. More comprehensive type checking happens in the typechecker.
        if (t && !type_eq(type_obj, t))
        {
            // Allow conversions that are safe or handled by the typechecker:
            // numeric (int->float), pointer (char*->void*), function (closures),
            // struct/array (Slice__char vs Slice_char naming), and opaque alias compat.
            int is_numeric = (is_integer_type(type_obj) || is_float_type(type_obj)) &&
                             (is_integer_type(t) || is_float_type(t));
            int is_pointer = (type_obj->kind == TYPE_POINTER) || (t->kind == TYPE_POINTER);
            int is_function = (type_obj->kind == TYPE_FUNCTION) || (t->kind == TYPE_FUNCTION);
            int is_struct = (type_obj->kind == TYPE_STRUCT) || (t->kind == TYPE_STRUCT);
            int is_array = (type_obj->kind == TYPE_ARRAY) || (t->kind == TYPE_ARRAY);
            int is_opaque = (type_obj->kind == TYPE_ALIAS && type_obj->alias.is_opaque_alias) ||
                            (t->kind == TYPE_ALIAS && t->alias.is_opaque_alias);
            if (!is_numeric && !is_pointer && !is_function && !is_struct && !is_array &&
                (!is_opaque || !check_opaque_alias_compat(ctx, type_obj, t)))
            {
                char *expected = zen_c_type_spelling(ctx, type_obj);
                char *got = zen_c_type_spelling(ctx, t);
                zpanic_at(init->token, "Type validation failed. Expected '%s', but got '%s'",
                          expected, got);
                return 0;
                zfree(expected);
                zfree(got);
            }
        }
    }

    // NEW: Capture Const Integer Values
    if (init && init->kind == NODE_EXPR_LITERAL && init->literal.kind == LITERAL_INT)
    {
        ZenSymbol *s = find_symbol_entry(ctx, name); // Helper to find the struct
        if (s)
        {
            s->is_const_value = 1;
            s->const_int_val = (long long)init->literal.int_val;
        }
    }
    return 1;
}

// An interpolated string runs at run time, so it cannot be a global's value: the C would put a
// statement expression (`({ ... })`) at file scope. Only that is rejected: `embed "file" as
// string` is a raw statement too, but a constant.
static void check_global_value(ASTNode *init)
{
    if (init && init->kind == NODE_EXPR_FSTRING)
    {
        zerror_at(init->token,
                  "A global cannot be initialized with an interpolated string (it is built "
                  "at run time); move it into a function, or write literal braces as "
                  "'{{' and '}}'");
    }
}

void wait_for_global_value(ParserContext *ctx, ASTNode *node, char *type, Lexer start)
{
    PendingGlobalValue *p = xcalloc(1, sizeof(PendingGlobalValue));
    p->node = node;
    p->type = type;
    p->start = start;
    p->module_prefix = ctx->imports.current_module_prefix;
    p->filename = ctx->current_filename;
    p->symbol = symbol_lookup_local(ctx->current_scope ? ctx->current_scope : ctx->global_scope,
                                    node->var_decl.name);
    p->next = ctx->pending_global_values;
    ctx->pending_global_values = p;
}

void parse_pending_global_values(ParserContext *ctx)
{
    // In the order written: a value may name a global written above it.
    PendingGlobalValue *in_order = NULL;
    while (ctx->pending_global_values)
    {
        PendingGlobalValue *p = ctx->pending_global_values;
        ctx->pending_global_values = p->next;
        p->next = in_order;
        in_order = p;
    }
    char *saved_prefix = ctx->imports.current_module_prefix;
    const char *saved_filename = ctx->current_filename;
    Scope *saved_scope = ctx->current_scope;
    ctx->current_scope = ctx->global_scope;
    for (PendingGlobalValue *p = in_order; p; p = p->next)
    {
        ASTNode *node = p->node;
        ctx->imports.current_module_prefix = p->module_prefix;
        ctx->current_filename = p->filename;
        Lexer l = p->start;
        char *type = p->type;
        Type *type_obj = node->var_decl.type_info;
        ASTNode *init = read_var_value(ctx, &l, &type, &type_obj);
        if (!init)
        {
            continue;
        }
        Token after = lexer_peek(&l);
        if (after.kind != TOK_SEMICOLON && after.kind != TOK_COMMA)
        {
            zpanic_at(after, "Expected ';' after the value of '%s'", node->var_decl.name);
            continue;
        }
        node->var_decl.init_expr = init;
        if (!node->var_decl.type_info)
        {
            // Without a type written, the global has its value's.
            node->var_decl.type_info = type_obj;
            node->type_info = type_obj;
            if (p->symbol)
            {
                p->symbol->type_info = type_obj;
            }
        }
        check_var_value(ctx, node->var_decl.name, type_obj, init);
        check_global_value(init);
    }
    ctx->current_scope = saved_scope;
    ctx->imports.current_module_prefix = saved_prefix;
    ctx->current_filename = saved_filename;
}

ASTNode *parse_var_decl(ParserContext *ctx, Lexer *l, int is_export)
{
    Token tk = lexer_next(l); // eat 'let'

    // Destructuring: let {x, y} = ... OR let (a: type, b: type) = ...
    if (lexer_peek(l).kind == TOK_LBRACE || lexer_peek(l).kind == TOK_LPAREN)
    {
        int is_struct = (lexer_peek(l).kind == TOK_LBRACE);
        lexer_next(l);
        zvec_Text names = {0};
        zvec_Type type_infos = {0};
        zvec_Token tokens = {0};
        while (1)
        {
            Token t = lexer_next(l);
            check_identifier(t);
            char *nm = token_strdup(t);
            zvec_push_Token(&tokens, t);

            // Check for optional type annotation: name: type
            Type *type_obj = NULL;
            if (!is_struct && lexer_peek(l).kind == TOK_COLON)
            {
                lexer_next(l); // eat :
                type_obj = parse_type_formal(ctx, l);
            }
            zvec_push_Text(&names, nm);
            zvec_push_Type(&type_infos, type_obj);
            add_symbol(ctx, nm, type_obj, is_export);

            Token next = lexer_next(l);
            if (next.kind == TOK_EOF)
            {
                zpanic_at(next, "Unexpected end of file in destructuring pattern");
                break;
            }
            if (next.kind == (is_struct ? TOK_RBRACE : TOK_RPAREN))
            {
                break;
            }
            if (next.kind != TOK_COMMA)
            {
                zpanic_at(next, "Expected comma in destructuring list");
                return NULL;
            }
        }
        if (lexer_next(l).kind != TOK_OP)
        {
            zpanic_at(lexer_peek(l), "Expected =");
            return NULL;
        }
        ASTNode *init = parse_expression(ctx, l);
        if (lexer_peek(l).kind == TOK_SEMICOLON)
        {
            lexer_next(l);
        }
        ASTNode *n = ast_create(NODE_DESTRUCT_VAR);
        n->token = tk;
        n->destruct.names = names.data;
        n->destruct.type_infos = type_infos.data;
        n->destruct.count = (int)names.length;
        n->destruct.init_expr = init;
        n->destruct.is_struct_destruct = is_struct;
        if (is_struct)
        {
            // `let {x, y}`: each name is the field's.
            n->destruct.field_tokens = tokens.data;
            n->destruct.name_tokens = tokens.data;
        }
        return n;
    }

    // Normal Declaration OR Named Struct Destructuring
    Token name_tok = lexer_next(l);
    check_identifier(name_tok);
    char *name = token_strdup(name_tok);

    // Check for Struct Destructuring: var Point { x, y }
    if (lexer_peek(l).kind == TOK_LBRACE)
    {
        lexer_next(l);
        zvec_Text names = {0};
        zvec_Text fields = {0};
        zvec_Token field_tokens = {0};
        zvec_Token name_tokens = {0};

        while (1)
        {
            // Parse field:name or just name
            Token t = lexer_next(l);
            check_identifier(t);
            char *field = token_strdup(t);
            char *variable = field; // Shorthand: field (implies var name = field)
            Token variable_token = t;

            if (lexer_peek(l).kind == TOK_COLON)
            {
                // field: var_name
                lexer_next(l); // eat :
                Token v = lexer_next(l);
                check_identifier(v);
                variable = token_strdup(v);
                variable_token = v;
            }
            zvec_push_Text(&fields, field);
            zvec_push_Text(&names, variable);
            zvec_push_Token(&field_tokens, t);
            zvec_push_Token(&name_tokens, variable_token);
            // Register symbol for variable
            add_symbol(ctx, variable, NULL, is_export);

            Token next = lexer_next(l);
            if (next.kind == TOK_EOF)
            {
                zpanic_at(next, "Unexpected end of file in struct destructuring");
                break;
            }
            if (next.kind == TOK_RBRACE)
            {
                break;
            }
            if (next.kind != TOK_COMMA)
            {
                zpanic_at(next, "Expected comma in struct pattern");
                return NULL;
            }
        }

        if (lexer_next(l).kind != TOK_OP)
        {
            zpanic_at(lexer_peek(l), "Expected =");
            return NULL;
        }
        ASTNode *init = parse_expression(ctx, l);
        if (lexer_peek(l).kind == TOK_SEMICOLON)
        {
            lexer_next(l);
        }

        ASTNode *n = ast_create(NODE_DESTRUCT_VAR);
        n->token = name_tok;
        n->destruct.names = names.data;
        n->destruct.field_names = fields.data;
        n->destruct.field_tokens = field_tokens.data;
        n->destruct.name_tokens = name_tokens.data;
        n->destruct.count = (int)names.length;
        n->destruct.init_expr = init;
        n->destruct.is_struct_destruct = 1;
        n->destruct.struct_name = name; // "Point"
        return n;
    }

    // Check for Guard Pattern: var Some(val) = opt else { ... }
    if (lexer_peek(l).kind == TOK_LPAREN)
    {
        lexer_next(l);
        Token val_tok = lexer_next(l);
        check_identifier(val_tok);
        char *val_name = token_strdup(val_tok);

        if (lexer_next(l).kind != TOK_RPAREN)
        {
            zpanic_at(lexer_peek(l), "Expected ')' in guard pattern");
            return NULL;
        }

        if (lexer_next(l).kind != TOK_OP)
        {
            zpanic_at(lexer_peek(l), "Expected '=' after guard pattern");
            return NULL;
        }

        ASTNode *init = parse_expression(ctx, l);

        Token t = lexer_next(l);
        if (t.kind != TOK_IDENT || strncmp(t.start, "else", 4) != 0)
        {
            zpanic_at(t, "Expected 'else' in guard statement");
            return NULL;
        }

        ASTNode *else_blk;
        if (lexer_peek(l).kind == TOK_LBRACE)
        {
            else_blk = parse_block(ctx, l);
        }
        else
        {
            else_blk = ast_create(NODE_BLOCK);
            else_blk->block.statements = parse_statement(ctx, l);
        }

        if (lexer_peek(l).kind == TOK_SEMICOLON)
        {
            lexer_next(l);
        }

        ASTNode *n = ast_create(NODE_DESTRUCT_VAR);
        n->token = t;
        n->destruct.names = xmalloc(sizeof(char *));
        n->destruct.names[0] = val_name;
        n->destruct.count = 1;
        n->destruct.init_expr = init;
        n->destruct.is_guard = 1;
        n->destruct.guard_variant = name;
        n->destruct.else_block = else_blk;

        add_symbol(ctx, val_name, NULL, is_export);

        return n;
    }

    char *type = NULL;
    Type *type_obj = NULL; // --- NEW: Formal Type Object ---

    if (lexer_peek(l).kind == TOK_COLON)
    {
        lexer_next(l);
        // Hybrid Parse: Get Object AND String
        type_obj = parse_type_formal(ctx, l);
        if (!type_obj)
        {
            return NULL;
        }
        type = type_mangle_spelling(type_obj);
    }

    ASTNode *init = NULL;
    // A global's value is read once every declaration is: it may name one declared further down
    // (parse_pending_global_values).
    int is_global = !ctx->current_scope || !ctx->current_scope->parent;
    int value_waits = 0;
    Lexer value_start = *l;
    if (lexer_peek(l).kind == TOK_OP && is_token(lexer_peek(l), "="))
    {
        lexer_next(l);
        if (is_global && !ctx->declarations_read && strcmp(name, "_") != 0)
        {
            value_start = *l;
            value_waits = 1;
            if (!skip_expression(l))
            {
                zpanic_at(lexer_peek(l), "Expected ';' after the value of '%s'", name);
                return NULL;
            }
        }
        else
        {
            init = read_var_value(ctx, l, &type, &type_obj);
            if (!init)
            {
                return NULL;
            }
        }
    }

    if (!type && !init && !value_waits)
    {
        zpanic_at(name_tok, "Variable '%s' requires a type or initializer", name);
        return NULL;
    }

    // Register in symbol table with actual token. `let _ = <value>` declares nothing.
    if (strcmp(name, "_") != 0)
    {
        add_symbol_with_token(ctx, name, type_obj, name_tok, is_export);
    }

    if (!check_var_value(ctx, name, type_obj, init))
    {
        return NULL;
    }

    int ended = 0;
    if (lexer_peek(l).kind == TOK_SEMICOLON)
    {
        lexer_next(l);
        ended = 1;
    }

    ASTNode *n = ast_create(NODE_VAR_DECL);
    n->token = name_tok; // Save location
    n->var_decl.name = name;
    n->var_decl.type_info = type_obj;
    n->type_info = type_obj;

    n->var_decl.init_expr = init;

    if (is_global)
    {
        collect_declaration(ctx, n);
    }
    if (value_waits)
    {
        wait_for_global_value(ctx, n, type, value_start);
    }

    // Check for 'defer' (Value-Returning Defer): `let x = v defer f(it);`.
    // Only capture if it is NOT a block defer (defer { ... }), and not after a `;`:
    // `let x = v; defer f();` is a defer statement of its own, left for the next
    // parse_statement call like a block defer.
    if (!ended && lexer_peek(l).kind == TOK_DEFER)
    {
        Lexer lookahead = *l;
        lexer_next(&lookahead); // Eat defer
        if (lexer_peek(&lookahead).kind != TOK_LBRACE)
        {
            // Proceed to consume
            lexer_next(l); // Eat defer
            // Parse the defer expression/statement
            // Usually defer close(it);
            // We parse expression.
            ASTNode *expr = parse_expression(ctx, l);

            // Handle "it" substitution
            replace_it_with_var(expr, name);

            if (lexer_peek(l).kind == TOK_SEMICOLON)
            {
                lexer_next(l);
            }

            ASTNode *d = ast_create(NODE_DEFER);
            d->token = tk;
            d->defer_stmt.stmt = expr;

            // Chain it: var_decl -> defer
            n->next = d;
        }
    }

    // Multiple declarations: let a = 0, b = 1
    if (lexer_peek(l).kind == TOK_COMMA)
    {
        ASTNode *head = NULL;
        ASTNode *prev = NULL;
        // The current n is the first declaration; include it in the chain
        head = n;
        prev = n;

        while (lexer_peek(l).kind == TOK_COMMA)
        {
            lexer_next(l); // eat comma

            // Parse next variable: name [:type] [= expr]
            Token next_name_tok = lexer_next(l);
            check_identifier(next_name_tok);
            char *next_name = token_strdup(next_name_tok);

            char *next_type = NULL;
            Type *next_type_obj = NULL;
            if (lexer_peek(l).kind == TOK_COLON)
            {
                lexer_next(l);
                next_type_obj = parse_type_formal(ctx, l);
                if (!next_type_obj)
                {
                    return NULL;
                }
                next_type = type_mangle_spelling(next_type_obj);
            }

            ASTNode *next_init = NULL;
            int next_waits = 0;
            Lexer next_start = *l;
            if (lexer_peek(l).kind == TOK_OP && is_token(lexer_peek(l), "="))
            {
                lexer_next(l);
                if (is_global && !ctx->declarations_read && strcmp(next_name, "_") != 0)
                {
                    next_start = *l;
                    next_waits = 1;
                    if (!skip_expression(l))
                    {
                        zpanic_at(lexer_peek(l), "Expected ';' after the value of '%s'", next_name);
                        return NULL;
                    }
                }
                else
                {
                    next_init = read_var_value(ctx, l, &next_type, &next_type_obj);
                    if (!next_init)
                    {
                        return NULL;
                    }
                }
            }

            if (!next_type && !next_init && !next_waits)
            {
                zpanic_at(next_name_tok, "Variable '%s' requires a type or initializer", next_name);
                return NULL;
            }

            add_symbol_with_token(ctx, next_name, next_type_obj, next_name_tok, is_export);
            if (!check_var_value(ctx, next_name, next_type_obj, next_init))
            {
                return NULL;
            }

            ASTNode *next_node = ast_create(NODE_VAR_DECL);
            next_node->token = next_name_tok;
            next_node->var_decl.name = next_name;
            next_node->var_decl.type_info = next_type_obj;
            next_node->type_info = next_type_obj;
            next_node->var_decl.init_expr = next_init;

            if (is_global)
            {
                collect_declaration(ctx, next_node);
            }
            if (next_waits)
            {
                wait_for_global_value(ctx, next_node, next_type, next_start);
            }

            prev->next = next_node;
            prev = next_node;
        }

        if (lexer_peek(l).kind == TOK_SEMICOLON)
        {
            lexer_next(l);
        }

        return head;
    }

    return n;
}
