# Zen C Symbol Mangling

This document describes how Zen C maps user-level names (functions, structs,
methods, generics, traits) to the C symbols emitted by the code generator.
It is meant as a guide for contributors and for anyone reading generated C or
debugger/crash output.

## Overview

Zen C symbols are built from `__`-separated segments. The scheme is designed to
be **readable** (the segments mirror the source) and **injective** (distinct
source declarations map to distinct symbols).

| Construct | Symbol | Example |
|:---|:---|:---|
| Global function | `Name` | `fib` |
| Module-qualified function | `Module__Name` | `math__sqrt` |
| Struct method | `Struct__Method` | `String__free` |
| Trait method | `Struct__Trait__Method` | `Vec__Drop__drop` |
| Generic instance | `Base__TypeArgs` | `Vec__int32_t` |
| Generic method | `Base__TypeArgs__Method` | `Vec__int32_t__push` |
| Slice | `Slice__Type` | `Slice__int32_t` |
| Tuple | `Tuple__Type1__Type2` | `Tuple__int32_t__double` |
| Enum | `Enum` | `Color` |
| Enum variant | `Enum__Variant` (tag: `Enum__Variant_Tag`) | `Color__Red`, `Color__Red_Tag` |
| Trait object vtable type | `Trait_VTable` (one underscore) | `Show_VTable` |
| Vtable of a trait for a struct | `Struct__Trait__VTable` | `Point__Show__VTable` |
| Drop glue | `Type__Drop__glue` | `Vec__int32_t__Drop__glue` |

Type arguments appear by their C names (`int` is `int32_t`), and a pointer
argument is spelled with `Ptr` (`Vec<Foo*>` is `Vec__FooPtr`).

A slice is an instance of the generic `Slice`, so it follows the generic rule.
For compatibility with 0.5.1, the 0.5.x releases also accept the old
single-underscore spelling in source (`Slice_char` for `Slice<char>`), under
`FF_COMPATIBILITY_0_5` (see `src/feature_flags.h`).

## The `__` separator

`__` separates segments. Because `__` can also appear inside a single
identifier, the following rule keeps the scheme injective:

> **A leading underscore of a method name is preserved.** When a method is
> named `_foo`, the emitted symbol keeps the resulting three-underscore run
> (`Struct___foo`), which is distinct from the plain `Struct__foo` of a method
> named `foo`.

Without this rule, `merge_underscores` would collapse `Struct___foo` to
`Struct__foo` and the two methods would collide.

## Underscore collapsing (`merge_underscores`)

Joining segments with `__` gives longer runs when a segment begins or ends
with underscores. `merge_underscores` normalizes them:

* A run of **4 or more** underscores collapses to **2** (`____` -> `__`).
* A run of **exactly 3** underscores is **preserved** — it encodes the `__`
  separator plus an identifier that begins with `_` (see above), so collapsing
  it would break injectivity.

## `link_name` / `extern` and C interop

Names controlled by the user that must match real C symbols are **not**
mangled:

* Declarations with `@link_name("c_name")`, which are emitted as `c_name`.
  (`@link("...")` is a different attribute: it names a library to link.)
* `extern` declarations (which reference symbols directly).

These are emitted verbatim so that Zen C code can call into C libraries and be
called from C.

## Where mangling happens

The C names of Zen C symbols are built in `src/utils/mangle.c` (declared in
`src/utils/mangle.h`), whether the parser, the typechecker or codegen needs them:

| Function | Symbol | Rule |
|---|---|---|
| `mangle_method_symbol(type, trait, method)` | `Type__method`, `Type__Trait__method` | merges runs of four or more `_`, except when the method starts with `_` |
| `mangle_module_symbol(prefix, name)` | `prefix__name` | no merging: the name the declaration registers |
| `mangle_variant_symbol(enum_c_name, variant)` | `Enum__Variant` (tag: `Enum__Variant_Tag`) | merges; `enum_c_name` is the enum's `link_name` if it has one |
| `mangle_generic_name(base, args, count)` | `Base__Arg1__Arg2` | each argument through `sanitize_mangled_name` (`int*` -> `intPtr`), trailing `_` of the base dropped, no merging |
| `mangle_generic_name_from_arg_list(base, "K, V")` | same | splits the list at its top-level commas first |
| `mangle_vtable_symbol(type, trait)` | `Type__Trait__VTable` | no merging: the name codegen defines |

If you add a new place that builds one of these names, call the function instead
of formatting `"%s__%s"`, so a lookup always builds the name the declaration
registered. The keys of `match` patterns (`Enum__Variant`, spelled from the
source path) are not C names and do not go through these functions.

Three names are still built outside `mangle.c`:

* The struct of a tuple, `Tuple__...`: in `src/ast/ast.c`, from the element
  types through `sanitize_mangled_name`.
* The drop glue, `Type__Drop__glue`: a name of the compiler's own, written by
  codegen where it defines and calls the glue.
* The vtable type of a trait object, `Trait_VTable`: written by codegen
  (`codegen_decl_struct.c`, `codegen_decl_emit.c`). Unlike the other names it
  has a single underscore.
