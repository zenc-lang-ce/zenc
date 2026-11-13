// SPDX-License-Identifier: MIT
#ifndef CONST_FOLD_H
#ifndef ZC_ALLOW_INTERNAL
#error "analysis/const_fold.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define CONST_FOLD_H

#include "ast/ast.h"

typedef struct ParserContext ParserContext;

typedef enum
{
    FOLD_OK,
    FOLD_DIVISION_BY_ZERO, ///< `/` or `%` by zero.
    FOLD_OVERFLOW,         ///< `LLONG_MIN / -1` and `LLONG_MIN % -1`.
    FOLD_SHIFT_RANGE,      ///< A shift by a negative amount or by 64 or more.
    FOLD_UNSUPPORTED,      ///< Not an operator on integers.
} FoldResult;

// The value of `left op right` on 64-bit integers, as C computes it but without its undefined
// behaviour: `+`, `-`, `*` and `<<` wrap around, and the cases C leaves undefined are reported
// instead. Comparisons and `&&`/`||` give 0 or 1. Constant folding and the comptime interpreter
// share it.
FoldResult fold_int_binary(OperatorKind op, long long left, long long right, long long *out);
// The value of `op operand` on a 64-bit integer: `-` (wrapping), `+`, `~` and `!`.
FoldResult fold_int_unary(OperatorKind op, long long operand, long long *out);

// Evaluates a constant integer expression.
// Returns 1 if successful and sets *out_val.
// Returns 0 if the expression is not a compile-time constant.
int eval_const_int_expr(ASTNode *node, ParserContext *ctx, long long *out_val);

#endif
