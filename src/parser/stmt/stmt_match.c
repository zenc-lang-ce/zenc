// SPDX-License-Identifier: MIT

#include "parser.h"
#include "constants.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ast/ast.h"
#include "utils/format_expr.h"
#include "plugins/plugin_manager.h"
#include "zen/zen_facts.h"
#include "zprep_plugin.h"
#include "analysis/move_check.h"

char *unmangle_ptr_suffix(const char *s);

ASTNode *parse_match(ParserContext *ctx, Lexer *l)
{
    init_builtins();
    Token start_token = lexer_peek(l);
    lexer_next(l); // eat 'match'
    return parse_match_after_keyword(ctx, l, start_token);
}

ASTNode *parse_match_after_keyword(ParserContext *ctx, Lexer *l, Token start_token)
{
    ASTNode *expr = parse_expression(ctx, l);

    Token t_brace = lexer_next(l);
    if (t_brace.kind != TOK_LBRACE)
    {
        zpanic_at(t_brace, "Expected '{' in match");
        if (ctx->is_fault_tolerant)
        {
            ASTNode *node = ast_create(NODE_MATCH);
            node->token = start_token;
            node->match_stmt.expr = expr;
            node->match_stmt.cases = NULL;
            return node;
        }
        return NULL;
    }

    ASTNode *h = 0, *tl = 0;
    while (1)
    {
        skip_comments(l);
        if (lexer_peek(l).kind == TOK_RBRACE)
        {
            break;
        }
        if (lexer_peek(l).kind == TOK_EOF)
        {
            zpanic_at(lexer_peek(l), "Unexpected end of file in match body");
            break;
        }
        if (lexer_peek(l).kind == TOK_COMMA)
        {
            lexer_next(l);
        }
        skip_comments(l);
        if (lexer_peek(l).kind == TOK_RBRACE)
        {
            break;
        }
        if (lexer_peek(l).kind == TOK_EOF)
        {
            zpanic_at(lexer_peek(l), "Unexpected end of file in match body");
            break;
        }

        MatchPattern *patterns = NULL;
        MatchPattern *last_pattern = NULL;

        while (1)
        {
            Token p = lexer_next(l);
            char *written_scope = NULL;
            char *written_name = token_strdup(p);

            while (1)
            {
                skip_comments(l);
                Token pk = lexer_peek(l);
                if (pk.kind == TOK_DCOLON)
                {
                    lexer_next(l); // eat ::
                    Token suffix = lexer_next(l);
                    if (written_scope)
                    {
                        size_t scope_size = strlen(written_scope) + strlen(written_name) + 3;
                        char *joined = xmalloc(scope_size);
                        snprintf(joined, scope_size, "%s::%s", written_scope, written_name);
                        written_scope = joined;
                    }
                    else
                    {
                        written_scope = written_name;
                    }
                    written_name = token_strdup(suffix);
                }
                else if (pk.kind == TOK_LANGLE)
                {
                    lexer_next(l); // eat <
                    int depth = 1;
                    while (depth > 0)
                    {
                        Token t = lexer_next(l);
                        if (t.kind == TOK_LANGLE)
                        {
                            depth++;
                        }
                        else if (t.kind == TOK_RANGLE)
                        {
                            depth--;
                        }
                        else if (t.kind == TOK_EOF)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    break;
                }
            }

            char *range_start = NULL;
            char *range_end = NULL;
            int range_is_inclusive = 0;
            if (lexer_peek(l).kind == TOK_DOTDOT || lexer_peek(l).kind == TOK_DOTDOT_EQ ||
                lexer_peek(l).kind == TOK_DOTDOT_LT)
            {
                range_is_inclusive = (lexer_peek(l).kind == TOK_DOTDOT_EQ);
                lexer_next(l); // eat operator
                Token end_tok = lexer_next(l);
                range_start = written_scope ? mangle_module_symbol(written_scope, written_name)
                                            : written_name;
                range_end = token_strdup(end_tok);
            }

            MatchPattern *alternative = xcalloc(1, sizeof(MatchPattern));
            if (range_end)
            {
                alternative->kind = PATTERN_RANGE;
                alternative->range_start = range_start;
                alternative->range_end = range_end;
                alternative->is_inclusive = range_is_inclusive;
            }
            else if (p.kind == TOK_IDENT && !written_scope && strcmp(written_name, "_") == 0)
            {
                alternative->kind = PATTERN_WILDCARD;
            }
            else if (p.kind == TOK_IDENT)
            {
                alternative->kind = PATTERN_NAME;
                alternative->name.scope = written_scope;
                alternative->name.name = written_name;
            }
            else
            {
                alternative->kind = PATTERN_LITERAL;
                alternative->literal = token_strdup(p);
            }
            if (last_pattern)
            {
                last_pattern->next = alternative;
            }
            else
            {
                patterns = alternative;
            }
            last_pattern = alternative;

            Token next = lexer_peek(l);
            skip_comments(l);
            int is_or = (next.kind == TOK_OR) ||
                        (next.kind == TOK_OP && next.len == 2 && next.start[0] == '|' &&
                         next.start[1] == '|') ||
                        (next.kind == TOK_COMMA);
            if (is_or)
            {
                lexer_next(l); // eat ||, 'or', or comma
                skip_comments(l);
                continue;
            }
            else
            {
                break;
            }
        }

        // The variant the case names, when it is a single name (`Variant(x)`, `Enum::Variant`).
        QualifiedName *named =
            (patterns->kind == PATTERN_NAME && !patterns->next) ? &patterns->name : NULL;
        int is_default = (patterns->kind == PATTERN_WILDCARD && !patterns->next);
        int is_destructure = 0;

        char **bindings = NULL;
        int *binding_refs = NULL;
        int binding_count = 0;

        skip_comments(l);
        if (lexer_peek(l).kind == TOK_LBRACE)
        {
            const char *hints[] = {"match on a field instead: `match value.field { ... }`", NULL};
            zpanic_with_hints(lexer_peek(l), "Struct patterns are not supported in match", hints);
            return NULL;
        }
        if (!is_default && !patterns->next && lexer_peek(l).kind == TOK_LPAREN)
        {
            lexer_next(l);

            bindings = xcalloc(8, sizeof(char *));
            binding_refs = xcalloc(8, sizeof(int));
            int binding_cap = 8;

            while (lexer_peek(l).kind != TOK_RPAREN)
            {
                int is_r = 0;
                if (lexer_peek(l).kind == TOK_IDENT && lexer_peek(l).len == 3 &&
                    strncmp(lexer_peek(l).start, "ref", 3) == 0)
                {
                    lexer_next(l); // eat 'ref'
                    is_r = 1;
                }

                Token b = lexer_next(l);
                if (b.kind != TOK_IDENT)
                {
                    zpanic_at(b, "Expected variable name in pattern");
                    break;
                }

                if (binding_count >= binding_cap)
                {
                    binding_cap *= 2;
                    bindings = xrealloc(bindings, sizeof(char *) * (size_t)(binding_cap));
                    binding_refs = xrealloc(binding_refs, sizeof(int) * (size_t)(binding_cap));
                }

                bindings[binding_count] = token_strdup(b);
                binding_refs[binding_count] = is_r;
                binding_count++;

                if (lexer_peek(l).kind == TOK_COMMA)
                {
                    lexer_next(l);
                    continue;
                }
                break;
            }

            Token end = lexer_next(l);
            if (end.kind != TOK_RPAREN)
            {
                zpanic_at(end, "Expected )");
                return NULL;
            }
            is_destructure = 1;
        }

        ASTNode *guard = NULL;
        if (lexer_peek(l).kind == TOK_IDENT && strncmp(lexer_peek(l).start, "if", 2) == 0)
        {
            lexer_next(l);
            guard = parse_expression(ctx, l);
        }

        Token arrow = lexer_next(l);
        if (arrow.kind == TOK_ARROW)
        {
            const char *hints[] = {"write the arm as `pattern => value`", NULL};
            zpanic_with_hints(arrow, "Match arms use '=>', not '->'", hints);
            return NULL;
        }
        if (arrow.kind != TOK_FAT_ARROW)
        {
            zpanic_at(lexer_peek(l), "Expected => after match pattern");
            return NULL;
            if (ctx->is_fault_tolerant)
            {
                while (lexer_peek(l).kind != TOK_RBRACE && lexer_peek(l).kind != TOK_COMMA &&
                       lexer_peek(l).kind != TOK_EOF)
                {
                    lexer_next(l);
                }
                if (lexer_peek(l).kind == TOK_COMMA)
                {
                    lexer_next(l);
                }
                continue;
            }
        }

        enter_scope(ctx);
        if (binding_count > 0)
        {
            EnumVariantReg *vreg = NULL;
            if (named)
            {
                vreg = named->scope ? find_enum_variant_of(ctx, named->scope, named->name)
                                    : find_enum_variant(ctx, named->name);
            }

            int is_tuple_payload = 0;
            Type *payload_type = NULL;
            ASTNode *enum_def = NULL;

            if (vreg)
            {
                enum_def = find_struct_def(ctx, vreg->enum_name);
                if (enum_def && enum_def->kind == NODE_ENUM)
                {
                    ASTNode *v = enum_def->enm.variants;
                    while (v)
                    {
                        if (strcmp(v->variant.name, named->name) == 0 && v->variant.payload)
                        {
                            payload_type = v->variant.payload;
                            is_tuple_payload = payload_type->kind == TYPE_TUPLE;
                            break;
                        }
                        v = v->next;
                    }
                }
            }
            else if (binding_count == 1 && named && expr && expr->type_info)
            {
                // Struct constructor pattern, e.g. `Ok(val)` matching a
                // `Result<String>`. The payload type is the constructor's
                // argument type with the struct's generic parameter substituted
                // by the instance's argument.
                Type *mt = expr->type_info;
                GenericTemplate *gt = (mt->count > 0 && mt->args && mt->name)
                                          ? find_generic_template(ctx, mt->name)
                                          : NULL;
                if (gt && gt->struct_node && gt->struct_node->kind == NODE_STRUCT &&
                    gt->struct_node->strct.generic_param_count > 0)
                {
                    char *ctor = mangle_method_symbol(named->scope ? named->scope : gt->name, NULL,
                                                      named->name);
                    GenericFuncTemplate *ft = find_func_template(ctx, ctor);
                    zfree(ctor);
                    if (ft && ft->func_node && ft->func_node->kind == NODE_FUNCTION &&
                        ft->func_node->func.count >= 1 && ft->func_node->func.arg_types[0])
                    {
                        // The constructor's parameter, with the struct's generic parameter
                        // replaced by the instance's argument (`T` or `T*`); or a concrete type
                        // (`char*` for Err).
                        const char *gparam = gt->struct_node->strct.generic_params[0];
                        Type *param = ft->func_node->func.arg_types[0];
                        Type *named_type = param->kind == TYPE_POINTER ? param->inner : param;
                        int is_param =
                            named_type && named_type->name &&
                            (named_type->kind == TYPE_STRUCT || named_type->kind == TYPE_GENERIC) &&
                            strcmp(named_type->name, gparam) == 0;
                        if (!is_param)
                        {
                            payload_type = param;
                        }
                        else if (param->kind == TYPE_POINTER)
                        {
                            payload_type = type_new_ptr(mt->args[0]);
                        }
                        else
                        {
                            payload_type = mt->args[0];
                        }
                    }
                }
            }

            for (int i = 0; i < binding_count; i++)
            {
                char *binding = bindings[i];
                if (!binding)
                {
                    continue;
                }
                int is_ref = binding_refs[i];
                char *binding_type = is_ref ? "void*" : "unknown";
                Type *binding_type_info = NULL;

                if (payload_type)
                {
                    if (binding_count == 1 && !is_tuple_payload)
                    {
                        binding_type = type_to_string(payload_type);
                        binding_type_info = payload_type;
                    }
                    else if (binding_count == 1 && is_tuple_payload)
                    {
                        binding_type = type_to_string(payload_type);
                        binding_type_info = payload_type;
                    }
                    else if (binding_count > 1 && is_tuple_payload && i < payload_type->count)
                    {
                        binding_type_info = payload_type->args[i];
                        binding_type = type_to_string(binding_type_info);
                    }
                }

                if (is_ref && binding_type_info)
                {
                    Type *ptr = type_new(TYPE_POINTER);
                    ptr->inner = binding_type_info;
                    binding_type_info = ptr;

                    char *ptr_s = xmalloc(strlen(binding_type) + 2);
                    sprintf(ptr_s, "%s*", binding_type); /* safe */
                    binding_type = ptr_s;
                }

                int is_generic_unresolved = 0;

                if (enum_def)
                {
                    if (enum_def->enm.generic_param)
                    {
                        char *param = enum_def->enm.generic_param;
                        if (strstr(binding_type, param))
                        {
                            is_generic_unresolved = 1;
                        }
                    }
                }

                if (!is_generic_unresolved &&
                    (strcmp(binding_type, "T") == 0 || strcmp(binding_type, "T*") == 0))
                {
                    is_generic_unresolved = 1;
                }

                if (is_generic_unresolved)
                {
                    if (is_ref)
                    {
                        binding_type = "unknown*";
                        Type *u = type_new(TYPE_UNKNOWN);
                        Type *p = type_new(TYPE_POINTER);
                        p->inner = u;
                        binding_type_info = p;
                    }
                    else
                    {
                        binding_type = "unknown";
                        binding_type_info = type_new(TYPE_UNKNOWN);
                    }
                }

                if (strcmp(binding, "parsed") == 0)
                {
                }
                add_symbol(ctx, binding, binding_type_info, 0);
            }
        }

        ASTNode *body = NULL;
        Token pk = lexer_peek(l);
        if (pk.kind == TOK_LBRACE)
        {
            body = parse_block(ctx, l);
        }
        else if (pk.kind == TOK_EXPECT ||
                 (pk.kind == TOK_IDENT && strncmp(pk.start, "expect", 6) == 0))
        {
            body = parse_expect(ctx, l);
        }
        else if (pk.kind == TOK_ASSERT ||
                 (pk.kind == TOK_IDENT && strncmp(pk.start, "assert", 6) == 0))
        {
            body = parse_assert(ctx, l);
        }
        else if (pk.kind == TOK_IDENT && strncmp(pk.start, "return", 6) == 0)
        {
            body = parse_return(ctx, l);
        }
        else if (ctx->is_fault_tolerant && (pk.kind == TOK_RBRACE || pk.kind == TOK_COMMA))
        {
            body = ast_create(NODE_BLOCK);
            body->token = pk;
        }
        else
        {
            body = parse_expression(ctx, l);
        }

        if (!body)
        {
            body = ast_create(NODE_BLOCK);
            body->token = pk;
        }

        exit_scope(ctx);

        ASTNode *c = ast_create(NODE_MATCH_CASE);
        c->token = pk;
        c->match_case.patterns = patterns;
        c->match_case.binding_names = bindings;
        c->match_case.binding_count = binding_count;
        c->match_case.binding_refs = binding_refs;
        c->match_case.is_destructuring = is_destructure;
        c->match_case.guard = guard;
        c->match_case.body = body;
        c->match_case.is_default = is_default;

        if (!h)
        {
            h = c;
        }
        else
        {
            tl->next = c;
        }
        tl = c;
    }
    lexer_next(l); // eat }

    ASTNode *n = ast_create(NODE_MATCH);
    n->line = start_token.line;
    n->token = start_token;
    n->match_stmt.expr = expr;
    n->match_stmt.cases = h;
    return n;
}
