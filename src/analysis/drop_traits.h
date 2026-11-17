// SPDX-License-Identifier: MIT
#ifndef ZC_DROP_TRAITS_H
#define ZC_DROP_TRAITS_H

#include "parser/parser.h"

/**
 * @brief Traverses all parsed structs and propagates `has_drop` from fields to their parent
 * structs.
 */
void propagate_drop_traits(ParserContext *ctx);
/**
 * @brief Whether a value of type `t` needs dropping: an owning closure, a struct with Drop, or a
 *        tuple or fixed-size array holding a value that needs dropping.
 */
int type_needs_drop(ParserContext *ctx, Type *t);
/**
 * @brief Whether a value of type `t` will be dropped, decided from the declarations alone: what
 *        type_needs_drop says once propagate_drop_traits has run, but also before it, while bodies
 *        are parsed (a struct with an `impl Drop` or a field that will be dropped).
 */
int type_will_drop(ParserContext *ctx, Type *t);
/**
 * @brief Whether `t` has drop glue: for a struct or enum, what its declaration says (its own
 * `impl Drop`, or a field or payload that needs dropping); for another type, such as an owning
 * closure, its own flag.
 */
int type_has_drop(ParserContext *ctx, Type *t);

#endif
