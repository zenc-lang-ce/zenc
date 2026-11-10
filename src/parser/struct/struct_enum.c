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
    Token n = lexer_next(l);
    check_identifier(n);

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
    char *ename = token_strdup(n); // Store enum name

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
                // A variant with a payload names a field of the generated union, so it
                // cannot be a C keyword; a payload-less one is only emitted as `Enum__name`.
                check_c_identifier(vt);
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

            // Explicit value: `Ok = 200`. It must be known before the variant is
            // registered, since match and constant folding read the registered tag.
            if (lexer_peek(l).kind == TOK_OP && is_token(lexer_peek(l), "="))
            {
                lexer_next(l);
                Token value_tok = lexer_peek(l);
                ASTNode *value_expr = parse_constant_expression(ctx, l);
                long long value = 0;
                if (!value_expr || !eval_const_int_expr(value_expr, ctx, &value))
                {
                    zpanic_at(value_tok, "Enum variant value must be a constant integer");
                    return NULL;
                }
                if (value < INT_MIN || value > INT_MAX)
                {
                    zpanic_at(value_tok, "Enum variant value %lld does not fit in an 'int'", value);
                    return NULL;
                }
                v = (int)value;
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

            // Register Constructor Function Signature
            if (payload && !gp) // Only for non-generic enums for now
            {
                if (payload->kind == TYPE_TUPLE)
                {
                    Type *ret_t = type_new_enum(ename);

                    // We can reuse the tuple_types collected during parsing!
                    register_func(ctx, ctx->current_scope, mangled, tuple_count, NULL, tuple_types,
                                  ret_t, 0, 0, c_mangled, va->variant.name, vt, is_export);
                }
                else
                {
                    Type **at = xmalloc(sizeof(Type *));
                    at[0] = payload;
                    Type *ret_t = type_new_enum(ename);
                    register_func(ctx, ctx->current_scope, mangled, 1, NULL, at, ret_t, 0, 0,
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

    // Auto-prefix enum name if in module context
    char *written_name = NULL;
    if (ctx->imports.current_module_prefix && !gp)
    { // Don't prefix generic templates
        written_name = module_written_name(ctx, ename);
        char *prefixed_name = mangle_module_symbol(ctx->imports.current_module_prefix, ename);
        zfree(ename);
        ename = prefixed_name;
    }

    ASTNode *node = ast_create(NODE_ENUM);
    node->token = n;
    node->enm.name = ename;
    node->enm.written_name = written_name;
    node->link_name = definition_link_name(ctx, link_name, ename);
    node->type_info = type_new_enum(ename);

    node->enm.variants = h;
    node->enm.generic_param = gp; // Store generic param
    node->enm.is_export = is_export;

    if (gp)
    {
        node->enm.is_template = 1;
        ctx->known_generics_count--;
        register_template(ctx, node->enm.name, node);
    }

    register_struct_def(ctx, node->enm.name, node);

    add_to_enum_list(ctx, node); // Register globally

    return node;
}
