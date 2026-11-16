// SPDX-License-Identifier: MIT
#include "operators.h"
#include "../parser/parser.h"
#include <string.h>

// clang-format off
// `?\?` is `??`: written plainly, `??=` would be a trigraph.
static const OperatorInfo OPERATORS[] = {
    // kind               spelling  unary class                prec             right method    compound
    {OP_ADD,              "+",      0, OPERATOR_ARITHMETIC, PREC_TERM,       0, "add",    OP_NONE},
    {OP_SUB,              "-",      0, OPERATOR_ARITHMETIC, PREC_TERM,       0, "sub",    OP_NONE},
    {OP_MUL,              "*",      0, OPERATOR_ARITHMETIC, PREC_FACTOR,     0, "mul",    OP_NONE},
    {OP_DIV,              "/",      0, OPERATOR_ARITHMETIC, PREC_FACTOR,     0, "div",    OP_NONE},
    {OP_MOD,              "%",      0, OPERATOR_ARITHMETIC, PREC_FACTOR,     0, "rem",    OP_NONE},
    {OP_POW,              "**",     0, OPERATOR_ARITHMETIC, PREC_POWER,      1, "pow",    OP_NONE},
    {OP_BIT_AND,          "&",      0, OPERATOR_BITWISE,    PREC_TERM,       0, "bitand", OP_NONE},
    {OP_BIT_OR,           "|",      0, OPERATOR_BITWISE,    PREC_TERM,       0, "bitor",  OP_NONE},
    {OP_BIT_XOR,          "^",      0, OPERATOR_BITWISE,    PREC_TERM,       0, "bitxor", OP_NONE},
    {OP_SHL,              "<<",     0, OPERATOR_SHIFT,      PREC_TERM,       0, "shl",    OP_NONE},
    {OP_SHR,              ">>",     0, OPERATOR_SHIFT,      PREC_TERM,       0, "shr",    OP_NONE},
    {OP_EQ,               "==",     0, OPERATOR_COMPARISON, PREC_EQUALITY,   0, "eq",     OP_NONE},
    {OP_NE,               "!=",     0, OPERATOR_COMPARISON, PREC_EQUALITY,   0, "neq",    OP_NONE},
    {OP_LT,               "<",      0, OPERATOR_COMPARISON, PREC_COMPARISON, 0, "lt",     OP_NONE},
    {OP_LE,               "<=",     0, OPERATOR_COMPARISON, PREC_COMPARISON, 0, "le",     OP_NONE},
    {OP_GT,               ">",      0, OPERATOR_COMPARISON, PREC_COMPARISON, 0, "gt",     OP_NONE},
    {OP_GE,               ">=",     0, OPERATOR_COMPARISON, PREC_COMPARISON, 0, "ge",     OP_NONE},
    {OP_AND,              "&&",     0, OPERATOR_LOGICAL,    PREC_AND,        0, NULL,     OP_NONE},
    {OP_OR,               "||",     0, OPERATOR_LOGICAL,    PREC_OR,         0, NULL,     OP_NONE},
    {OP_COALESCE,         "?\?",    0, OPERATOR_OTHER,      PREC_OR,         0, NULL,     OP_NONE},
    {OP_PIPE,             "|>",     0, OPERATOR_OTHER,      PREC_TERM,       0, NULL,     OP_NONE},
    {OP_ASSIGN,           "=",      0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_NONE},
    {OP_ADD_ASSIGN,       "+=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_ADD},
    {OP_SUB_ASSIGN,       "-=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_SUB},
    {OP_MUL_ASSIGN,       "*=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_MUL},
    {OP_DIV_ASSIGN,       "/=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_DIV},
    {OP_MOD_ASSIGN,       "%=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_MOD},
    {OP_POW_ASSIGN,       "**=",    0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 1, NULL,     OP_POW},
    {OP_BIT_AND_ASSIGN,   "&=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_BIT_AND},
    {OP_BIT_OR_ASSIGN,    "|=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_BIT_OR},
    {OP_BIT_XOR_ASSIGN,   "^=",     0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_BIT_XOR},
    {OP_SHL_ASSIGN,       "<<=",    0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_SHL},
    {OP_SHR_ASSIGN,       ">>=",    0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_SHR},
    {OP_COALESCE_ASSIGN,  "?\?=",   0, OPERATOR_ASSIGNMENT, PREC_ASSIGNMENT, 0, NULL,     OP_COALESCE},
    {OP_NEG,              "-",      1, OPERATOR_ARITHMETIC, 0,               0, "neg",    OP_NONE},
    {OP_PLUS,             "+",      1, OPERATOR_ARITHMETIC, 0,               0, NULL,     OP_NONE},
    {OP_NOT,              "!",      1, OPERATOR_LOGICAL,    0,               0, "not",    OP_NONE},
    {OP_BIT_NOT,          "~",      1, OPERATOR_BITWISE,    0,               0, "bitnot", OP_NONE},
    {OP_DEREF,            "*",      1, OPERATOR_OTHER,      0,               0, NULL,     OP_NONE},
    {OP_ADDRESS,          "&",      1, OPERATOR_OTHER,      0,               0, NULL,     OP_NONE},
    {OP_PRE_INC,          "++",     1, OPERATOR_OTHER,      0,               0, NULL,     OP_NONE},
    {OP_PRE_DEC,          "--",     1, OPERATOR_OTHER,      0,               0, NULL,     OP_NONE},
    {OP_POST_INC,         "_post++", 1, OPERATOR_OTHER,     0,               0, NULL,     OP_NONE},
    {OP_POST_DEC,         "_post--", 1, OPERATOR_OTHER,     0,               0, NULL,     OP_NONE},
    {OP_ADDRESS_RVALUE,   "&_rval", 1, OPERATOR_OTHER,      0,               0, NULL,     OP_NONE},
    {OP_PREFIX_AND_AND,   "&&",     1, OPERATOR_OTHER,      0,               0, NULL,     OP_NONE},
};
// clang-format on

#define OPERATOR_TABLE_SIZE (sizeof(OPERATORS) / sizeof(OPERATORS[0]))

const OperatorInfo *operator_info(OperatorKind kind)
{
    for (size_t i = 0; i < OPERATOR_TABLE_SIZE; i++)
    {
        if (OPERATORS[i].kind == kind)
        {
            return &OPERATORS[i];
        }
    }
    return NULL;
}

static OperatorKind operator_from_text(const char *text, size_t len, int is_unary)
{
    if (!text)
    {
        return OP_NONE;
    }
    for (size_t i = 0; i < OPERATOR_TABLE_SIZE; i++)
    {
        const OperatorInfo *op = &OPERATORS[i];
        if (op->is_unary == is_unary && strlen(op->spelling) == len &&
            strncmp(op->spelling, text, len) == 0)
        {
            return op->kind;
        }
    }
    return OP_NONE;
}

OperatorKind binary_operator_from_text(const char *text, size_t len)
{
    return operator_from_text(text, len, 0);
}

OperatorKind unary_operator_from_text(const char *text, size_t len)
{
    return operator_from_text(text, len, 1);
}

int is_assignment_op_text(const char *op, size_t len)
{
    return is_assignment_operator(binary_operator_from_text(op, len));
}

OperatorClass operator_class(OperatorKind kind)
{
    const OperatorInfo *info = operator_info(kind);
    return info ? info->operator_class : OPERATOR_OTHER;
}

int is_assignment_operator(OperatorKind kind)
{
    return kind != OP_NONE && operator_class(kind) == OPERATOR_ASSIGNMENT;
}

int is_comparison_operator(OperatorKind kind)
{
    return kind != OP_NONE && operator_class(kind) == OPERATOR_COMPARISON;
}

const char *operator_method(OperatorKind kind)
{
    const OperatorInfo *info = operator_info(kind);
    return info ? info->method : NULL;
}

const char *operator_spelling(OperatorKind kind)
{
    const OperatorInfo *info = operator_info(kind);
    return info ? info->spelling : "";
}
