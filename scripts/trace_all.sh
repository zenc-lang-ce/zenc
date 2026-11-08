#!/bin/bash
# Runs a zc built with temporary traces over every program, to show that a code
# path is dead or that a new check answers like the old one.
#
# Usage (from the repository root): scripts/trace_all.sh ZC OUTFILE
#
# Add `fprintf(stderr, "TEMP-TRACE ...")` where the old and new code would
# differ, and a `TT-REACHED` line printed only under `if (getenv("ZC_TT"))` to
# prove the code ran. This runs ZC over every test, MISRA program and
# awesome-zenc example ($AWESOME_ZENC, default ../awesome-zenc) in C,
# --misra, --cpp and `zc check`, and collects those lines in OUTFILE.
#
# It writes real output files (deleted as it goes): with `-o /dev/null`,
# `zc transpile` fails before codegen, so a trace there never runs.
# Remove every trace before committing: grep -rn 'TEMP-TRACE\|TT-REACHED' src
if [ $# -ne 2 ]; then
    echo "Usage: $0 ZC OUTFILE"
    exit 2
fi
ZC=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
OUT=$2
WORK=${ZC_WORK:-${TMPDIR:-/tmp}/zc-work}
S=${WORK:?}/tr
mkdir -p "$S"
export ZC S
one() {
  f=$1; t=$(echo "$f" | tr '/.' '__')
  for m in "" --misra --cpp; do
    ZC_TT=1 "$ZC" transpile $m "$f" -o "$S/$t$m.c" 2>&1 | grep -o "TEMP-TRACE.*\|TT-REACHED.*" | sed "s#^#$m #"
  done
  ZC_TT=1 "$ZC" check "$f" 2>&1 | grep -o "TEMP-TRACE.*\|TT-REACHED.*" | sed "s#^#check #"
  rm -f "${S:?}/${t:?}"*
}
export -f one
awesome=${AWESOME_ZENC:-../awesome-zenc}
{
    bash tests/scripts/list_tests.sh
    if [ -d "$awesome" ]; then
        find "$awesome" -name '*.zc' | grep -v /std/
    fi
    ls tests/misra/*.zc
} | tr '\n' '\0' | xargs -0 -P 8 -I{} bash -c 'one "$1"' _ {} > "$OUT"
rm -rf "$S"
echo "TEMP-TRACE lines: $(grep -c TEMP-TRACE "$OUT")"
grep -o "TT-REACHED.*" "$OUT" | sort | uniq -c
