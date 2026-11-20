# Changelog

The most significant changes of each release. Every change is listed in the [full changelog](https://zenc-lang-ce.github.io/docs/changelog/).

## 0.5.2 (WIP)
Mainly a bug-fix release, with some improvements and new features.

### Changes
- `zc build` and `zc run` run the same typechecker as `zc check`. What only `zc check` reported (a type mismatch, an undefined name, a pointer that escapes its scope, a `match` that is not exhaustive...) is a warning in a build, with the message of the error and a note that `zc check` rejects it and a build will from 0.6 (also in the editor, and as `error_from` and `check_rejects` with `--json`), and an error with `--misra`; the backend no longer reports those warnings a second time, and `zc check` reports a string literal as the arm of a `match` statement ("a string on its own no longer prints"), which only the build did.
- Unreachable code is a warning (`-Wno-logic` turns it off), not a `zc check` error, and an error under `--misra` (Rule 2.1). It is also found after an `if`/`else` or a `match` whose branches all return and after a `loop` without a `break`; code after a label is reachable.
- A lambda parameter without a type where nothing at the lambda gives it one (`let add = (x, y) => x + y;`) is a warning, with a note that it will be an error from 0.6.
- Calling `print`, `println`, `eprint` or `eprintln` without importing `std/io.zc` is an undefined function, as for any other name.
- `-Werror` and `--warn-errors` make the compiler's own warnings errors; `--warn-errors` did nothing, and `-Werror` only reached the C compiler.
- A type is named as the program writes it, in diagnostics, hover, `zc-doc` and the REPL (`Vec<i32>`, `(i32, bool)`, `geo::Vector`); a literal is an `i32` (`int` is `i32`) and one with the `u` suffix a `u32`. [#273](https://github.com/zenc-lang-ce/zenc/pull/273), [#274](https://github.com/zenc-lang-ce/zenc/pull/274)
- An undefined function or type gets the closest name as a hint ("Did you mean 'compute'?"), and the hint for a type proposes a generic one too (`Vec`).

### Fixes
- `@derive` generates complete methods for types with many fields, long field names, or many enum variants.
- Types in `zc check`: a literal without a suffix takes the type its context expects everywhere (`return 0;` for an `f64`, `Point{x: 1}` with an `f32` field, `let x: f64 = -1;`); an integer converts to a float without a cast, with a warning when the float cannot hold every value; a `string` is the `char*` it is, a string literal fits a `char[N]`, an array is a pointer in pointer arithmetic and a pointer compares with a `void*`; the value of a `let` or a `def` is checked against its type; an opaque alias follows the parser's rule; a shift is judged by the width of what is shifted; an enum's `.tag` is an integer; a float divided by zero is a warning; a pointer or a string compares with the literal 0 (null), and a comparison in an f-string is checked as any other.
- `zc check` accepts what `zc build` compiles: SIMD and `@vector` types, `comptime` blocks (run by the interpreter), a default value for a trait parameter, calling a field that holds a function pointer, a closure that changes its copy of a captured variable, and a `match` that names every variant as covering every value.
- `zc` reports what reached the C compiler: an undefined variable (with "Did you mean"), a method the type does not have [#314](https://github.com/zenc-lang-ce/zenc/pull/314), a call of a field, a variable or a value that is not a function (`b.len()` with `len: int`) [#358](https://github.com/zenc-lang-ce/zenc/issues/358), the type a method mixed in with `use` returns, a function whose last statement is an expression and not a `return`, a compound assignment to a `const`, `const x = 5;` in a function [#309](https://github.com/zenc-lang-ce/zenc/pull/309), a default value that names a parameter or a local of the caller, moving a value bound by a `match` arm, a variant written without its enum, and a struct with the name of an enum.
- Escape analysis: no false escape for a pointer cursor moved along a text, a recursive call that returns its argument, a `test` block or a cast of `malloc`; a pointer to a local is still found when it escapes through a struct, an array, a cast or a later assignment.
- Lambdas: parameters without a type take them from the first call, which checks the body with them; a returned lambda takes the function's return type.
- `match`: negative literals and ranges as patterns (`-1 =>`, `-5..-2 =>`); a variant without its enum (`Red =>`) is the one of the matched value's enum; a `ref` binding points to the value through a pointer (`match *p`) [#352](https://github.com/zenc-lang-ce/zenc/pull/352) and into a copy for a field of a temporary (`match make().held`) [#353](https://github.com/zenc-lang-ce/zenc/issues/353); a pointer subject works.
- Enums: `Color.Green` and `Enum::Variant()` for a variant without a payload; a payload-less variant of a generic enum is a value of its enum; a variable hides an enum or a variant with the same name.
- Drop: enums, tuples, arrays, closures and `@link_name` structs are dropped; assigning drops the old value; a struct is `Drop` whatever the order of its `impl`; a tuple that holds a `Drop` value moves.
- `panic(...)` and a call to a function marked `@noreturn` end the flow, as `return` does: a function with a value may end in one without a `return` after it (also as the last arm of a `match`), the code after it is unreachable, and the C compiler is told too (the `@noreturn` goes in the prototype as well). A `panic` in a `defer` does not end the flow where the `defer` is written.
- `?` runs the defers of the function, and the drops of its locals and parameters, when it returns early, as `return` does; under MISRA it leaves through the single exit [#367](https://github.com/zenc-lang-ce/zenc/issues/367).
- Default values: a default value can name what is declared further down: a variant without `()` (`Rate::Normal`), a generic struct, and, in a module imported with an alias, a variant or a static method.
- Constants: an array size and the value of a `def` can name a constant declared further down (a `def`, a global `let`, a `def` that names another, an alias of an array), also in a module imported with an alias; an integer `def` is written in the C as its value, so a `def` that names one declared after it compiles.
- Shadowing: the initializer of a `let` and the bounds and step of a `for` read the variable they shadow (`let x = x + 1;`, `for i in 0..i`), as `zc check` does [#359](https://github.com/zenc-lang-ce/zenc/pull/359), [#365](https://github.com/zenc-lang-ce/zenc/pull/365); a `let` that hides a lambda's parameter [#355](https://github.com/zenc-lang-ce/zenc/issues/355) or, with `--cpp`, the variable of a `for` [#356](https://github.com/zenc-lang-ce/zenc/issues/356) compiles; a local declared twice in one block is an error, from `zc check` too [#268](https://github.com/zenc-lang-ce/zenc/issues/268); `let _ = <value>` discards the value, dropping it at once, and a block may have several [#357](https://github.com/zenc-lang-ce/zenc/issues/357); "shadows a previous declaration" is reported once for each name, where it is declared (a parameter at the parameter, also when what it hides is declared further down), and for a `def` in a block too.
- Methods and fields: `.` reaches through every pointer level [#264](https://github.com/zenc-lang-ce/zenc/pull/264); `?.` on a pointer to a pointer says to write `(*pp)?.value` [#279](https://github.com/zenc-lang-ce/zenc/pull/279); a method of a struct mixed in with `use` can be called on a temporary [#323](https://github.com/zenc-lang-ce/zenc/pull/323); `use` takes the fields of a struct declared further down, `@derive` included, and two structs that use each other are an error; an impl written before the type it is for works, a trait impl for a generic type written without its parameter (`impl Show for Cell`) included, also in a module imported with an alias, and `@derive(Clone)` clones a field whose `clone` is declared further down or comes from `impl Clone for X`; `module::f()` and `Type::f()` no longer call another type's function whose name starts the same [#191](https://github.com/zenc-lang-ce/zenc/pull/191); `self` is a pointer to the receiver in `zc check` and in the build alike: `return self;` compiles in a method that returns a pointer, and `return &self;` returns the receiver with a `defer` pending too, with a warning (an error from 0.6) [#387](https://github.com/zenc-lang-ce/zenc/issues/387).
- Generated C: a cast to a C type writes that type (`(long)x`); an f-string prints an `i8` as a number; a pointer to a struct becomes a trait object from any expression; assigning a string to a `char[N]` copies only the string; an index with `get`/`set` is decided once; `a[i..j]` slices an array, and `let b = a;` copies a fixed-size one; a global without a value is zeroed; a name that is a C or C++ keyword is renamed; an alias of an array type [#281](https://github.com/zenc-lang-ce/zenc/pull/281), `sizeof` of a tuple [#164](https://github.com/zenc-lang-ce/zenc/pull/164) and `?` on a type whose name ends in `__` [#320](https://github.com/zenc-lang-ce/zenc/pull/320) compile; `int` and `i32` share one generic instance; an `asm` operand of a 64-bit type takes a 64-bit register; `@derive(FromJson, ToJson)` handles integer fields of any width; a variable or a tuple element that takes its type from `true` or `false` is a `bool` the program can change, not a `const bool` (`true` and `false` are literals now); an array of function pointers, a function pointer that returns one and a pointer to an array compile wherever a value is declared (a field, a local, a parameter, a result, a capture, an enum payload, a trait method, a slice) and in `sizeof` [#325](https://github.com/zenc-lang-ce/zenc/issues/325).
- Modules: a module imported with an alias, one by one or `as *` uses its own functions and types; in a module imported with an alias, an alias or a generic struct named before it is declared is that one, and a type of a module imported further down in the file is that module's; the `//>` directives of an imported module apply [#322](https://github.com/zenc-lang-ce/zenc/pull/322); `import "x.h"` reads the names the header declares; `@cfg(macos)` is defined on macOS.
- C++ (`--cpp`): an f-string interpolation is evaluated once; a `@ctype` that is a template is declared as written [#321](https://github.com/zenc-lang-ce/zenc/pull/321).
- Crashes: `@derive(Clone)` of a struct with a field of a generic type that has `impl Clone for X<T>`, `impl<T> A for Cell<T>` followed by `impl B for Cell` [#280](https://github.com/zenc-lang-ce/zenc/pull/280), `include <stdio.h` without its `>` [#282](https://github.com/zenc-lang-ce/zenc/pull/282), and lists of the parser that overflowed their fixed size (the fields of a destructuring, `use`, the generics of a trait, captures), and the language server reading a C header after a hover over a C name.
- `comptime` computes `<< >> & | ^ ~`, and integer operations as constant folding does.
- A name of a module no longer "shadows" a declaration of the program that imports it. Diagnostics point at the place: an f-string interpolation, a struct field, a field of a struct literal, the call of an impure function. "Use of moved value" shows where the value was moved; the printf format warning (W701) runs; an integer constant is checked against the range of its target. A function, struct or enum of a module imported with an alias is named as the module writes it (`helper`, not `geo__helper`) in the messages about it, in the language server's outline and hover, in the REPL and in `zc-doc` A method is named as written in messages (`push`, not `Vec__int32_t__push`).
- MISRA: an `embed` is not a raw C block (Rule Zen 1.1); Rule 2.4 does not flag an enum the program uses; `assert` and `test` are Rule 21.6; Rule 10.4 is not reported for a literal that fits its unsigned target.
- CLI: `-I`, `-O` and other options read their value; `-g` passes `-g` to the C compiler; options can come before the command. `#line` is written where the file changes; the C is written in binary mode on Windows.
- Language server: an open document shows what `zc build` reports after parsing it, the typechecker's errors and warnings among them; hover, go to definition, references and rename follow the declaration a name resolves to, and work on types, fields, enums and their variants, generic functions and parameters, `for` variables and members reached through `.`; completion offers what each file declares now, what fits where a type, a field or a method goes, and what follows `::` or a trait value; a type the file does not see is warned about with the file that declares it, and code actions import it or implement a trait's missing methods; semantic highlighting covers the whole file; open documents are analyzed again when a file they import changes; every request gets an answer.
- Tools: `zc-doc` titles a declaration by the name the program uses and writes the parameters of methods [#275](https://github.com/zenc-lang-ce/zenc/pull/275); `zc-format` builds without the language server; Backspace and Escape work in the REPL's reverse search.

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
