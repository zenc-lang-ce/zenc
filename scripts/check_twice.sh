#!/bin/bash
# Metric for stage 2 of the architecture plan (an analysis that does not destroy what it
# reads): for every test, does checking the program twice (`--check-twice`) give the same
# tree and the same C as checking it once? A difference means the typechecker rewrote the
# tree in a way it cannot read again, or kept state between the two runs.
#
# Usage (from the repository root): scripts/check_twice.sh [-v] [ZC] [test.zc...]
#   ZC defaults to ./zc. Without test files it uses tests/scripts/list_tests.sh.
#   -v also lists the tests that differ, with what differs (ast, c or fails).
#   ZC_TEST_JOBS sets the parallelism. The outputs go to a temporary folder, deleted at the end.
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
if [ ${#TESTS[@]} -eq 0 ]; then
    echo "Error: no tests to measure"
    exit 2
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

measure_one() {
    # $1 = zc binary, $2 = test file
    local id dir
    id=$(printf '%s' "$2" | tr '/.' '__')
    dir="$WORK/$id"
    mkdir -p "$dir"
    # Only programs the typechecker accepts: a second run after errors means nothing.
    if ! "$1" transpile "$2" -o "$dir/once.c" >/dev/null 2>&1; then
        rm -rf "$dir"
        return
    fi
    echo "$2" >> "$WORK/measured.txt"
    local what=""
    if ! "$1" transpile --check-twice "$2" -o "$dir/twice.c" >/dev/null 2>&1; then
        what="fails"
    else
        cmp -s "$dir/once.c" "$dir/twice.c" || what="c"
        "$1" transpile --backend ast-dump "$2" -o "$dir/once.ast" >/dev/null 2>&1
        "$1" transpile --backend ast-dump --check-twice "$2" -o "$dir/twice.ast" >/dev/null 2>&1
        cmp -s "$dir/once.ast" "$dir/twice.ast" || what="${what:+$what+}ast"
    fi
    [ -n "$what" ] && echo "$what $2" >> "$WORK/differ.txt"
    rm -rf "$dir"
}
export -f measure_one
export WORK

printf '%s\n' "${TESTS[@]}" | xargs -P "$JOBS" -I{} bash -c 'measure_one "$0" "$1"' "$ZC" {}

measured=$( [ -f "$WORK/measured.txt" ] && wc -l < "$WORK/measured.txt" | tr -d ' ' || echo 0)
differ=$( [ -f "$WORK/differ.txt" ] && wc -l < "$WORK/differ.txt" | tr -d ' ' || echo 0)
echo "Tests the typechecker accepts: $measured"
echo "Of those, different when checked twice: $differ"
if [ -f "$WORK/differ.txt" ]; then
    echo "By what differs:"
    awk '{print $1}' "$WORK/differ.txt" | sort | uniq -c | sort -rn | sed 's/^/  /'
fi
if [ $VERBOSE -eq 1 ] && [ -f "$WORK/differ.txt" ]; then
    echo "Tests:"
    sort -k2 "$WORK/differ.txt" | sed 's/^/  /'
fi
