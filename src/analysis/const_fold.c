// SPDX-License-Identifier: MIT
#include "../constants.h"
#include "analysis/const_fold.h"
#include "../parser/parser.h"
#include "../ast/symbols.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>

FoldResult fold_int_binary(OperatorKind op, long long left, long long right, long long *out)
{
    // +, -, * and << work on the unsigned bits: a signed overflow is undefined
    // in C, an unsigned one wraps around.
    switch (op)
    {
    case OP_ADD:
        *out = (long long)((unsigned long long)left + (unsigned long long)right);
        return FOLD_OK;
    case OP_SUB:
        *out = (long long)((unsigned long long)left - (unsigned long long)right);
        return FOLD_OK;
    case OP_MUL:
        *out = (long long)((unsigned long long)left * (unsigned long long)right);
        return FOLD_OK;
    case OP_DIV:
    case OP_MOD:
        if (right == 0)
        {
            return FOLD_DIVISION_BY_ZERO;
        }
        if (left == LLONG_MIN && right == -1)
        {
            return FOLD_OVERFLOW;
        }
        *out = op == OP_DIV ? left / right : left % right;
        return FOLD_OK;
    case OP_SHL:
    case OP_SHR:
        if (right < 0 || right >= 64)
        {
            return FOLD_SHIFT_RANGE;
        }
        *out = op == OP_SHL ? (long long)((unsigned long long)left << right) : left >> right;
        return FOLD_OK;
    case OP_BIT_AND:
        *out = left & right;
        return FOLD_OK;
    case OP_BIT_OR:
        *out = left | right;
        return FOLD_OK;
    case OP_BIT_XOR:
        *out = left ^ right;
        return FOLD_OK;
    case OP_EQ:
        *out = (left == right);
        return FOLD_OK;
    case OP_NE:
        *out = (left != right);
        return FOLD_OK;
    case OP_LT:
        *out = (left < right);
        return FOLD_OK;
    case OP_GT:
        *out = (left > right);
        return FOLD_OK;
    case OP_LE:
        *out = (left <= right);
        return FOLD_OK;
    case OP_GE:
        *out = (left >= right);
        return FOLD_OK;
    case OP_AND:
        *out = (left && right);
        return FOLD_OK;
    case OP_OR:
        *out = (left || right);
        return FOLD_OK;
    default:
        return FOLD_UNSUPPORTED;
    }
}

FoldResult fold_int_unary(OperatorKind op, long long operand, long long *out)
{
    switch (op)
    {
    case OP_NEG:
        *out = (long long)(0ULL - (unsigned long long)operand); // -LLONG_MIN overflows
        return FOLD_OK;
    case OP_PLUS:
        *out = +operand;
        return FOLD_OK;
    case OP_BIT_NOT:
        *out = ~operand;
        return FOLD_OK;
    case OP_NOT:
        *out = !operand;
        return FOLD_OK;
    default:
        return FOLD_UNSUPPORTED;
    }
}

int eval_const_int_expr(ASTNode *node, ParserContext *ctx, long long *out_val)
{
    if (!node)
    {
        return 0;
    }

    switch (node->kind)
    {
    case NODE_EXPR_LITERAL:
        if (node->literal.kind == LITERAL_INT)
        {
            *out_val = (long long int)(node->literal.int_val);
            return 1;
        }
        return 0;

    case NODE_EXPR_VAR:
    {
        if (strcmp(node->var_ref.name, "true") == 0)
        {
            *out_val = 1;
            return 1;
        }
        if (strcmp(node->var_ref.name, "false") == 0)
        {
            *out_val = 0;
            return 1;
        }

        ZenSymbol *sym = find_symbol_entry(ctx, node->var_ref.name);
        if (sym)
        {
            // A variable or parameter is not a constant. If an enum variant has the same name,
            // the variable hides it: the expression does not take the variant's value.
            if (!sym->is_const_value)
            {
                return 0;
            }
            sym->is_used = 1;
            *out_val = sym->const_int_val;
            return 1;
        }
        // Check for enum variants
        TypeMember *variant = &node->var_ref.variant;
        EnumVariantReg *ev =
            variant->member
                ? find_enum_variant_of(ctx, type_def_name(variant->owner), variant->member)
                : find_enum_variant(ctx, node->var_ref.name);
        if (ev)
        {
            *out_val = (long long)ev->tag_id;
            return 1;
        }
        break;
    }

    case NODE_EXPR_BINARY:
    {
        long long left, right;
        if (!eval_const_int_expr(node->binary.left, ctx, &left))
        {
            return 0;
        }
        if (!eval_const_int_expr(node->binary.right, ctx, &right))
        {
            return 0;
        }

        return fold_int_binary(node->binary.op, left, right, out_val) == FOLD_OK;
    }

    case NODE_EXPR_UNARY:
    {
        long long operand;
        if (!eval_const_int_expr(node->unary.operand, ctx, &operand))
        {
            return 0;
        }

        return fold_int_unary(node->unary.op, operand, out_val) == FOLD_OK;
    }

    default:
        return 0;
    }
    return 0; // For warning.
}
