// SPDX-License-Identifier: MIT
#include "../parser/parser.h"

#include "codegen.h"
#include "codegen_internal.h"
#include "zprep.h"
#include "../constants.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../plugins/plugin_manager.h"
#include "ast.h"
#include "zprep_plugin.h"

// The C condition that the subject `_m_<id>` matches one alternative of a case.
static void emit_alternative_condition(ParserContext *ctx, MatchPattern *p, int id, int is_ptr)
{
    const char *subject = is_ptr ? "*_m_" : "_m_";
    switch (p->kind)
    {
    case PATTERN_WILDCARD:
        EMIT(ctx, "1");
        break;
    case PATTERN_RANGE:
        EMIT(ctx, "(%s%d >= %s && %s%d %s %s)", subject, id, p->range_start, subject, id,
             p->is_inclusive ? "<=" : "<", p->range_end);
        break;
    case PATTERN_LITERAL:
        if (p->literal[0] == '"')
        {
            EMIT(ctx, "strcmp(%s%d, %s) == 0", subject, id, p->literal);
        }
        else
        {
            EMIT(ctx, "%s%d == %s", subject, id, p->literal);
        }
        break;
    case PATTERN_NAME:
    {
        EnumVariantReg *reg = p->name.scope ? find_enum_variant_of(ctx, p->name.scope, p->name.name)
                                            : find_enum_variant(ctx, p->name.name);
        if (!reg)
        {
            // A constant, not a variant.
            EMIT(ctx, "%s%d == %s", subject, id,
                 p->name.scope ? mangle_module_symbol(p->name.scope, p->name.name) : p->name.name);
        }
        else if (is_simple_enum(ctx, reg->enum_name))
        {
            EMIT(ctx, "%s%d == %d", subject, id, reg->tag_id);
        }
        else
        {
            EMIT(ctx, "_m_%d%stag == %d", id, is_ptr ? "->" : ".", reg->tag_id);
        }
        break;
    }
    default:
        break;
    }
}

// The C condition that the subject matches any alternative of a case (`A || B`).
static void emit_pattern_condition(ParserContext *ctx, MatchPattern *patterns, int id, int is_ptr)
{
    if (!patterns->next)
    {
        emit_alternative_condition(ctx, patterns, id, is_ptr);
        return;
    }
    EMIT(ctx, "(");
    for (MatchPattern *p = patterns; p; p = p->next)
    {
        if (p != patterns)
        {
            EMIT(ctx, " || ");
        }
        emit_alternative_condition(ctx, p, id, is_ptr);
    }
    EMIT(ctx, ")");
}

// The variant a case names, as written ("Not_Found" for `Reply::Not_Found(n)`), or NULL.
static const char *case_variant(ASTNode *c)
{
    MatchPattern *p = c->match_case.patterns;
    return (p && p->kind == PATTERN_NAME) ? p->name.name : NULL;
}

// Whether the case is the wildcard `_` alone.
static int case_is_wildcard(ASTNode *c)
{
    MatchPattern *p = c->match_case.patterns;
    return p && p->kind == PATTERN_WILDCARD && !p->next;
}

// Helper
ZEN_CONST bool is_int_type(TypeKind k)
{
    static const bool is_int[] = {
        [TYPE_CHAR] = true,    [TYPE_I8] = true,         [TYPE_U8] = true,
        [TYPE_I16] = true,     [TYPE_U16] = true,        [TYPE_I32] = true,
        [TYPE_U32] = true,     [TYPE_I64] = true,        [TYPE_U64] = true,
        [TYPE_I128] = true,    [TYPE_U128] = true,       [TYPE_INT] = true,
        [TYPE_UINT] = true,    [TYPE_USIZE] = true,      [TYPE_ISIZE] = true,
        [TYPE_BYTE] = true,    [TYPE_RUNE] = true,       [TYPE_ENUM] = true,
        [TYPE_C_INT] = true,   [TYPE_C_UINT] = true,     [TYPE_C_LONG] = true,
        [TYPE_C_ULONG] = true, [TYPE_C_LONGLONG] = true, [TYPE_C_ULONGLONG] = true,
        [TYPE_C_SHORT] = true, [TYPE_C_USHORT] = true,   [TYPE_C_CHAR] = true,
        [TYPE_C_UCHAR] = true, [TYPE_BITINT] = true,     [TYPE_UBITINT] = true};
    if (k < (int)(sizeof(is_int) / sizeof(is_int[0])))
    {
        return is_int[k];
    }
    return false;
}

