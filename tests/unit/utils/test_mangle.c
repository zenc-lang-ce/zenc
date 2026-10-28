// SPDX-License-Identifier: MIT
// Unit tests of src/utils/mangle.c: the C names of Zen C symbols.
#include "utils/mangle.h"
#include <string.h>
#include "unit_test.h"

UTEST(merge_underscores, collapses_four_or_more_and_keeps_three)
{
    ASSERT_STREQ("a__b", merge_underscores("a____b"));
    ASSERT_STREQ("a__b", merge_underscores("a______b"));
    ASSERT_STREQ("a___b", merge_underscores("a___b"));
    ASSERT_STREQ("a__b", merge_underscores("a__b"));
}

UTEST(mangle_method_symbol, plain_and_trait_methods)
{
    ASSERT_STREQ("Point__add", mangle_method_symbol("Point", NULL, "add"));
    ASSERT_STREQ("Point__Drop__drop", mangle_method_symbol("Point", "Drop", "drop"));
}

// A leading `_` keeps its third underscore, so `_reset` and `reset` stay apart.
UTEST(mangle_method_symbol, a_leading_underscore_is_kept)
{
    ASSERT_STREQ("Point___reset", mangle_method_symbol("Point", NULL, "_reset"));
    ASSERT_STRNE(mangle_method_symbol("Point", NULL, "reset"),
                 mangle_method_symbol("Point", NULL, "_reset"));
}

UTEST(mangle_method_symbol, has_no_length_limit)
{
    char long_name[5001];
    memset(long_name, 'a', 5000);
    long_name[5000] = '\0';
    char *s = mangle_method_symbol(long_name, NULL, "m");
    ASSERT_EQ((size_t)5003, strlen(s));
    ASSERT_STREQ("__m", s + 5000);
}

// The name a declaration registers, used as is by every lookup: no merging.
UTEST(mangle_module_symbol, joins_without_merging)
{
    ASSERT_STREQ("util__parse", mangle_module_symbol("util", "parse"));
    ASSERT_STREQ("util____internal", mangle_module_symbol("util", "__internal"));
}

UTEST(mangle_variant_symbol, joins_the_enum_c_name_and_the_variant)
{
    ASSERT_STREQ("Color__Red", mangle_variant_symbol("Color", "Red"));
    ASSERT_STREQ("app_color__Red", mangle_variant_symbol("app_color", "Red"));
    ASSERT_STREQ("E__V", mangle_variant_symbol("E__", "__V"));
}

UTEST(sanitize_mangled_name, makes_a_type_fit_a_c_name)
{
    ASSERT_STREQ("intPtr", sanitize_mangled_name("int*"));
    ASSERT_STREQ("Point", sanitize_mangled_name("struct Point"));
    ASSERT_STREQ("Vec__int", sanitize_mangled_name("Vec<int>"));
}

// The name instantiate_generic registers: each argument sanitized on its own.
UTEST(mangle_generic_name, joins_the_sanitized_arguments)
{
    char *one[] = {"Foo*"};
    ASSERT_STREQ("Vec__FooPtr", mangle_generic_name("Vec", one, 1));
    char *two[] = {"K", "V"};
    ASSERT_STREQ("Map__K__V", mangle_generic_name("Map", two, 2));
    ASSERT_STREQ("Map__K__V", mangle_generic_name("Map_", two, 2));
}

// The argument list is split at the commas outside a nested <>, so it gives the same
// name as the arguments one by one (sanitizing "K, V" at once would give K____V).
UTEST(mangle_generic_name_from_arg_list, splits_at_the_top_level_commas)
{
    ASSERT_STREQ("Map__K__V", mangle_generic_name_from_arg_list("Map", "K, V"));
    ASSERT_STREQ("Map__K__Vec__A", mangle_generic_name_from_arg_list("Map", "K, Vec<A>"));
    ASSERT_STREQ("Vec__int", mangle_generic_name_from_arg_list("Vec", " int "));
}

// The name codegen defines for a trait object's vtable, without merging underscores.
UTEST(mangle_vtable_symbol, struct_trait_and_suffix)
{
    ASSERT_STREQ("Circle__Shape__VTable", mangle_vtable_symbol("Circle", "Shape"));
}
