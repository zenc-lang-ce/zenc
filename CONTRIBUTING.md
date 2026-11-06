# Contributing to Zen C

Thanks for contributing! This file covers the repository's expectations,
especially for changes to the compiler itself (`src/`).

Formatting and naming are in [STYLE.md](STYLE.md), the mangling scheme in
[MANGLING.md](MANGLING.md), and how tests are laid out in
[tests/DOCS.md](tests/DOCS.md). If you work with an AI agent, it follows
[AGENTS.md](AGENTS.md), which builds on this file.

## Architecture

The compiler runs in phases: the lexer, the parser (`src/parser/`), the
typechecker (`src/analysis/`) and the backends (`src/codegen/`: C, and the C++,
CUDA and Objective-C dialects; json, dot, ast-dump and lisp for tooling).

### How the phases run

- `zc check` runs the full typechecker (`check_program`).
- `zc build` and `zc run` also run the typechecker, in a reduced mode
  (`check_moves_only`). It walks the whole tree with the same `check_node`, and
  it reports move errors, control-flow errors, MISRA checks and warnings, but
  `tc_error` is muted. It also **rewrites the AST** before codegen: index
  operators become calls, implicit `&`/`*` nodes are inserted, `comptime`
  blocks are expanded, and so on.
- The language server only runs the parser (`parse_program`).

So:

- A check that reports only through `tc_error` does not stop a build.
- A change to `check_node`, or to anything it calls, can change the C a build
  emits.
- `zc check` and `zc build` can disagree in both directions. To verify a fix,
  run the program with `zc run`, not only `zc check`.

### Rules

The compiler grew with its semantics spread across the phases. These rules
describe where it is heading. New code moves toward them, never away from
them; code that does not follow them yet is not a precedent.

- **One responsibility per phase.** The parser builds the AST. The typechecker
  decides what the program means: types, moves, semantic diagnostics. The
  backends only write output. A new semantic check belongs in the
  typechecker, not in the parser or in codegen. If `zc build` must reject the
  program too, a `tc_error` is not enough today (see above): say so in the PR
  and agree there on where the check goes.
- **In the AST, a type is a `Type` with source names.** Only a backend writes
  C text (`size_t`, mangled names), when it emits. Do not store C type
  strings in AST nodes. The one exception is C written by the user
  (`@ctype`, `@crepr`).
- **Decide on kinds, not on names.** Compare `Type.kind` and `node->kind`,
  not type-name strings. `strcmp(type, "Vec") == 0` is wrong both ways: it
  takes a user type called `Vec` for the standard one, and it misses an alias.
- **The compiler does not know the standard library.** Do not hard-code std
  type, field or method names; go through traits and protocols. Some places
  still do it; do not add new ones.
- **One mechanism per concern.** Before writing a helper, look for the
  existing one. Examples: `mangle_method_symbol` for method symbols,
  `ast_children` to walk a node's children, `should_suppress_undef_warning`
  for calls to undeclared C functions. If you find two copies of the same
  rule, merge them.
- **Prefer the fix that simplifies the compiler.** Between fixes for the same
  bug, choose the one that leaves less code and fewer special cases: fix a
  wrong value where it is produced instead of patching every reader. A patch
  for a single case is a last resort.
- **Small functions, focused files.** Some functions are already hundreds of
  lines long. Do not make them longer: put what you add in a helper. Put a
  new file's code around one concern; do not add to catch-all modules such as
  `utils.c`.

## Compiler conventions

The compiler is intentionally written to be **coherent**: one rule per
concern, applied everywhere. When changing compiler code, follow these.

### Error handling

| Class | Macro | Behavior |
| :-- | :-- | :-- |
| Fatal / syntax | `zpanic_at(token, "…")` | Aborts (or delegates to the LSP handler in fault-tolerant mode). |
| Warning | `zwarn_at(token, "…")` | Non-fatal diagnostic. |
| Semantic (typechecker) | `tc_error(tc, token, "…")` | Recoverable type error. |
| Semantic (parser, recoverable) | `zerror_at(token, "…")` | Recoverable non-fatal error. |

- `"Expected …"` syntax errors use `zpanic_at`, consistently.
- Every error path that returns `NULL`/an error node must already have emitted
  a diagnostic.

### Symbol mangling

- The one canonical mangler is `mangle_method_symbol(base, trait, method)` in
  `src/utils/mangle.c`. Use it everywhere; never re-implement
  `%s__%s` + `merge_underscores` inline.
- A method beginning with `_` keeps its triple underscore (`Struct___method`)
  so it cannot collide with `Struct__method`.
- `merge_underscores` collapses runs of **four or more** underscores; a run of
  exactly three is preserved (it encodes `__` + a leading-`_` name).

### Memory management

- Use `xmalloc` / `xcalloc` / `xrealloc` / `xstrdup` (arena allocation);
  `zfree` is a no-op (arena memory is reclaimed all at once).
- `arena.h` macro-redirects bare `malloc` / `realloc` / `calloc` to the arena,
  so a bare `free()` on a redirected pointer is an **invalid free**. Frees on
  arena memory are `zfree` (no-op).
