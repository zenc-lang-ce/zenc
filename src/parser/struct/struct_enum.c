// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "ast/ast.h"
#include "analysis/move_check.h"
#include "plugins/plugin_manager.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analysis/const_fold.h"
#include "utils/utils.h"
#include "ast/primitives.h"

ASTNode *parse_enum(ParserContext *ctx, Lexer *l, const char *link_name, int is_export)
{
    lexer_next(l);
    // `register_generic` skips a name that is already a generic parameter: what the enum
    // registers is undone by restoring the count, not by subtracting what it declared.
    int generics_before = ctx->known_generics_count;
    Token n = lexer_next(l);
    check_type_name(n);

    char *gp = NULL;
    if (lexer_peek(l).kind == TOK_LANGLE)
    {
        lexer_next(l); // eat <
        Token g = lexer_next(l);
        check_identifier(g);
        gp = token_strdup(g);
        lexer_next(l); // eat >
        register_generic(ctx, g);
    }

    lexer_next(l);

    ASTNode *h = 0, *tl = 0;
    int v = 0;
    // The variant whose value waits for a constant, which the variants after it count on from.
    PendingConstant *counts_from = NULL;
    char *ename = token_strdup(n); // Store enum name

    // In a module parsed with a prefix (imported with an alias, one by one or as `*`), the enum
    // is named with the prefix. The variants and their constructors are registered below under
    // that name, the one the type and its uses carry.
    // A generic template keeps its name: each instance gets its own.
    // `ename` is the C name from here on; the node keeps the one the module writes.
    char *own_name = ename;
    char *written_name = NULL;
    char *prefixed_name =
        !gp ? module_prefixed_name(ctx, ctx->imports.current_module_prefix, ename) : NULL;
    if (prefixed_name)
    {
        written_name = module_written_name(ctx, ename);
        ename = prefixed_name;
    }

    while (1)
    {
        skip_comments(l);
        Token t = lexer_peek(l);
        if (t.kind == TOK_RBRACE)
        {
            lexer_next(l);
            break;
        }
        if (t.kind == TOK_COMMA)
        {
            lexer_next(l);
            continue;
        }
        if (t.kind == TOK_EOF)
        {
            zpanic_at(t, "Unexpected end of file in enum body");
            break;
        }

        if (t.kind == TOK_IDENT)
        {
            Token vt = lexer_next(l);
            check_reserved_keyword(vt);
            char *vname = token_strdup(vt);

            Type *payload = NULL;
            Type **tuple_types = NULL;
            int tuple_count = 0;
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Type *first_t = parse_type_obj(ctx, l);

                if (lexer_peek(l).kind == TOK_COMMA)
                {
                    // Several values: the payload is a tuple of them.
                    int tuple_cap = 4;
                    tuple_types = xmalloc(sizeof(Type *) * (size_t)tuple_cap);
                    tuple_types[tuple_count++] = first_t;
                    while (lexer_peek(l).kind == TOK_COMMA)
                    {
                        lexer_next(l); // eat ,
                        if (tuple_count >= tuple_cap)
                        {
                            tuple_cap *= 2;
                            tuple_types =
                                xrealloc(tuple_types, sizeof(Type *) * (size_t)(tuple_cap));
                        }
                        tuple_types[tuple_count++] = parse_type_obj(ctx, l);
                    }
                    payload = type_new_tuple(tuple_types, tuple_count);
                    register_tuple(ctx, payload);
                }
                else
                {
                    payload = first_t;
                }

                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected )");
                    return NULL;
                }
            }

            // Explicit value: `Ok = 200`.
            PendingConstant *waiting_value = NULL;
            if (lexer_peek(l).kind == TOK_OP && is_token(lexer_peek(l), "="))
            {
                lexer_next(l);
                Token value_tok = lexer_peek(l);
                Lexer value_start = *l;
                ASTNode *value_expr = parse_constant_expression(ctx, l);
                long long value = 0;
                counts_from = NULL;
                if (value_expr && eval_const_int_expr(value_expr, ctx, &value))
                {
                    if (value < INT_MIN || value > INT_MAX)
                    {
                        zpanic_at(value_tok, "Enum variant value %lld does not fit in an 'int'",
                                  value);
                        return NULL;
                    }
                    v = (int)value;
                }
                else if (value_expr && !ctx->declarations_read)
                {
                    // It names a constant without a value yet (a `def`, whose value is read
                    // once every declaration is): the variant takes its value then.
                    waiting_value =
                        wait_for_constant(ctx, value_expr, NULL, NULL, value_tok, &value_start);
                }
                else
                {
                    zpanic_at(value_tok, "Enum variant value must be a constant integer");
                    return NULL;
                }
            }

            ASTNode *va = ast_create(NODE_ENUM_VARIANT);
            ATTACH_DOC_COMMENT(ctx, va);
            va->token = vt;
            va->variant.name = vname;
            va->variant.tag_id = v++;      // Use tag_id instead of value
            va->variant.payload = payload; // Store Type*
            va->variant.payload_count = tuple_count ? tuple_count : (payload ? 1 : 0);

            // Register Variant (Mangled name to avoid collisions: Result_Ok). The
            // constructor is named after the enum; with @link_name, C calls it by that name.
            char *mangled = mangle_variant_symbol(ename, vname);
            char *c_mangled = link_name ? mangle_variant_symbol(link_name, vname) : mangled;
            register_enum_variant(ctx, vname, ename, va->variant.tag_id);
            // A variant whose value waits, and the ones that count on from it, have no value
            // yet (resolve_pending_constants gives it).
            if (waiting_value)
            {
                waiting_value->variant = va;
                waiting_value->enum_name = ename;
                counts_from = waiting_value;
            }
            else if (counts_from)
            {
                counts_from->followers++;
            }
            if (counts_from)
            {
                EnumVariantReg *reg = find_enum_variant_of(ctx, ename, vname);
                if (reg)
                {
                    reg->tag_pending = 1;
                }
            }

            // Register Constructor Function Signature
            if (payload && !gp) // Only for non-generic enums for now
            {
                if (payload->kind == TYPE_TUPLE)
                {
                    Type *ret_t = type_new_enum(ename);

                    // We can reuse the tuple_types collected during parsing!
                    register_func(ctx, ctx->current_scope, mangled, tuple_count, NULL, tuple_types,
                                  ret_t, 0, c_mangled, va->variant.name, vt, is_export);
                }
                else
                {
                    Type **at = xmalloc(sizeof(Type *));
                    at[0] = payload;
                    Type *ret_t = type_new_enum(ename);
                    register_func(ctx, ctx->current_scope, mangled, 1, NULL, at, ret_t, 0,
                                  c_mangled, va->variant.name, vt, is_export);
                }
            }
            else if (!gp)
            {
                // No payload: don't register as function.
                // Codegen handles calling the constructor via codegen_var_expr.
            }
            zfree(mangled);

            if (!h)
            {
                h = va;
            }
            else
            {
                tl->next = va;
            }
            tl = va;
        }
        else
        {
            lexer_next(l);
        }
    }

    ASTNode *node = ast_create(NODE_ENUM);
    node->token = n;
    node->enm.name = own_name;
    node->enm.module_prefix = ctx->imports.current_module_prefix;
    node->enm.is_module_prefixed = prefixed_name != NULL;
    node->enm.written_name = written_name;
    node->link_name = definition_link_name(ctx, link_name, ename);
    node->type_info = type_new_enum(ename);

    node->enm.variants = h;
    if (gp)
    {
        node->enm.generic_params = xmalloc(sizeof(char *));
        node->enm.generic_params[0] = gp;
        node->enm.generic_param_count = 1;
    }
    node->enm.is_export = is_export;

    if (gp)
    {
        node->enm.is_template = 1;
        ctx->known_generics_count = generics_before;
    }

    collect_declaration(ctx, node);
    return node;
}
