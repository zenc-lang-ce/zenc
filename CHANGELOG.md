# Changelog

The most significant changes of each release. Every change is listed in the [full changelog](https://docs.zenc-lang.org/changelog/).

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
- 19 bugs in diagnostics and `zc check`: false errors, missing errors and wrong locations;
- 32 fixes in tools and platforms: the LSP, the REPL, the CLI, installation, building the compiler, Windows and macOS.

See the [full changelog](https://docs.zenc-lang.org/changelog/) for every fix, addition and change.
