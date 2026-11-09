# Changelog

The most significant changes of each release. Every change is listed in the [full changelog](https://zenc-lang-ce.github.io/docs/changelog/).

## 0.6 (WIP)
This new release brings breaking changes to the language:

- `Slice_char` and the other `Slice_X` names are gone: write `Slice<char>` (or `[char]`).
- A slice no longer has a `cap` field: it is `{ data, len }`. Code that reads `.cap`, or that builds a slice in C with three fields, must change.
- Returning an array literal as a slice (`fn f() -> [float] { return [1.0, 2.0]; }`) is an error, as it already is for a local array: its elements live on the function's stack. Return a fixed-size array (`float[2]`) or a `Vec<T>`. Before, the literal was copied to the heap and never freed.
- `Enum_Variant` and `Enum_Variant_Tag` (`Result_Ok`, `Result_Ok_Tag`) no longer stand for `Enum::Variant` and its tag: write `Result::Ok`.

## 0.5.2
This is mainly a bug-fix release but it also brings some improvements and new features.

### Fixes
- Enums:
  - `Color.Green` works for a variant without a payload.
  - A variant written without its enum (`let s: Shape = Dot;`) is an error that says `Shape::Dot`, instead of C that does not compile.
  - A variable hides an enum or a variant with the same name; `Color_max` is no longer read as `Color::max`.
  - A variant as a callee gets its `()`; `Enum::Variant()` is accepted for a variant without a payload.
- Drop:
  - Dropped now: enums with `Drop` or a payload that needs it, tuples, arrays, closures (local, in a field or captured) and `@link_name` structs.
  - Assigning to a variable with `Drop` drops the old value in C too.
  - A struct is `Drop` whatever the order of its `impl`; a local that will be dropped gets no unused-variable warning.
  - Tuples that hold a `Drop` value move, and a `const` parameter can be dropped and moved out through a mutable pointer.
- Slices: `a[i..j]` slices an array.
- Names that are C or C++ keywords are written under another name.
- `match` works through a pointer subject.
- `-I`, `-O` and other options read their value correctly.
- With `--cpp`, an f-string interpolation is evaluated once.
- `int` and `i32` share one generic instance.
- A type alias is emitted once; `@fields` names a `@link_name` struct by its C name.
- Casts are decided by whether the name is a type, as in C.
- Typechecker: one exhaustiveness check for `match`; `return` in lambdas, `Self` literals and casts to `@link_name` structs; a struct's address only for a pointer to the same struct.
- Diagnostics:
  - `println "..."` is reported at the `match` arm.
  - `extern enum` and `extern const` say they are not declarations.
  - A token that does not start a struct field is reported.
  - A variable used as the message of `assert` or `expect` counts as used.
  - Implicit `std` imports report the module path.
  - MISRA Rule 2.4 does not flag an enum the program uses.
- `#line` is emitted when the file changes on the same line; a `for` loop restores the lexer after its lookahead.
- The C is written in binary mode on Windows.
- Language server: completes what the program declares by the name it declares, `__` or not.
- REPL: Backspace and Escape work in reverse search.

### Added
- `--prune-unused`: leaves unreached functions out of the generated C, for faster builds and smaller executables. It reports how many it pruned.

### Flagged as deprecated
Still accepted in 0.5.2 and removed in 0.6:

- `Slice_char` and the other `Slice_X` names: use `Slice<char>`.
- The `cap` field of a slice.
- Returning an array literal as a slice.
- `Enum_Variant` and `Enum_Variant_Tag` (`Result_Ok`): use `Enum::Variant`.

## 0.5.1

A bug-fix release.

- Release downloads: `zc.com` finds its standard library in any directory, `zc-boot.com` installs the `zc.com` of this release, the tools come as portable executables too, and there are native binaries for Linux, macOS and Windows. All of them need a C compiler on the system.
- The language server no longer crashes when two files declare the same enum, one with a payload on a variant and one without.

See the [full changelog](https://zenc-lang-ce.github.io/docs/changelog/#0-5-1) for every change.

## 0.5.0

### Language changes
- Only `f"..."` interpolates, and `print`, `println`, `eprint` and `eprintln` are functions of `std/io.zc` that take one string: `println(f"total: {n}")` (RFC 0001). The old forms are compile errors that point to the new ones.
- Lambdas and match arms take `=>`; `->` is member access through a pointer and the return type arrow, so `p->value` reads the field.
- Removed: `autofree` (use `defer free(x)` or `Drop`), `async`/`await` (use `std/thread.zc`), `unless`/`guard`, `repeat`/`do … while`, and the `?` input statements (use `readln(...)`).
- `x[i]` calls the struct's `get` method, and only `get`; `Vec::index` is gone.
- A failed `assert` outside a `test` block stops the program with exit code 1.

### Highlights
- Security: a stack buffer overflow when mangling long generic names ([GHSA-rv74-w6q7-h8xr](https://github.com/Zuhaitz-dev/zenc-lang/security/advisories/GHSA-rv74-w6q7-h8xr)), interpolated text read as a `printf` format, and `pkg-config` directives that went through a shell.
- Function bodies can use anything declared further down the file, imports included.
- Escape analysis in `zc check`: returning or storing the address of a local is an error.
- MISRA C:2012: `--misra` (or `--profile misra-c2012`) checks the rules and builds MISRA-shaped C.
- Backends: `--backend c|cpp|cuda|objc|json|ast-dump|lisp|dot`.
- Testing: `expect`, `std/test.zc`, and each `test` block reports `OK` or `FAIL`; failed checks, panics and out-of-bounds indexes report `file:line:`.
- The LSP, REPL, documentation generator and formatter are separate programs: `zc-lsp`, `zc-repl`, `zc-doc` and `zc-format`. The LSP indexes large workspaces several times faster and shows its progress.
- Faster compilation: a program of 10,000 lines transpiles in about 6 s instead of 33–38 s.
- `zc help`, `zc run FILE ARGS...`, and clear errors instead of broken C for many invalid programs.
- Standard library additions in `Option`, `Result`, `String`, `VecIter`, `BigInt`, `BigFloat` and `std/mem.zc`.

### Fixed
More than 140 fixes:
- 59 bugs that kept valid code from compiling, with an error from `zc` or invalid C: generics, closures, traits, modules, enums, `match`, `@derive`, operators and the generated C;
- 19 bugs where a program compiled but did the wrong thing: `@derive(Eq)` on enums with payloads, `continue` in indexed loops, interpolations lost with `-O2`, leaks and double frees with `Drop`, `readln` into a `string`…;
- 15 compiler crashes and memory errors, three of them security issues;
- 21 bugs in diagnostics and `zc check`: false errors, missing errors and wrong locations;
- 33 fixes in tools and platforms: the LSP, the REPL, the CLI, installation, building the compiler, Windows and macOS.

See the [full changelog](https://zenc-lang-ce.github.io/docs/changelog/) for every fix, addition and change.
