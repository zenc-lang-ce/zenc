#!/usr/bin/env python3
"""Migrates Zen C source to RFC 0001: only f"..." interpolates.

A plain "..." that relies on the old meaning is now an error, and this script
fixes the two cases the compiler reports by adding the f prefix, which keeps
what the string meant:

    "total: {n}"  -> f"total: {n}"   (the string interpolates)
    "{{x}}"       -> f"{{x}}"        (escaped braces that mean one brace)

It does not decide on its own which strings to change: it compiles each file
with `zc transpile --json`, applies the fixes at the positions of those errors,
and compiles again until none is left. Strings in files imported by a given
file are fixed too, as long as they are under one of the given paths.

Usage: scripts/migrate_fstrings.py [--zc PATH] [--dry-run] FILE_OR_DIR...
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

INTERPOLATES = 'only f"..." strings interpolate'
ESCAPED_BRACES = "'{{' and '}}' only escape braces in f\"...\" strings"
MAX_PASSES = 10


def zc_files(paths):
    for path in paths:
        if os.path.isdir(path):
            for root, _, names in os.walk(path):
                for name in sorted(names):
                    if name.endswith(".zc"):
                        yield os.path.join(root, name)
        else:
            yield path


def string_errors(zc, path):
    """The (file, line, col, message) of each RFC 0001 error in a compile of path, and
    whether the compile stopped on other errors (which may hide some of them)."""
    with tempfile.TemporaryDirectory() as tmp:
        out = os.path.join(tmp, "out.c")
        proc = subprocess.run(
            [zc, "transpile", path, "-o", out, "--json"],
            capture_output=True,
            text=True,
        )
    errors = set()
    other_errors = False
    for line in (proc.stdout + proc.stderr).splitlines():
        if not line.startswith("{"):
            continue
        try:
            diag = json.loads(line)
        except json.JSONDecodeError:
            continue
        if diag.get("level") != "error":
            continue
        if diag.get("message") in (INTERPOLATES, ESCAPED_BRACES):
            errors.add((os.path.abspath(diag["file"]), diag["line"], diag["col"], diag["message"]))
        else:
            other_errors = True
    return errors, other_errors


def fix_file(path, fixes, dry_run):
    """Adds the f prefix at each fix ((line, col), col in bytes) of one file. Returns how
    many it added and the fixes it could not apply (no string at that position)."""
    with open(path, "rb") as f:
        data = f.read()
    line_starts = [0]
    for i, byte in enumerate(data):
        if byte == ord("\n"):
            line_starts.append(i + 1)

    applied = 0
    skipped = set()
    # From the end of the file backwards, so earlier offsets stay valid.
    for line, col in sorted(fixes, reverse=True):
        start = line_starts[line - 1] + col - 1
        if data[start : start + 1] != b'"':
            skipped.add((path, line, col))
            continue
        data = data[:start] + b"f" + data[start:]
        applied += 1
        print(f"  {path}:{line}:{col}")

    if applied and not dry_run:
        with open(path, "wb") as f:
            f.write(data)
    return applied, skipped


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--zc", default=os.environ.get("ZC", "zc"), help="the zc binary (default: zc)")
    parser.add_argument("--dry-run", action="store_true", help="list the fixes without writing")
    parser.add_argument("paths", nargs="+", help="files or directories with .zc files")
    args = parser.parse_args()

    zc = os.path.abspath(args.zc) if os.path.sep in args.zc else args.zc
    roots = [os.path.abspath(p) for p in args.paths]
    files = list(zc_files(args.paths))
    total = 0
    left = set()
    unchecked = set()
    skipped = set()

    for _ in range(MAX_PASSES):
        by_file = {}
        left = set()
        unchecked = set()
        for path in files:
            errors, other_errors = string_errors(zc, path)
            if other_errors and not errors:
                unchecked.add(path)
            for file, line, col, message in errors:
                if any(file == r or file.startswith(r + os.sep) for r in roots):
                    by_file.setdefault(file, set()).add((line, col))
                else:
                    left.add((file, line, col, message))
        if not by_file:
            break
        skipped = set()
        for file in sorted(by_file):
            applied, not_applied = fix_file(file, by_file[file], args.dry_run)
            total += applied
            skipped |= not_applied
        if skipped and len(skipped) == sum(len(f) for f in by_file.values()):
            break
        if args.dry_run:
            break

    print(f"{total} string(s) fixed" + (" (dry run)" if args.dry_run else ""))
    for file, line, col, message in sorted(left):
        print(f"  not under the given paths: {file}:{line}:{col}: {message}", file=sys.stderr)
    for path in sorted(unchecked):
        print(f"  stopped on other errors, check by hand: {path}", file=sys.stderr)
    for path, line, col in sorted(skipped):
        # The compiler reports some positions wrong (e.g. in the body of a C-style for).
        print(f"  no string at the reported position, fix by hand: {path}:{line}:{col}", file=sys.stderr)
    return 1 if left or unchecked or skipped else 0


if __name__ == "__main__":
    sys.exit(main())
