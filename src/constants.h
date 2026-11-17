// SPDX-License-Identifier: MIT
#ifndef ZEN_CONSTANTS_H
#ifndef ZC_ALLOW_INTERNAL
#error "constants.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define ZEN_CONSTANTS_H

/**
 * @file constants.h
 * @brief Type checking helpers, buffer size constants.
 */

#include "compat/c23_compat.h"
#include <string.h>

// Buffer sizes — enum for compile-time constant expression (array sizes, etc.)
// Use ZEN_CONSTEXPR for non-array-size constants.
enum
{
    MAX_TYPE_NAME_LEN = 1024,
    MAX_VAR_NAME_LEN = 256,
    MAX_FUNC_NAME_LEN = 1024,
    MAX_SHORT_MSG_LEN = 256,
    MAX_ERROR_MSG_LEN = 2048,
    MAX_MANGLED_NAME_LEN = 2048,
    MAX_PATH_LEN = 4096
};

// The `struct ` a C type name may start with.
static inline bool str_is_struct_prefix(const char *t)
{
    return t && strncmp(t, "struct ", 7) == 0;
}

static inline const char *str_strip_struct_prefix(const char *t)
{
    return str_is_struct_prefix(t) ? t + 7 : t;
}

#endif // ZEN_CONSTANTS_H
