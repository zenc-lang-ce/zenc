// SPDX-License-Identifier: MIT

/**
 * @file operators.h
 * @brief The operators of the language, in one table: how each is written, its class, its
 *        precedence, the method that overloads it and, for a compound assignment, the operator
 *        it applies. The phases ask the table instead of comparing the operator's text.
 */

#ifndef ZC_ALLOW_INTERNAL
#error "ast/operators.h is internal to Zen C. Include the appropriate public header instead."

#endif

#ifndef OPERATORS_H
#define OPERATORS_H

#include <stddef.h>

typedef enum
{
    OP_NONE = 0,

    // Binary.
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_POW,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_SHL,
    OP_SHR,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_AND,
    OP_OR,
    OP_COALESCE,
    OP_PIPE,
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_POW_ASSIGN,
    OP_BIT_AND_ASSIGN,
    OP_BIT_OR_ASSIGN,
    OP_BIT_XOR_ASSIGN,
    OP_SHL_ASSIGN,
    OP_SHR_ASSIGN,
    OP_COALESCE_ASSIGN,

    // Unary.
    OP_NEG,
    OP_PLUS,
    OP_NOT,
    OP_BIT_NOT,
    OP_DEREF,
    OP_ADDRESS,
    OP_PRE_INC,
    OP_PRE_DEC,
    OP_POST_INC,       ///< `x++`, written `_post++` in the AST.
    OP_POST_DEC,       ///< `x--`, written `_post--` in the AST.
    OP_ADDRESS_RVALUE, ///< `&` of a temporary, written `&_rval`: the call that takes it drops it.
    OP_PREFIX_AND_AND, ///< A prefix `&&`, emitted as written (GNU C's address of a label).

    OP_COUNT
} OperatorKind;

typedef enum
{
    OPERATOR_ARITHMETIC, ///< `+ - * / % **`, unary `-` and `+`.
    OPERATOR_BITWISE,    ///< `& | ^`, unary `~`.
    OPERATOR_SHIFT,      ///< `<< >>`.
    OPERATOR_COMPARISON, ///< `== != < <= > >=`.
    OPERATOR_LOGICAL,    ///< `&& ||`, unary `!`.
    OPERATOR_ASSIGNMENT, ///< `=` and the compound assignments.
    OPERATOR_OTHER,      ///< `??`, `|>`, and the unary `*`, `&`, `++`, `--` and their forms.
} OperatorClass;

typedef struct
{
    OperatorKind kind;
    const char *spelling; ///< As the AST and Zen C write it: `+=`, `<<`, `&&` (also for `and`).
    int is_unary;
    OperatorClass operator_class;
    int precedence;        ///< The parser's Precedence of a binary operator; 0 for unary ones.
    int right_associative; ///< `a ** b ** c` is `a ** (b ** c)`.
    const char *method;    ///< The method that overloads it (`add`, `eq`, `neg`); NULL if none.
    OperatorKind compound; ///< For a compound assignment, the operator it applies (`+=` -> `+`).
} OperatorInfo;

/** @brief The entry of `kind` in the table; NULL for OP_NONE. */
const OperatorInfo *operator_info(OperatorKind kind);

/** @brief The binary operator written `text` (`len` bytes, not necessarily terminated). */
OperatorKind binary_operator_from_text(const char *text, size_t len);
/** @brief The unary (prefix) operator written `text` (`len` bytes). */
OperatorKind unary_operator_from_text(const char *text, size_t len);
/** @brief The binary operator of a terminated text, such as `binary.op`; OP_NONE for NULL. */
OperatorKind binary_operator_of(const char *op);
/** @brief The unary operator of a terminated text, such as `unary.op`; OP_NONE for NULL. */
OperatorKind unary_operator_of(const char *op);
/** @brief The class of the binary operator `op`; OPERATOR_OTHER if it is none. */
OperatorClass binary_operator_class(const char *op);

/**
 * @brief Whether the operator text `op` (`len` bytes, not necessarily terminated) assigns:
 * `=` or a compound such as `+=`, `<<=` or `??=`.
 */
int is_assignment_op_text(const char *op, size_t len);
/** @brief is_assignment_op_text for a terminated string, such as `binary.op`. */
int is_assignment_op(const char *op);
/** @brief Whether the binary operator `op` compares: `== != < <= > >=`. */
int is_comparison_op(const char *op);
/** @brief The method that overloads the binary operator `op` (`add` for `+`), or NULL. */
const char *binary_operator_method(const char *op);

#endif
