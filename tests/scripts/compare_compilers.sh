#!/bin/bash
# Runs every test with two zc binaries and lists the tests whose outcome
# (exit code of `zc run`) differs. Use it to check that a compiler change
# does not regress anything compared with a reference build. A test marked
# `// EXPECT: FAIL` counts as passing (0) when `zc run` fails and as failing
# (1) when it succeeds, as in run_tests.sh.
#
# By default it first transpiles every test with both binaries and only runs
# the tests whose generated C or transpile exit code differ: the same C gives
# the same `zc run` outcome, and running is what takes the time. A change the
# generated C does not show (how the driver builds the C compiler command,
# `//>` build directives...) needs --full, which runs every test.
#
# Usage: tests/scripts/compare_compilers.sh [--full] OLD_ZC NEW_ZC [test.zc...]
#   Without test files it uses tests/scripts/list_tests.sh.
#   ZC_TEST_JOBS sets the parallelism (default 8).
# Exit code: 1 if some test passes with OLD_ZC and fails with NEW_ZC.
. "$(dirname "$0")/lib_portable.sh"

FULL=0
if [ "$1" = "--full" ]; then
    FULL=1
    shift
fi
if [ $# -lt 2 ]; then
    echo "Usage: $0 [--full] OLD_ZC NEW_ZC [test.zc...]"
    exit 2
fi
OLD_ZC=$1
NEW_ZC=$2
shift 2
JOBS=${ZC_TEST_JOBS:-8}

if [ $# -gt 0 ]; then
    TESTS=("$@")
else
    read_lines TESTS < <("$(dirname "$0")/list_tests.sh")
fi
if [ ${#TESTS[@]} -eq 0 ]; then
    echo "Error: no tests to compare"
    exit 2
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

test_id() {
    # Without a subprocess: this runs once per test and side.
    printf '%s' "${1//[\/.]/_}"
}

transpile_one() {
    # $1 = label, $2 = zc binary, $3 = test file
    local id
    id=$(test_id "$3")
    "$2" transpile "$3" -o "$WORK/$1/$id.c" >/dev/null 2>&1
    echo $? > "$WORK/$1/$id.rc"
}

run_one() {
    # $1 = label, $2 = zc binary, $3 = test file
    local id
    id=$(test_id "$3")
    "$2" run -w "$3" -o "$WORK/$1$id" >/dev/null 2>&1
    local rc=$?
    if grep -q "// EXPECT: FAIL" "$3"; then
        if [ $rc -ne 0 ]; then rc=0; else rc=1; fi
    fi
    echo "$rc $3" >> "$WORK/$1.txt"
}
export -f test_id transpile_one run_one
export WORK

if [ $FULL -eq 1 ]; then
    TO_RUN=("${TESTS[@]}")
else
    mkdir -p "$WORK/old" "$WORK/new"
    for side in old new; do
        if [ "$side" = old ]; then zc=$OLD_ZC; else zc=$NEW_ZC; fi
        printf '%s\n' "${TESTS[@]}" | xargs -P "$JOBS" -I{} bash -c 'transpile_one "$0" "$1" "$2"' "$side" "$zc" {}
    done
    # One diff for all the tests: a file that differs or exists on one side
    # only (C not generated) marks its test.
    diff -rq "$WORK/old" "$WORK/new" | sed -nE \
        -e 's#^Files .*/old/(.*)\.(c|rc) and .* differ$#\1#p' \
        -e 's#^Only in .*: (.*)\.(c|rc)$#\1#p' | sort -u > "$WORK/changed"
    for t in "${TESTS[@]}"; do
        printf '%s %s\n' "${t//[\/.]/_}" "$t" # test_id, without a subshell
    done > "$WORK/ids"
    read_lines TO_RUN < <(awk 'NR == FNR { changed[$1] = 1; next } ($1 in changed) { print $2 }' \
        "$WORK/changed" "$WORK/ids")
fi

if [ ${#TO_RUN[@]} -gt 0 ]; then
    for side in old new; do
        if [ "$side" = old ]; then zc=$OLD_ZC; else zc=$NEW_ZC; fi
        printf '%s\n' "${TO_RUN[@]}" | xargs -P "$JOBS" -I{} bash -c 'run_one "$0" "$1" "$2"' "$side" "$zc" {}
        sort -k2 "$WORK/$side.txt" > "$WORK/$side.sorted"
    done
else
    : > "$WORK/old.sorted"
    : > "$WORK/new.sorted"
fi

echo "Compared ${#TESTS[@]} tests: $OLD_ZC vs $NEW_ZC"
if [ $FULL -eq 0 ]; then
    echo "  $(( ${#TESTS[@]} - ${#TO_RUN[@]} )) with the same generated C, ${#TO_RUN[@]} run with both"
fi
join -1 2 -2 2 "$WORK/old.sorted" "$WORK/new.sorted" > "$WORK/joined"
regressions=$(awk '$2 == 0 && $3 != 0' "$WORK/joined" | wc -l | tr -d ' ')
fixes=$(awk '$2 != 0 && $3 == 0' "$WORK/joined" | wc -l | tr -d ' ')
other=$(awk '$2 != 0 && $3 != 0 && $2 != $3' "$WORK/joined" | wc -l | tr -d ' ')
awk '$2 == 0 && $3 != 0 {print "  REGRESSION " $1}' "$WORK/joined"
awk '$2 != 0 && $3 == 0 {print "  FIXED      " $1}' "$WORK/joined"
awk '$2 != 0 && $3 != 0 && $2 != $3 {print "  EXIT " $2 " -> " $3 " " $1}' "$WORK/joined"
echo "Regressions: $regressions  Fixed: $fixes  Other changes: $other"
[ "$regressions" -eq 0 ]
