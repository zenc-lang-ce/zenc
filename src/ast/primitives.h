// SPDX-License-Identifier: MIT

/**
 * @file primitives.h
 * @brief Primitive type definitions and lookup tables.
 */

#ifndef ZC_ALLOW_INTERNAL
#error "ast/primitives.h is internal to Zen C. Include the appropriate public header instead."

#endif

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "../compat/c23_compat.h"
#include "ast.h"

typedef struct
{
    const char *zen_name;
    TypeKind kind;
    const char *c_name;
} ZenPrimitive;

/**
 * @brief Returns the table of Zen primitives.
 */
const ZenPrimitive *get_zen_primitives(int *count);

/**
 * @brief Finds a primitive by its Zen name.
 */
const ZenPrimitive *find_primitive_by_name(const char *name);

/**
 * @brief Finds a primitive by its C name.
 */
const ZenPrimitive *find_primitive_by_c_name(const char *c_name);

/**
 * @brief Normalizes a Zen type name to its C equivalent.
 */
const char *get_primitive_c_name(const char *name);

/**
 * @brief Returns the TypeKind for a given primitive name (Zen or C).
 */
TypeKind find_primitive_kind(const char *name);

/**
 * @brief The name a program writes for a fixed-width primitive (`i32`, `usize`...), or NULL
 * for any other kind. type_to_string spells these kinds by it.
 */
ZEN_CONST const char *primitive_zen_name(TypeKind kind);

/** @brief The sign of an integer kind: `bool` and `char` have none. */
typedef enum
{
    INTEGER_NO_SIGN,
    INTEGER_SIGNED,
    INTEGER_UNSIGNED
} IntegerSign;

/** @brief What the compiler knows of an integer kind. */
typedef struct
{
    TypeKind kind;
    int width; ///< Bits; 0 when it is not fixed (`bool`, `_BitInt(N)`).
    IntegerSign sign;
} IntegerKind;

/**
 * @brief The facts of the integer kind `kind`, or NULL when it is not an integer. The one table
 *        of them: is_integer_type, is_signed_type, is_unsigned_type and integer_type_width read it.
 */
ZEN_CONST const IntegerKind *integer_kind(TypeKind kind);

#endif
