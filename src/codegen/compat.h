// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "codegen/compat.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef ZC_COMPAT_H
#define ZC_COMPAT_H

#include "platform/compiler.h"

/* File extensions for mixed source compilation */
static const char ZC_EXT_C[] = ".c";
static const char ZC_EXT_CPP[] = ".cpp";
static const char ZC_EXT_M[] = ".m";
static const char ZC_EXT_MM[] = ".mm";
static const char ZC_EXT_O[] = ".o";
static const char ZC_EXT_A[] = ".a";
static const char ZC_EXT_DYLIB[] = ".dylib";
static const char ZC_EXT_SO[] = ".so";

#define ZC_IS_BACKEND_EXT(ext)                                                                     \
    (strcmp(ext, ZC_EXT_C) == 0 || strcmp(ext, ZC_EXT_CPP) == 0 || strcmp(ext, ZC_EXT_M) == 0 ||   \
     strcmp(ext, ZC_EXT_MM) == 0 || strcmp(ext, ZC_EXT_O) == 0 || strcmp(ext, ZC_EXT_A) == 0 ||    \
     strcmp(ext, ZC_EXT_DYLIB) == 0 || strcmp(ext, ZC_EXT_SO) == 0)

/* Centralized string definition for codegen emission */
#define ZC_TCC_COMPAT_STR                                                                          \
    "#ifdef __TINYC__\n"                                                                           \
    "#undef ZC_AUTO\n"                                                                             \
    "#define ZC_AUTO __auto_type\n"                                                                \
    "#undef ZC_AUTO_INIT\n"                                                                        \
    "#define ZC_AUTO_INIT(var, init) __typeof__((init)) var = (init)\n"                            \
    "\n"                                                                                           \
    "#ifndef __builtin_expect\n"                                                                   \
    "#define __builtin_expect(x, v) (x)\n"                                                         \
    "#endif\n"                                                                                     \
    "\n"                                                                                           \
    "#ifndef __builtin_unreachable\n"                                                              \
    "#define __builtin_unreachable()\n"                                                            \
    "#endif\n"                                                                                     \
    "#else\n"                                                                                      \
    "#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202300L\n"                               \
    "#define ZC_AUTO_INIT(var, init) auto var = (init)\n"                                          \
    "#else\n"                                                                                      \
    "#define ZC_AUTO_INIT(var, init) __auto_type var = (init)\n"                                   \
    "#endif\n"                                                                                     \
    "#endif\n"

#ifdef __SIZEOF_INT128__
#define _z_safe_i128(x) _Generic((x), __int128: (x), default: (__int128)0)
#define _z_safe_u128(x) _Generic((x), unsigned __int128: (x), default: (unsigned __int128)0)
#define _z_128_map , __int128 : "%s", unsigned __int128 : "%s"
#define _z_128_arg_map(x)                                                                          \
    , __int128 : _z_i128_str(_z_safe_i128(x)), unsigned __int128 : _z_u128_str(_z_safe_u128(x))
#else
#define _z_128_map
#define _z_128_arg_map(x)
#endif

#define ZC_C_GENERIC_STR                                                                           \
    "#ifdef __SIZEOF_INT128__\n"                                                                   \
    "static inline const char *_z_u128_str(unsigned __int128 val) {\n"                             \
    "    static _Thread_local char buf[40];\n"                                                     \
    "    if (val == 0) return \"0\";\n"                                                            \
    "    int i = 38;\n"                                                                            \
    "    buf[39] = 0;\n"                                                                           \
    "    while (val > 0) { buf[i--] = (char)((val % 10) + '0'); val /= 10; }\n"                    \
    "    return &buf[i + 1];\n"                                                                    \
    "}\n"                                                                                          \
    "static inline const char *_z_i128_str(__int128 val) {\n"                                      \
    "    static _Thread_local char buf[41];\n"                                                     \
    "    if (val == 0) return \"0\";\n"                                                            \
    "    int neg = val < 0;\n"                                                                     \
    "    unsigned __int128 uval = (unsigned __int128)(neg ? -val : val);\n"                        \
    "    int i = 39;\n"                                                                            \
    "    buf[40] = 0;\n"                                                                           \
    "    while (uval > 0) { buf[i--] = (char)((uval % 10) + '0'); uval /= 10; }\n"                 \
    "    if (neg) buf[i--] = '-';\n"                                                               \
    "    return &buf[i + 1];\n"                                                                    \
    "}\n"                                                                                          \
    "#define _z_128_map ,__int128: \"%s\", unsigned __int128: \"%s\"\n"                            \
    "#else\n"                                                                                      \
    "#define _z_128_map\n"                                                                         \
    "#endif\n"                                                                                     \
    "#ifdef __OBJC__\n"                                                                            \
    "#define _z_objc_map\n"                                                                        \
    "#define _z_objc_arg_map(x) ,id: [(id)(x) description].UTF8String\n"                           \
    "#else\n"                                                                                      \
    "#define _z_objc_map\n"                                                                        \
    "#define _z_objc_arg_map(x)\n"                                                                 \
    "#endif\n"                                                                                     \
    "\n"

#define ZC_C_ARG_GENERIC_STR                                                                       \
    "#ifdef __SIZEOF_INT128__\n"                                                                   \
    "#define _z_safe_i128(x) _Generic((x), __int128: (x), default: (__int128)0)\n"                 \
    "#define _z_safe_u128(x) _Generic((x), unsigned __int128: (x), default: (unsigned "            \
    "__int128)0)\n"                                                                                \
    "#define _z_128_arg_map(x) ,__int128: _z_i128_str(_z_safe_i128(x)), unsigned __int128: "       \
    "_z_u128_str(_z_safe_u128(x))\n"                                                               \
    "#else\n"                                                                                      \
    "#define _z_128_arg_map(x)\n"                                                                  \
    "#endif\n"                                                                                     \
    "#define _z_safe_bool(x) _Generic((x), _Bool: (x), default: (_Bool)0)\n"                       \
    "#define _z_arg(x) _Generic((x), _Bool: _z_bool_str(_z_safe_bool(x)) _z_128_arg_map(x), "      \
    "default: (x))\n"

#endif
