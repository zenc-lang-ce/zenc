# Agent instructions

Instructions for AI coding agents working on the Zen C compiler. They build on
[CONTRIBUTING.md](CONTRIBUTING.md), which holds the rules for everyone.

## Read first

- [CONTRIBUTING.md](CONTRIBUTING.md): architecture, compiler conventions,
  how to test a change, versioning and which branch a PR targets. Its rules
  are mandatory.
- [STYLE.md](STYLE.md): formatting and naming. `make format` applies the
  formatting.
- [MANGLING.md](MANGLING.md) before touching symbol names.
- [tests/DOCS.md](tests/DOCS.md) before adding a test.
- The [language reference](https://zenc-lang-ce.github.io/docs/tour/)
  (source in the [zenc-docs](https://github.com/zenc-lang-ce/zenc-docs)
  repository), the tests under `tests/language/` and the standard library in
  `std/`, before writing Zen C. Do not write Zen C from memory or by analogy with
  Rust: the syntax and the standard library change between versions, and the
  tests show what the current compiler accepts.

## How to work

- **Find the cause before fixing.** Reproduce the bug with a small `.zc`
  program, then find where the wrong value is produced. Fix it there, not
  where it is read. Then try variants of the program (another name, another
  literal, another position): if one still fails in the same way, the fix
  covers a case, not the bug.
- **Choose the fix that simplifies the compiler** (see "Rules" in
  CONTRIBUTING.md). If you see two ways, say which one simplifies and
  recommend it.
- **Look for the existing mechanism** before writing a helper. Do not add a
  parallel one.
- **Keep the change to its topic.** If you find another bug on the way, write
  it down in your summary or open an issue; do not fix it in the same PR.
- **Decide the target branch before starting** (see "Versioning" in
  CONTRIBUTING.md): `main` for fixes that keep working programs as they are,
  `next/<version>` for language changes. If a change does both, split it.
- **Memory:** use the arena (`xmalloc`, `xcalloc`, `xrealloc`, `xstrdup`).
  `zfree` is a no-op; a bare `free()` on arena memory is an invalid free. See
  "Memory management" in CONTRIBUTING.md.
- **Diagnostics:** use the macro of the right class (`zpanic_at`, `zerror_at`,
  `zwarn_at`, `tc_error`), and never return an error node without having
  emitted one.

## Verifying a change

Follow "Testing a change" in CONTRIBUTING.md. In short:

1. `make` and `make test`.
2. Run the program that showed the bug with `zc run`, not only `zc check`:
   the two follow different paths ("How the phases run" in CONTRIBUTING.md).
3. Compare with a build of `main` (the table in "Testing a change" says which
   scripts a change needs): `tests/scripts/compare_compilers.sh`, and
   `scripts/measure_all.sh`, whose only differences in the generated C are
   the ones the change intends. If you touched the typechecker,
   `scripts/check_vs_run.sh` must not go up.
4. Before pushing C code: `scripts/validate_gcc14.sh` with the files you
   changed, if docker is available. Otherwise say in the PR that the gcc-14
   checks were not run locally.

Report what you ran and its result, including failures. Do not call something
fixed because it compiles, or because `zc check` accepts it.

## Do not

- **Fill the disk.** A full run over the suite writes hundreds of MB of
  generated C. Write outputs to a temporary directory, delete them as you go,
  and keep only what differs. The scripts in `scripts/` already do it, in
  `$ZC_WORK`: set it to a folder of your own scratch space. Do not
  commit generated C, binaries or test outputs.
- **Copy the repository** to build a reference compiler. Export `main` with
  `git archive` instead (about 20 MB once built), and delete it when done.
- **Trace codegen with `-o /dev/null`.** `zc transpile` fails before codegen
  when it cannot open its output, so a trace there never runs and "no
  differences" means nothing. Write to a real file.
- **Leave temporary traces.** `grep -rn 'TEMP-TRACE\|TT-REACHED' src` must
  print nothing before you commit.
- **Patch one case.** `strcmp(type, "string") == 0`, a check for a function
  name taken from the issue, and similar special cases are a last resort, not
  a fix. Do not leave inputs that fail in the same way out of scope.
- **Store C text in the AST**, or compare type names as strings (see "Rules"
  in CONTRIBUTING.md).
- **Change a test's expectation to make it pass** without explaining why the
  old expectation was wrong.
- **Silence a warning** (casts, `#pragma`, removing `-Werror`) instead of
  fixing its cause. An unused parameter that must stay is cast to `(void)`.

## Pull requests

- One topic per PR. The description says what changes for the user of the
  compiler and how it was verified.
- Labels: `ai assisted` for work done with an AI agent; `bug` for a fix;
  `lsp` if it changes the language server; `language` if it changes the
  language; `breaking-change` if it breaks existing programs.
