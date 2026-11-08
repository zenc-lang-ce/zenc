#!/bin/bash
# Metric for making the typechecker the single semantics: among the tests that
# compile and pass with `zc run`, how many does `zc check` (the full
# typechecker) reject, and with which errors. The goal is 0, so the
# typechecker can run in every build. A change must not make it go up.
#
# Usage (from the repository root): scripts/check_vs_run.sh [-v] [ZC] [test.zc...]
#   ZC defaults to ./zc. Without test files it uses tests/scripts/list_tests.sh.
#   -v also lists the rejected tests. ZC_TEST_JOBS sets the parallelism.
TEST_SCRIPTS="$(dirname "$0")/../tests/scripts"
. "$TEST_SCRIPTS/lib_portable.sh"

VERBOSE=0
if [ "$1" = "-v" ]; then
    VERBOSE=1
    shift
fi
ZC=${1:-./zc}
[ $# -gt 0 ] && shift
JOBS=${ZC_TEST_JOBS:-8}

if [ $# -gt 0 ]; then
    TESTS=("$@")
else
    read_lines TESTS < <("$TEST_SCRIPTS/list_tests.sh")
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

measure_one() {
    # $1 = zc binary, $2 = test file
    local id out
    id=$(printf '%s' "$2" | tr '/.' '__')
    if grep -q "// EXPECT: FAIL" "$2"; then return; fi
    if ! "$1" run -w "$2" -o "$WORK/bin$id" >/dev/null 2>&1; then return; fi
    rm -f "$WORK/bin$id"
    echo "$2" >> "$WORK/passing.txt"
    out=$("$1" check "$2" 2>&1)
    if [ $? -ne 0 ]; then
        echo "$2" >> "$WORK/rejected.txt"
        printf '%s\n' "$out" | grep '^error' | sed -E "s/'[^']*'/'X'/g" | sort -u >> "$WORK/errors.txt"
    fi
}
export -f measure_one
export WORK

printf '%s\n' "${TESTS[@]}" | xargs -P "$JOBS" -I{} bash -c 'measure_one "$0" "$1"' "$ZC" {}

passing=$( [ -f "$WORK/passing.txt" ] && wc -l < "$WORK/passing.txt" | tr -d ' ' || echo 0)
rejected=$( [ -f "$WORK/rejected.txt" ] && wc -l < "$WORK/rejected.txt" | tr -d ' ' || echo 0)
echo "Tests passing with 'zc run': $passing"
echo "Of those, rejected by 'zc check': $rejected"
if [ -f "$WORK/errors.txt" ]; then
    echo "Errors (number of tests showing each):"
    sort "$WORK/errors.txt" | uniq -c | sort -rn | sed 's/^/  /'
fi
if [ $VERBOSE -eq 1 ] && [ -f "$WORK/rejected.txt" ]; then
    echo "Rejected tests:"
    sort "$WORK/rejected.txt" | sed 's/^/  /'
fi
