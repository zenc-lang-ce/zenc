// SPDX-License-Identifier: MIT

#include "primitives.h"
#include <string.h>

static const ZenPrimitive primitives[] = {{"U0", TYPE_VOID},
                                          {"u0", TYPE_VOID},
                                          {"void", TYPE_VOID},
                                          {"bool", TYPE_BOOL},
                                          {"char", TYPE_CHAR},
                                          {"I8", TYPE_I8},
                                          {"i8", TYPE_I8},
                                          {"int8", TYPE_I8},
                                          {"U8", TYPE_U8},
                                          {"u8", TYPE_U8},
                                          {"uint8", TYPE_U8},
                                          {"byte", TYPE_BYTE},
                                          {"I16", TYPE_I16},
                                          {"i16", TYPE_I16},
                                          {"int16", TYPE_I16},
                                          {"short", TYPE_I16},
                                          {"U16", TYPE_U16},
                                          {"u16", TYPE_U16},
                                          {"uint16", TYPE_U16},
                                          {"ushort", TYPE_U16},
                                          {"I32", TYPE_I32},
                                          {"i32", TYPE_I32},
                                          {"int32", TYPE_I32},
                                          {"int", TYPE_I32},
                                          {"U32", TYPE_U32},
                                          {"u32", TYPE_U32},
                                          {"uint32", TYPE_U32},
                                          {"uint", TYPE_U32},
                                          {"rune", TYPE_RUNE},
                                          {"I64", TYPE_I64},
                                          {"i64", TYPE_I64},
                                          {"int64", TYPE_I64},
                                          {"long", TYPE_I64},
                                          {"U64", TYPE_U64},
                                          {"u64", TYPE_U64},
                                          {"uint64", TYPE_U64},
                                          {"ulong", TYPE_U64},
                                          {"unsigned", TYPE_U32},
                                          {"signed", TYPE_I32},
                                          {"I128", TYPE_I128},
                                          {"i128", TYPE_I128},
                                          {"U128", TYPE_U128},
                                          {"u128", TYPE_U128},
                                          {"F32", TYPE_F32},
                                          {"f32", TYPE_F32},
                                          {"float", TYPE_F32},
                                          {"F64", TYPE_F64},
                                          {"f64", TYPE_F64},
                                          {"double", TYPE_F64},
                                          {"f128", TYPE_F64},
                                          {"usize", TYPE_USIZE},
                                          {"isize", TYPE_ISIZE},
                                          {"ssize_t", TYPE_ISIZE},
                                          {"c_int", TYPE_C_INT},
                                          {"c_uint", TYPE_C_UINT},
                                          {"c_long", TYPE_C_LONG},
                                          {"c_ulong", TYPE_C_ULONG},
                                          {"c_longlong", TYPE_C_LONGLONG},
                                          {"c_ulonglong", TYPE_C_ULONGLONG},
                                          {"c_short", TYPE_C_SHORT},
                                          {"c_ushort", TYPE_C_USHORT},
                                          {"c_char", TYPE_C_CHAR},
                                          {"c_uchar", TYPE_C_UCHAR},
                                          {"string", TYPE_STRING}};

const ZenPrimitive *get_zen_primitives(int *count)
{
    if (count)
    {
        *count = sizeof(primitives) / sizeof(primitives[0]);
    }
    return primitives;
}

const ZenPrimitive *find_primitive_by_name(const char *name)
{
    int count = sizeof(primitives) / sizeof(primitives[0]);
    for (int i = 0; i < count; i++)
    {
        if (strcmp(primitives[i].zen_name, name) == 0)
        {
            return &primitives[i];
        }
    }
    return NULL;
}

const ZenPrimitive *find_primitive_by_c_name(const char *c_name)
{
    int count = sizeof(primitives) / sizeof(primitives[0]);
    for (int i = 0; i < count; i++)
    {
        const char *c = primitive_c_name(primitives[i].kind, 0);
        if (c && strcmp(c, c_name) == 0)
        {
            return &primitives[i];
        }
    }
    return NULL;
}

const char *get_primitive_c_name(const char *name)
{
    const ZenPrimitive *p = find_primitive_by_name(name);
    const char *c = p ? primitive_c_name(p->kind, 0) : NULL;
    return c ? c : name;
}

TypeKind find_primitive_kind(const char *name)
{
    const ZenPrimitive *p = find_primitive_by_name(name);
    if (p)
    {
        return p->kind;
    }
    p = find_primitive_by_c_name(name);
    if (p)
    {
        return p->kind;
    }
    return TYPE_UNKNOWN;
}

ZEN_CONST const char *primitive_zen_name(TypeKind kind)
{
    switch (kind)
    {
    case TYPE_I8:
        return "i8";
    case TYPE_U8:
        return "u8";
    case TYPE_I16:
        return "i16";
    case TYPE_U16:
        return "u16";
    case TYPE_I32:
        return "i32";
    case TYPE_U32:
        return "u32";
    case TYPE_I64:
        return "i64";
    case TYPE_U64:
        return "u64";
    case TYPE_I128:
        return "i128";
    case TYPE_U128:
        return "u128";
    case TYPE_F32:
        return "f32";
    case TYPE_F64:
        return "f64";
    case TYPE_USIZE:
        return "usize";
    case TYPE_ISIZE:
        return "isize";
    case TYPE_BYTE:
        return "byte";
    default:
        return NULL;
    }
}

