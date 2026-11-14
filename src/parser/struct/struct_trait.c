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

ASTNode *parse_trait(ParserContext *ctx, Lexer *l)
{
    lexer_next(l); // eat trait
    Token n = lexer_next(l);
    check_identifier(n);
    if (n.kind != TOK_IDENT)
    {
        zpanic_at(n, "Expected trait name");
        return NULL;
    }
    char *name = xmalloc(n.len + 1);
    strncpy(name, n.start, n.len);
    name[n.len] = 0;

    // Generics <T>. `register_generic` skips a name that is already a generic parameter: what
    // the trait registers is undone by restoring the count, not by subtracting what it declared.
    int generics_before = ctx->known_generics_count;
    zvec_Text generic_params = {0};
    if (lexer_peek(l).kind == TOK_LANGLE)
    {
        lexer_next(l); // eat <
        while (1)
        {
            Token p = lexer_next(l);
            check_identifier(p);
            if (p.kind != TOK_IDENT)
            {
                zpanic_at(p, "Expected generic parameter name");
                return NULL;
            }
            zvec_push_Text(&generic_params, token_strdup(p));
            register_generic(ctx, p);

            Token sep = lexer_peek(l);
            if (sep.kind == TOK_EOF)
            {
                zpanic_at(sep, "Expected '>' in generic parameter list");
                break;
            }
            if (sep.kind == TOK_COMMA)
            {
                lexer_next(l);
                continue;
            }
            else if (sep.kind == TOK_RANGLE)
            {
                lexer_next(l);
                break;
            }
            else
            {
                zpanic_at(sep, "Expected , or > in generic params");
                return NULL;
            }
        }
    }

    lexer_next(l);

    ASTNode *methods = NULL, *tail = NULL;
    while (1)
    {
        skip_comments(l);
        if (lexer_peek(l).kind == TOK_RBRACE)
        {
            lexer_next(l);
            break;
        }
        if (lexer_peek(l).kind == TOK_EOF)
        {
            zpanic_at(lexer_peek(l), "Unexpected end of file in trait body");
            break;
        }

        DeclarationAttributes attrs = {0};
        if (lexer_peek(l).kind == TOK_AT)
        {
            attrs = parse_attributes(ctx, l);
        }

        // Parse method signature: fn name(args...) -> ret;
        Token ft = lexer_next(l);
        if (ft.kind != TOK_IDENT || strncmp(ft.start, "fn", 2) != 0)
        {
            zpanic_at(ft, "Expected fn in trait");
            return NULL;
        }

        Token mn = lexer_next(l);
        check_identifier(mn);
        char *mname = xmalloc(mn.len + 1);
        strncpy(mname, mn.start, mn.len);
        mname[mn.len] = 0;

        int count = 0;
        Type **arg_types = NULL;
        ASTNode **default_values = NULL;
        char **param_names = NULL;
        int is_varargs = 0;
        // The parameters belong to the signature, as a function's: not to the enclosing scope.
        enter_scope(ctx);
        parse_and_convert_args(ctx, l, &default_values, &count, &arg_types, &param_names, NULL,
                               &is_varargs, NULL);
        exit_scope(ctx);

        Type *ret_type_obj = type_new(TYPE_VOID);
        if (lexer_peek(l).kind == TOK_ARROW)
        {
            lexer_next(l);
            ret_type_obj = parse_type_formal(ctx, l);
            if (!ret_type_obj)
            {
                return NULL;
            }
        }

        if (lexer_peek(l).kind == TOK_SEMICOLON)
        {
            lexer_next(l);
            ASTNode *m = ast_create(NODE_FUNCTION);
            ATTACH_DOC_COMMENT(ctx, m);
            m->token = ft;
            m->func.param_names = param_names;
            m->func.name = mname;
            m->func.method_name = mname;
            m->func.default_values = default_values;
            m->func.count = count;
            m->func.arg_types = arg_types;
            m->func.ret_type_info = ret_type_obj;
            m->func.body = NULL;
            m->link_name = attrs.link_name;
            m->cfg_condition = attrs.cfg_condition;
            m->func.pure = attrs.is_pure;
            if (!methods)
            {
                methods = m;
            }
            else
            {
                tail->next = m;
            }
            tail = m;
        }
        else
        {
            // Default implementation? Not supported yet.
            zpanic_at(lexer_peek(l), "Trait methods must end with ; for now");
            return NULL;
        }
    }

    ASTNode *n_node = ast_create(NODE_TRAIT);
    n_node->token = n;
    n_node->trait.name = name;
    n_node->trait.methods = methods;
    n_node->trait.generic_params = generic_params.data;
    n_node->trait.generic_param_count = (int)generic_params.length;

    ctx->known_generics_count = generics_before;

    register_trait(name);
    add_to_global_list(ctx, n_node); // Track for codegen (VTable emission)
    return n_node;
}