- Use the explicit `libc_malloc` / `libc_free` / `libc_realloc` escape hatches
  for short-lived buffers that should be heap-managed (e.g. `realpath` results,
  cJSON's allocator), and pair them correctly.

### Recursion safety

- Guard unbounded recursion with `RECURSION_GUARD` / `RECURSION_GUARD_TOKEN`.
- Recursive type/AST walkers that aren't parser-bounded carry a depth
  parameter (see `sync_type_linkage_depth`).

### Field naming

- Discriminant fields use `kind` (`ASTNode.kind`, `Token.kind`, `Type.kind`,
  `CValue.kind`, the literal union's `kind`). `field.type` is a type-name
  string, not a discriminant.
- Count fields use `<X>_count` (`arg_count`, `binding_count`,
  `capture_count`, `output_count`, `generic_param_count`).

### Dead code

- No consecutive duplicate `return` statements; no code after an unconditional
  return; no empty `if`/`while` bodies without an explanatory comment.
- Run `clang-format` (enforced by the CI Lint job) and the fuzzer before
  landing compiler changes.

## Development workflow

- `make` builds the compiler; `make test` runs the full suite.
- `make fuzz-libfuzzer-build` builds the libFuzzer target; the nightly
  `Scheduled Fuzzing` workflow runs it.
- Keep changes in small, focused commits, and one topic per PR. The website
  docs and the awesome-zenc examples live in separate repositories.

### Testing a change

- **Add a test** with every fix: the program that failed, in the folder of
  its area (see [tests/DOCS.md](tests/DOCS.md)). A typechecker fix also adds
  its program to `tests/compiler/check_clean/`, which `zc check` must accept
  with no errors (`make test-check`).
- **Run the suite:** `make test`.
- **Compare with `main`.** Build `main` apart, for example
  `git archive main | tar -x -C ../zc-ref && make -C ../zc-ref`, and compare:
  - `tests/scripts/compare_compilers.sh ../zc-ref/zc ./zc`: tests that pass
    with one binary and fail with the other. Exits 1 on any regression.
  - `scripts/measure_all.sh ../zc-ref/zc ./zc TAG`: the generated C and the
    diagnostics of every test, MISRA program and awesome-zenc example (with
    `../awesome-zenc` next to the checkout). For a change meant for `main`,
    the only differences should be the ones the fix intends.
  - `scripts/cpp_cmp.sh ../zc-ref/zc ./zc`: the same for the C++ of `--cpp`.
- **Typechecker changes:** `scripts/check_vs_run.sh [-v] ./zc` counts the
  tests that pass with `zc run` but that `zc check` rejects, grouped by error.
  It measures how far the typechecker is from being the single semantics: it
  must not go up.
- **Run the CI's Linux checks before pushing a change in C.** macOS's clang
  misses warnings that fail the CI (`-Wformat-truncation`,
  `-Wsuggest-attribute=const`, `-Wnull-dereference`, the static analyzer).
  `scripts/validate_gcc14.sh FILES...` runs the gcc-14 sanitizer and analyzer
  builds, the suite (also with `--cpp` and tcc) and the diagnostic output
  tests in a container, then formats FILES with the CI's clang-format. Build
  its image once with
  `docker build -t zc-validate -f scripts/validate.Dockerfile scripts`.
- **Formatting and lint:** `make format`, `make lint`, `make check-codebase`.
- The `.expected` files of `tests/compiler/diagnostics_output/` are generated
  on Linux (`UPDATE=1 tests/scripts/run_diagnostic_output_tests.sh`): on
  macOS the linker adds a warning line, so some of them always fail there.
- To show that a code path is dead before removing it, add temporary traces
  and run `scripts/trace_all.sh` (its header explains how).

## Versioning

Zen C follows [semantic versioning](https://semver.org) (`MAJOR.MINOR.PATCH`).

- **Patch** (`0.5.x`): bug fixes and internal changes. A program that builds
  and works with an earlier `0.5.x` release builds with the new one and
  behaves the same.
- **Minor** (`0.6.0`): language changes, including new features. While Zen C
  is pre-1.0, a minor release may break existing programs: new or changed
  syntax, different semantics, or new errors on code that used to compile
  and work.

From 1.0 on, minor releases will stay backwards compatible, and only a major
release will be allowed to break existing programs.

Rejecting code that the compiler used to accept but silently miscompiled
(generated C that failed to build, wrong behavior at run time) is a fix, not
a breaking change: those programs were already broken. The same goes for
clearer or more detailed diagnostics.

Each line has its own branch. Base your branch on the one your PR targets:

| Branch | Holds | Target it when your change… |
| :-- | :-- | :-- |
| `main` | the current patch line (`0.5.x`) | fixes a bug or refactors without changing what working programs do |
| `next/<version>` (now `next/0.6`) | the next minor release | changes the language, or rejects programs that used to compile and work |

- If a change does both, split it: the fix to `main`, the language change to
  `next/<version>`.
- If an existing test must change for your PR to pass, it probably belongs
  in `next/<version>`.
- `next/<version>` is kept up to date by rebasing it onto `main`. If your
  branch starts from it, rebase your branch too; don't merge.
- Label PRs that target `next/<version>` with `language`, and add
  `breaking-change` if they break existing programs.

When a change must be in both branches but behave differently in each, guard
the old behavior with a compatibility flag in `src/feature_flags.h` (for
example `FF_COMPATIBILITY_0_5`): it is `1` on `main` and `0` on
`next/<version>`, and the flag is removed with all its `#if` blocks when the
minor version is released. Tests that depend on a flag mention it, so
`grep -rn FF_COMPATIBILITY_0_5 src tests` finds everything to remove.

