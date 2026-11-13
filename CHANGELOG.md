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
  - A variant without a payload of a generic enum (`Maybe<int>::Nothing`) is a value of its enum, not a function.
- Drop:
  - Dropped now: enums with `Drop` or a payload that needs it, tuples, arrays, closures (local, in a field or captured) and `@link_name` structs.
  - Assigning to a variable with `Drop` drops the old value in C too, and assigning a closure drops the old closure.
  - A struct is `Drop` whatever the order of its `impl`; a local that will be dropped gets no unused-variable warning.
  - Tuples that hold a `Drop` value move, and a `const` parameter can be dropped and moved out through a mutable pointer.
- Arrays and slices: `a[i..j]` slices an array; `let b = a;` and `b = a;` copy a fixed-size array, and move one whose elements have `Drop`.
- A default value that names a parameter of its function, or a local of the caller, is an error instead of reading the caller's variable.
- A global without a value is zeroed in the C, so `ld` no longer warns about its alignment on arm64 macOS.
- Names that are C or C++ keywords are written under another name.
- `match` works through a pointer subject.
- CLI: `-I`, `-O` and other options read their value correctly; `-g` passes `-g` to the C compiler; options can come before the command (`zc -g build app.zc`).
- C headers: `import "x.h"` reads the names the header declares, and `zc check` accepts a struct literal of a type from a header.
- With `--cpp`, an f-string interpolation is evaluated once.
- `int` and `i32` share one generic instance.
- A type alias is emitted once; `@fields` names a `@link_name` struct by its C name.
- Casts are decided by whether the name is a type, as in C.
- Typechecker: one exhaustiveness check for `match`; `return` in lambdas, `Self` literals and casts to `@link_name` structs; a struct's address only for a pointer to the same struct; moving a value bound by a `match` arm is an error.
- Diagnostics:
  - `println "..."` is reported at the `match` arm.
  - `extern enum` and `extern const` say they are not declarations.
  - A token that does not start a struct field is reported.
  - A variable used as the message of `assert` or `expect` counts as used.
  - Implicit `std` imports report the module path.
  - An integer constant is checked against the range of its target: `take(20)` for a `u8` no longer warns, and `let big: u8 = 300;` does.
  - An error inside an f-string interpolation points at the expression and prints its line.
  - A MISRA diagnostic about a struct field points at the field.
  - A diagnostic about a field of a struct literal (a type mismatch, a pointer that escapes, a MISRA rule) points at that field, not at the name of the struct.
  - "Use of moved value" shows where the value was moved, with its own line of code and `^`, or says that a loop moved it on an earlier iteration. With `--json` the place comes in a `note` field.
  - MISRA Rule 2.4 does not flag an enum the program uses; `assert` and `test` blocks are reported under Rule 21.6 instead of C that does not compile.
