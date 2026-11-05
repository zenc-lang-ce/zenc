// SPDX-License-Identifier: MIT
// Unit tests of src/parser/utils/utils_c_types.c: C types written as C text, as the C header
// reader gives them, turned into Types.
#include "unit_parse.h"
#include "unit_test.h"
#include <string.h>

// The C spelling of the Type for `text`, as codegen writes it, or "(none)".
static const char *spelled(const char *text)
{
    parse_source("fn main() {}\n");
    Type *t = c_type_from_text(parse_source_context(), text);
    return t ? type_to_c_string(t, 0) : "(none)";
}

UTEST(c_type_from_text, integers_keep_their_c_type)
{
    ASSERT_STREQ("int", spelled("int"));
    ASSERT_STREQ("unsigned int", spelled("unsigned"));
    ASSERT_STREQ("long", spelled("long int"));
    ASSERT_STREQ("unsigned long", spelled("long unsigned int"));
    ASSERT_STREQ("long long", spelled("long long"));
    ASSERT_STREQ("short", spelled("short"));
    ASSERT_STREQ("char", spelled("char"));
    ASSERT_STREQ("unsigned char", spelled("unsigned char"));
}

UTEST(c_type_from_text, standard_typedefs_are_primitives)
{
    ASSERT_STREQ("size_t", spelled("size_t"));
    ASSERT_STREQ("uint8_t", spelled("uint8_t"));
    ASSERT_STREQ("double", spelled("double"));
    ASSERT_STREQ("bool", spelled("_Bool"));
}

UTEST(c_type_from_text, pointers_const_and_structs)
{
    ASSERT_STREQ("void*", spelled("void *"));
    ASSERT_STREQ("const char*", spelled("const char *"));
    ASSERT_STREQ("char**", spelled("char **"));
    ASSERT_STREQ("struct tm*", spelled("struct tm *"));
    // A name the compiler does not know is an opaque C struct behind a pointer.
    ASSERT_STREQ("FILE*", spelled("FILE *"));
}

UTEST(c_type_from_text, unknown_or_unsupported_types_have_none)
{
    // A typedef alone may be an integer: no type rather than a wrong one.
    ASSERT_STREQ("(none)", spelled("time_t"));
    ASSERT_STREQ("(none)", spelled("long double"));
    ASSERT_STREQ("(none)", spelled(""));
}