// clang-format off
static const IntegerKind integer_kinds[] = {
    {TYPE_BOOL,   0,   INTEGER_NO_SIGN},
    {TYPE_CHAR,   8,   INTEGER_NO_SIGN},
    {TYPE_I8,     8,   INTEGER_SIGNED},       {TYPE_U8,          8,   INTEGER_UNSIGNED},
    {TYPE_BYTE,   8,   INTEGER_UNSIGNED},
    {TYPE_C_CHAR, 8,   INTEGER_SIGNED},       {TYPE_C_UCHAR,     8,   INTEGER_UNSIGNED},
    {TYPE_I16,    16,  INTEGER_SIGNED},       {TYPE_U16,         16,  INTEGER_UNSIGNED},
    {TYPE_C_SHORT, 16, INTEGER_SIGNED},       {TYPE_C_USHORT,    16,  INTEGER_UNSIGNED},
    {TYPE_I32,    32,  INTEGER_SIGNED},       {TYPE_U32,         32,  INTEGER_UNSIGNED},
    {TYPE_RUNE,   32,  INTEGER_SIGNED},
    {TYPE_C_INT,  32,  INTEGER_SIGNED},       {TYPE_C_UINT,      32,  INTEGER_UNSIGNED},
    {TYPE_I64,    64,  INTEGER_SIGNED},       {TYPE_U64,         64,  INTEGER_UNSIGNED},
    {TYPE_ISIZE,  64,  INTEGER_SIGNED},       {TYPE_USIZE,       64,  INTEGER_UNSIGNED},
    {TYPE_C_LONG, 64,  INTEGER_SIGNED},       {TYPE_C_ULONG,     64,  INTEGER_UNSIGNED},
    {TYPE_C_LONGLONG, 64, INTEGER_SIGNED},    {TYPE_C_ULONGLONG, 64,  INTEGER_UNSIGNED},
    {TYPE_I128,   128, INTEGER_SIGNED},       {TYPE_U128,        128, INTEGER_UNSIGNED},
    {TYPE_BITINT, 0,   INTEGER_SIGNED},       {TYPE_UBITINT,     0,   INTEGER_UNSIGNED},
};
// clang-format on

ZEN_CONST const IntegerKind *integer_kind(TypeKind kind)
{
    for (size_t i = 0; i < sizeof(integer_kinds) / sizeof(integer_kinds[0]); i++)
    {
        if (integer_kinds[i].kind == kind)
        {
            return &integer_kinds[i];
        }
    }
    return NULL;
}

// clang-format off
static const struct
{
    TypeKind kind;
    const char *c_name;
    const char *misra_c_name; ///< Under --misra, a C type of a fixed width; NULL: the same.
} primitive_c_names[] = {
    {TYPE_VOID, "void", NULL},             {TYPE_BOOL, "bool", NULL},
    {TYPE_STRING, "string", NULL},         {TYPE_CHAR, "char", NULL},
    {TYPE_I8, "int8_t", NULL},             {TYPE_U8, "uint8_t", NULL},
    {TYPE_I16, "int16_t", NULL},           {TYPE_U16, "uint16_t", NULL},
    {TYPE_I32, "int32_t", NULL},           {TYPE_U32, "uint32_t", NULL},
    {TYPE_I64, "int64_t", NULL},           {TYPE_U64, "uint64_t", NULL},
    {TYPE_F32, "float", NULL},             {TYPE_F64, "double", NULL},
    {TYPE_USIZE, "size_t", NULL},          {TYPE_ISIZE, "ptrdiff_t", NULL},
    {TYPE_BYTE, "uint8_t", NULL},          {TYPE_RUNE, "int32_t", NULL},
    {TYPE_I128, "__int128", NULL},         {TYPE_U128, "unsigned __int128", NULL},
    {TYPE_C_INT, "int", "int32_t"},        {TYPE_C_UINT, "unsigned int", "uint32_t"},
    {TYPE_C_LONG, "long", "int64_t"},      {TYPE_C_ULONG, "unsigned long", "uint64_t"},
    {TYPE_C_LONGLONG, "long long", "int64_t"}, {TYPE_C_ULONGLONG, "unsigned long long", "uint64_t"},
    {TYPE_C_SHORT, "short", "int16_t"},    {TYPE_C_USHORT, "unsigned short", "uint16_t"},
    {TYPE_C_CHAR, "char", "int8_t"},       {TYPE_C_UCHAR, "unsigned char", "uint8_t"},
};
// clang-format on

ZEN_CONST const char *primitive_c_name(TypeKind kind, int misra_mode)
{
    for (size_t i = 0; i < sizeof(primitive_c_names) / sizeof(primitive_c_names[0]); i++)
    {
        if (primitive_c_names[i].kind == kind)
        {
            return misra_mode && primitive_c_names[i].misra_c_name
                       ? primitive_c_names[i].misra_c_name
                       : primitive_c_names[i].c_name;
        }
    }
    return NULL;
}
