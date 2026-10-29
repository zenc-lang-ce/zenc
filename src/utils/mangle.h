// SPDX-License-Identifier: MIT
#ifndef UTILS_MANGLE_H
#ifndef ZC_ALLOW_INTERNAL
#error "utils/mangle.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define UTILS_MANGLE_H

/**
 * @file utils/mangle.h
 * @brief The C names of Zen C symbols (see MANGLING.md).
 *
 * The parser, the typechecker and codegen all build these names, so they live in one place
 * and nobody re-implements them with "%s__%s".
 */

/**
 * @brief Collapses runs of four or more underscores into a double underscore.
 *        A run of exactly three is preserved: it encodes the "__" separator
 *        plus an identifier that begins with "_" (e.g. `Struct::_method`), so
 *        collapsing it would make distinct symbols collide.
 */
char *merge_underscores(const char *name);

/**
 * @brief The C name of a method: `Struct__method`, or `Struct__Trait__method` for the
 *        method of a trait impl (trait_name may be NULL).
 */
char *mangle_method_symbol(const char *struct_name, const char *trait_name,
                           const char *method_name);

/**
 * @brief The C name of a symbol declared in a module: `prefix__name`, without merging
 *        underscores (the name the declaration registers).
 */
char *mangle_module_symbol(const char *module_prefix, const char *name);

/**
 * @brief The C name of an enum variant: `Enum__Variant` (its tag adds `_Tag`).
 *        enum_c_name is the enum's name in C: its link_name, if it has one.
 */
char *mangle_variant_symbol(const char *enum_c_name, const char *variant_name);

/**
 * @brief A type written as it can appear inside a C name: `*` becomes `Ptr`, `<`, `,` and
 *        spaces become `__`, `>` and `&` are dropped, a leading `struct ` is skipped.
 */
char *sanitize_mangled_name(const char *s);

/**
 * @brief The C name of an instance of a generic: `Base__Arg1__Arg2`, each argument
 *        sanitized (Vec<Foo*> is Vec__FooPtr). The name instantiate_generic registers.
 */
char *mangle_generic_name(const char *base, char **args, int count);

/**
 * @brief mangle_generic_name from the argument list as written between `<` and `>`
 *        ("K, Vec<A, B>" is two arguments).
 */
char *mangle_generic_name_from_arg_list(const char *base, const char *arg_list);

/**
 * @brief The C name of the vtable of a trait for a struct: `Struct__Trait__VTable`.
 */
char *mangle_vtable_symbol(const char *struct_name, const char *trait_name);

#endif // UTILS_MANGLE_H