- No limit on the length of four lists the compiler wrote past the end of, crashing it or corrupting what it compiled: the fields of `let Point{...} = p` (16), the `use` of a struct (8), the generic parameters of a trait (8) and the captures of `fn[a, b, ...]` (32).
- `#line` is emitted when the file changes on the same line, and again when a newline was written since the last one; a `for` loop restores the lexer after its lookahead.
- The C is written in binary mode on Windows.
- Language server:
  - Completes what the program declares by the name it declares, `__` or not.
  - Hover and go to definition work on type names, inside f-string interpolations, and on both parts of a path: the type and the function of `Vec<int>::new()`.
  - Go to definition reaches the standard library from a workspace that does not contain it.
  - A definition in another file is looked up in that file, not at the same line of the open one.
  - Go to definition, hover, references and rename follow the declaration a name resolves to, not every declaration with that name: renaming a local or a parameter edits only its own uses, and a type name leads to the type its file declares.
  - Every request gets an answer: on a file the server does not know, or without its parameters, it is `null` instead of none, which left the client waiting.
  - The variants offered after `value.` are enum members, not values, and a `def` is highlighted as readonly.
  - Semantic highlighting covers the whole file: every declaration and method, and what is inside the bodies of `if`, `while`, `for` and `match`, not only the first declaration. Completion offers the locals of every kind of body, such as a `match` arm.
  - Keywords are the language's, from one table that the lexer reads too: `for` and `in` are highlighted as keywords, and `print`, `println` and `printf` are functions. Completion offers each keyword where it starts a construct, and C functions such as `malloc` as functions, not keywords; words that are not Zen C (`module`, `unsafe`, `typedef`) are not offered.
  - A file whose path has spaces or other characters that a URI encodes: the server answers its URI encoded, and closing it goes back to the file on disk (it kept the editor's text).
  - Hover describes every primitive type by any of its names (`int32`, `byte` and `c_long` too), with its C spelling.
  - The workspace index leaves out what the project's .gitignore or `exclude` in zenc.server.json leave out, and nothing else: it no longer skips `node_modules`, `obj`, `vhdl` and `vivado` by their names.
  - `initialize` gives the compiler's version as the server's, instead of 1.0.0.
  - Completion no longer offers the constructs the language removed: `repeat`, `unless`, `guard`, `do`, `async`, `await` and `autofree`.
  - Completion and signature help read what each file declares now: a function, type or method that was deleted is no longer offered. They offer generic structs and functions and traits too, and the std modules that a `test` block imports by itself (std/test.zc) are indexed, so go to definition reaches them.
  - Hover and go to definition work on the fields and methods reached through `.` (`self.count`, `self.is_full()`, `list.push(x)`), and on a field's declaration.
  - The variable of a `for` has hover and go to definition, with the type of the elements it walks (`for entry in list`, `for i, x in v`, `for i in 0..n`), and the hover of a `let` gives its type.
  - Enum variants have hover and go to definition where they are declared, built (`Value::Hour(3)`, `Value::Nothing`) and matched (`Value::Hour(h) =>`, `Hour(h) =>`, `Some(x) =>` on an Option); the hover gives the enum and the payload: `Value::Hour(i32)`.
  - Generic functions (`fn f<T>`) are indexed: their name, parameters and body have hover and go to definition, and so have their calls (`f<int>(1)`, `f(1)`).
  - The field names of a struct literal (`Point { x: 1, y: 2 }`, `Box<int> { val: 3 }`) have hover and go to definition.
  - Generic parameters (`T` of `struct Box<T>`, `impl<T>`, `fn max<T>`, a trait or an enum) have hover and go to definition: each `T` leads to the declaration in scope where it is written.
  - Completion after `.` reads the members of the type the value has, whatever qualifies it: `h.` on a `let h: const Header` or a `const Header*`, a field of a C header through a pointer (`font.recs.`), and the methods of an `impl int` on an `int`.
  - A type name that names no type (`origin: Pont`) is warned about, as `zc build` does, with the closest name as a suggestion. The types that the C headers of the file declare are not warned about.
  - Completion where a type goes offers the types and only them: the primitives, and the structs, enums, traits and aliases. That is the type of a field, a parameter, a `let` or a `def`, the return type, the right side of `alias Word =`, a variant's payload, and inside `Vec<...>` and `[...]`. Before, a field, a parameter, a return type or an alias offered no primitive. Aliases are offered in the rest of the completion too.
  - Completion where a field's name goes in a struct literal (after `Point {` or a `,`) offers the fields of that struct that the literal does not give yet, written as `name: `.
  - Completion after `.` on a value of a trait (`h: Header`) offers the trait's methods. Before it offered nothing, since it looked for the fields of a struct of a C header with that name.
  - Completion where a method goes in an `impl Trait for Type` offers the methods of the trait that the impl does not define yet, and inserts each with the signature written in the trait and an empty body (without `fn` when it is typed already).
  - A code action, "Implement the missing methods of Trait", in the header or the body of an `impl Trait for Type`, writes the methods of the trait that the impl does not define, with the signature written in the trait and an empty body, after the last member of the impl.
  - On a type name that the file does not see, a code action imports a file that declares it, one action per file: "Import Header from \"std/net/http.zc\"". The files of the workspace go before the standard library, the nearest first. The import goes after the file's last one and is written as the parser resolves it: from `std/` for the standard library, relative to the file otherwise.
  - A type that only files the document does not import declare is warned about, as `zc build` warns about it, with the file to import: "Type 'Shape' is declared in \"a/shapes.zc\", which this file does not import". Before there was no warning, since the server parses the whole workspace with one context, which knows that type.
  - Completion of a type that the file does not see adds the import of a file that declares it: one item for each such file, which the item names on its right (`a/shapes.zc`).
  - A type name goes to the declaration the file sees: its own or one of a file it imports. The server parses the whole workspace with one context, so a type of the same name in a file it does not import (`Header` of `std/net/http.zc`) could win: hover, go to definition and the members after `.` showed that one.
- `zc-format` no longer needs the language server: it is built with `ZC_LSP=0` too.
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
