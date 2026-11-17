// SPDX-License-Identifier: MIT
#ifndef ZC_DECLARATION_CHECKS_H
#define ZC_DECLARATION_CHECKS_H

#include "parser/parser.h"

/**
 * @brief Warns that the type `use` names is unknown, with the closest known name as a hint.
 */
void warn_unknown_type(ParserContext *ctx, const TypeUsage *use);

/**
 * @brief Warns about every type name the parse read that names no type. Always returns 1:
 * the C compiler decides whether the name is an external C type.
 */
int validate_types(ParserContext *ctx);

/**
 * @brief Reports functions and methods defined twice under the same C name.
 *
 * Prototypes, externs, generic templates, `@cfg` definitions and definitions
 * inside a preprocessor `#if` block are not counted. Returns 0 on error.
 */
int validate_function_definitions(ParserContext *ctx, ASTNode *root);

#endif
