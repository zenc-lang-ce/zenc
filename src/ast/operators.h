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
/**
 * @brief Whether the operator text `op` (`len` bytes, not necessarily terminated) assigns:
 * `=` or a compound such as `+=`, `<<=` or `??=`.
 */
int is_assignment_op_text(const char *op, size_t len);

/** @brief The class of `kind`; OPERATOR_OTHER for OP_NONE. */
OperatorClass operator_class(OperatorKind kind);
/** @brief Whether `kind` assigns: `=` or a compound assignment. */
int is_assignment_operator(OperatorKind kind);
/** @brief Whether `kind` compares: `== != < <= > >=`. */
int is_comparison_operator(OperatorKind kind);
/** @brief The method that overloads `kind` (`add` for `+`, `neg` for unary `-`), or NULL. */
const char *operator_method(OperatorKind kind);
/** @brief How the AST writes `kind` (`+=`, `_post++`); "" for OP_NONE. */
const char *operator_spelling(OperatorKind kind);

#endif