void codegen_match_internal(ParserContext *ctx, ASTNode *node, int use_result)
{
    int id = ctx->cg.tmp_counter++;
    int is_self = (node->match_stmt.expr->kind == NODE_EXPR_VAR &&
                   strcmp(node->match_stmt.expr->var_ref.name, "self") == 0);

    char *ret_type = infer_type(ctx, node);
    int is_expr = (use_result && ret_type && strcmp(ret_type, "void") != 0);

    if (is_expr)
    {
        EMIT(ctx, "({ ");
    }
    else
    {
        EMIT(ctx, "{ ");
    }

    // Check if any case uses ref binding - only take address if needed
    int has_ref_binding = 0;
    ASTNode *ref_check = node->match_stmt.cases;
    while (ref_check)
    {
        if (ref_check->match_case.binding_refs)
        {
            for (int i = 0; i < ref_check->match_case.binding_count; i++)
            {
                if (ref_check->match_case.binding_refs[i])
                {
                    has_ref_binding = 1;
                    break;
                }
            }
        }
        if (has_ref_binding)
        {
            break;
        }
        ref_check = ref_check->next;
    }

    // A pointer subject is matched through the pointer: `_m_<id>` holds it as it is, and the
    // cases read the value it points to with `->`, as they do for a `ref` binding.
    Type *scrutinee = get_inner_type(node->match_stmt.expr->type_info);
    Type *subject = match_subject_type(scrutinee);
    int is_option = type_is_instance_of(subject, "Option");
    int is_result = type_is_instance_of(subject, "Result");
    int subject_is_ptr = scrutinee && scrutinee->kind == TYPE_POINTER;
    int m_is_ptr = has_ref_binding || subject_is_ptr;

    // A subject that designates a place: a `ref` binding points into it instead of a copy.
    // `*p` is one too: `&*p` is the address p holds.
    ASTNode *subject_expr = node->match_stmt.expr;
    int is_lvalue_opt =
        (subject_expr->kind == NODE_EXPR_VAR || subject_expr->kind == NODE_EXPR_MEMBER ||
         subject_expr->kind == NODE_EXPR_INDEX ||
         (subject_expr->kind == NODE_EXPR_UNARY && strcmp(subject_expr->unary.op, "*") == 0));

    emit_source_mapping(ctx, node); // Step through match statements elegantly

    if (is_self)
    {
        emit_auto_type(ctx, node->match_stmt.expr, node->token);
        EMIT(ctx, " _m_%d = ", id);
        codegen_expression(ctx, node->match_stmt.expr);
        EMIT(ctx, "; ");
    }
    else if (has_ref_binding && !subject_is_ptr && is_lvalue_opt)
    {
        // Take address for ref bindings
        EMIT(ctx, "ZC_AUTO_INIT(_m_%d, &", id);
        codegen_expression(ctx, node->match_stmt.expr);
        EMIT(ctx, "); ");
    }
    else if (has_ref_binding && !subject_is_ptr)
    {
        // Non-lvalue with ref binding: create temporary
        emit_auto_type(ctx, node->match_stmt.expr, node->token);
        EMIT(ctx, " _temp_%d = ", id);
        codegen_expression(ctx, node->match_stmt.expr);
        EMIT(ctx, "; ");

        EMIT(ctx, "ZC_AUTO_INIT(_m_%d, &_temp_%d); ", id, id);
    }
    else
    {
        // No ref bindings, or a pointer subject: store the value as it is
        emit_auto_type(ctx, node->match_stmt.expr, node->token);
        EMIT(ctx, " _m_%d = ", id);
        codegen_expression(ctx, node->match_stmt.expr);
        EMIT(ctx, "; ");
    }

    if (is_expr)
    {
        EMIT(ctx, "%s _r_%d; ", ret_type, id);
    }

    // `has_wildcard` also decides the MISRA warnings further down.
    int has_wildcard = 0;
    for (ASTNode *chk = node->match_stmt.cases; chk; chk = chk->next)
    {
        has_wildcard = has_wildcard || case_is_wildcard(chk);
    }

    const char *missing = match_missing_variant(ctx, node, node->match_stmt.expr->type_info);
    if (missing && strcmp(missing, "_") != 0)
    {
        zwarn_at(node->token, "Non-exhaustive match: Missing variant '%s'", missing);
    }

    ASTNode *c = node->match_stmt.cases;
    int first = 1;
    while (c)
    {
        int is_wildcard = case_is_wildcard(c);
        int is_final_wildcard = (is_wildcard && c->next == NULL);

        if (!first)
        {
            EMIT(ctx, " else ");
        }

        emit_source_mapping(ctx, c); // Step through match cases elegantly

        if (!is_final_wildcard || first)
        {
            EMIT(ctx, "if (");
            if (is_wildcard)
            {
                EMIT(ctx, "1");
            }
            else if (is_option)
            {
                const char *acc = m_is_ptr ? "->" : ".";

                if (case_variant(c) && strcmp(case_variant(c), "Some") == 0)
                {
                    EMIT(ctx, "_m_%d%sis_some", id, acc);
                }
                else if (case_variant(c) && strcmp(case_variant(c), "None") == 0)
                {
                    EMIT(ctx, "!_m_%d%sis_some", id, acc);
                }
                else
                {
                    EMIT(ctx, "1");
                }
            }
            else if (is_result)
            {
                const char *acc = m_is_ptr ? "->" : ".";

                if (case_variant(c) && strcmp(case_variant(c), "Ok") == 0)
                {
                    EMIT(ctx, "_m_%d%sis_ok", id, acc);
                }
                else if (case_variant(c) && strcmp(case_variant(c), "Err") == 0)
                {
                    EMIT(ctx, "!_m_%d%sis_ok", id, acc);
                }
                else
                {
                    EMIT(ctx, "1");
                }
            }
            else
            {
                // Use helper for OR patterns, range patterns, and simple patterns
                if (c->match_case.patterns)
                {
                    emit_pattern_condition(ctx, c->match_case.patterns, id, m_is_ptr);
                }
                else
                {
                    EMIT(ctx, "1");
                }
            }
        }

        if (!is_final_wildcard || first)
        {
            EMIT(ctx, ") ");
        }
        EMIT(ctx, "{ ");
        if (c->match_case.binding_count > 0)
        {
            for (int i = 0; i < c->match_case.binding_count; i++)
            {
                if (!c->match_case.binding_names[i])
                {
                    continue;
                }
                const char *bname = c_name(ctx, c->match_case.binding_names[i]);
                int is_r = c->match_case.binding_refs ? c->match_case.binding_refs[i] : 0;

                if (is_option)
                {
                    if (is_r)
                    {
                        EMIT(ctx, "ZC_AUTO_INIT(%s, &_m_%d->val); ", bname, id);
                    }
                    else if (m_is_ptr)
                    {
                        EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d->val); ", bname, id);
                    }
                    else
                    {
                        EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d.val); ", bname, id);
                    }
                }
                else if (is_result)
                {
                    char *field = "val";
                    if (case_variant(c) && strcmp(case_variant(c), "Err") == 0)
                    {
                        field = "err";
                    }

                    if (is_r)
                    {
                        EMIT(ctx, "ZC_AUTO_INIT(%s, &_m_%d->%s); ", bname, id, field);
                    }
                    else if (m_is_ptr)
                    {
                        EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d->%s); ", bname, id, field);
                    }
                    else
                    {
                        EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d.%s); ", bname, id, field);
                    }
                }
                else
                {
                    const char *v = case_variant(c);

                    if (c->match_case.binding_count > 1)
                    {
                        // Tuple destructuring: data.Variant.vI
                        if (is_r)
                        {
                            EMIT(ctx, "ZC_AUTO_INIT(%s, &_m_%d->data.%s.v%d); ", bname, id, v, i);
                        }
                        else if (m_is_ptr)
                        {
                            EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d->data.%s.v%d); ", bname, id, v, i);
                        }
                        else
                        {
                            EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d.data.%s.v%d); ", bname, id, v, i);
                        }
                    }
                    else
                    {
                        // Single destructuring: data.Variant
                        if (is_r)
                        {
                            EMIT(ctx, "ZC_AUTO_INIT(%s, &_m_%d->data.%s); ", bname, id, v);
                        }
                        else if (m_is_ptr)
                        {
                            EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d->data.%s); ", bname, id, v);
                        }
                        else
                        {
                            EMIT(ctx, "ZC_AUTO_INIT(%s, _m_%d.data.%s); ", bname, id, v);
                        }
                    }
                }
            }
        }

        // Check if body is a string literal (should auto-print).
        ASTNode *body = c->match_case.body;
        int is_string_literal =
            (body->kind == NODE_EXPR_LITERAL && body->literal.kind == LITERAL_STRING);

        int gives_no_value =
            (body->kind == NODE_RETURN || body->kind == NODE_ASSERT || body->kind == NODE_EXPECT);
        if (is_expr && gives_no_value)
        {
            // An arm that returns, asserts or expects has no value to assign.
            codegen_node_single(ctx, body);
        }
        else if (is_expr)
        {
            EMIT(ctx, "_r_%d = ", id);
            if (body->kind == NODE_BLOCK)
            {
                int saved = ctx->cg.defer_count;
                EMIT(ctx, "({ ");
                ASTNode *stmt = body->block.statements;
                while (stmt)
                {
                    emit_source_mapping(ctx, stmt);
                    codegen_node_single(ctx, stmt);
                    stmt = stmt->next;
                }
                emit_defers_down_to(ctx, saved);
                ctx->cg.defer_count = saved;
                EMIT(ctx, " })");
            }
            else
            {
                // The value form: a nested match or if gives a value and a
                // string literal is not printed.
                codegen_expression(ctx, body);
            }
            EMIT(ctx, ";");
        }
        else
        {
            if (is_string_literal)
            {
                // RFC 0001: a string as the arm of a match statement no longer prints. For one
                // release it is an error that tells how to print it; afterwards it is an unused
                // expression, which would compile and print nothing. Only codegen knows whether
                // the match is a statement: in a match expression the string is its value.
                const char *hints[] = {"print it with println(\"...\"), from std/io.zc", NULL};
                zerror_with_hints(body->token, "a string on its own no longer prints", hints);
            }
            else
            {
                codegen_node_single(ctx, body);
            }
        }

        EMIT(ctx, " }");
        first = 0;
        c = c->next;
    }

    if (is_expr)
    {
        if (ctx->config->misra_mode && !has_wildcard)
        {
            EMIT(ctx, " else { } /* MISRA 15.7 */ ");
        }
        EMIT(ctx, " _r_%d; })", id);
    }
    else
    {
        if (ctx->config->misra_mode && !has_wildcard)
        {
            EMIT(ctx, " else { } /* MISRA 15.7 */ ");
        }
        EMIT(ctx, " }");
    }
}
