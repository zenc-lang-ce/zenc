#!/bin/bash
# Compares what two zc binaries produce, to show that a change keeps the output.
#
# Usage (from the repository root): scripts/measure_all.sh REF_ZC NEW_ZC TAG
#
# For every test, every MISRA program and every awesome-zenc example, it
# compares the generated C and the diagnostics of `zc transpile` and `zc check`
# (and of `--misra` for the MISRA programs), 8 at a time. The awesome-zenc
# examples are read from $AWESOME_ZENC (default ../awesome-zenc) and skipped
# if it does not exist.
#
# Output in $ZC_WORK/m_TAG (ZC_WORK defaults to $TMPDIR/zc-work): report.txt,
# and the C of the programs that differ. The C of a program is deleted as soon
# as it compares equal: a full run over the suite and the examples is about
# 440 MB of C.
#
# Each binary runs from its own directory (its std/ is found from the current
# directory first), on absolute paths; the reference directory is rewritten to
# the new one in the reference's outputs, so paths do not count as differences.
# The reference is usually `main` built apart: git archive main | tar -x -C DIR.
if [ $# -ne 3 ]; then
    echo "Usage: $0 REF_ZC NEW_ZC TAG"
    exit 2
fi
WORK=${ZC_WORK:-${TMPDIR:-/tmp}/zc-work}
mkdir -p "$WORK"
tag=$3
out=${WORK:?}/m_${tag:?}
rm -rf "$out"
mkdir -p "$out/o" "$out/n" "$out/r"
olddir=$(cd "$(dirname "$1")" && pwd)
newdir=$(cd "$(dirname "$2")" && pwd)
old=$olddir/$(basename "$1")
new=$newdir/$(basename "$2")
export XDG_CACHE_HOME=$WORK/cache_m old new out olddir newdir
norm() { sed "s#$olddir#$newdir#g"; }
# sed -i differs between GNU and BSD sed: rewrite through a temporary file.
rebase_paths() { sed "s#$olddir#$newdir#g" "$1" > "$1.tmp" && mv "$1.tmp" "$1"; }
export -f norm rebase_paths
one() {
  f=$1; n=$(echo "$f" | tr "/.'" '___'); r=$out/r/$n.txt; : > "$r"
  a=$(cd "$olddir" && "$old" transpile "$f" -o "$out/o/$n.c" 2>&1 | grep -v "Compiling\|Finished\|Transpiled" | norm)
  b=$(cd "$newdir" && "$new" transpile "$f" -o "$out/n/$n.c" 2>&1 | grep -v "Compiling\|Finished\|Transpiled")
  [ -s "$out/o/$n.c" ] && rebase_paths "$out/o/$n.c"
  if [ -s "$out/o/$n.c" ] && [ -s "$out/n/$n.c" ] && ! cmp -s "$out/o/$n.c" "$out/n/$n.c"; then
    echo "CDIFF $f" >> "$r"
  else
    rm -f "$out/o/$n.c" "$out/n/$n.c"
  fi
  if [ "$a" != "$b" ]; then echo "BUILDDIAG $f" >> "$r"; diff <(echo "$a") <(echo "$b") >> "$r"; fi
  a=$(cd "$olddir" && "$old" check "$f" 2>&1 | grep -v Compiling | norm)
  b=$(cd "$newdir" && "$new" check "$f" 2>&1 | grep -v Compiling)
  if [ "$a" != "$b" ]; then echo "CHECKDIAG $f" >> "$r"; diff <(echo "$a") <(echo "$b") >> "$r"; fi
}
onem() {
  f=$1; n=$(echo "$f" | tr '/.' '__'); r=$out/r/misra_$n.txt; : > "$r"
  a=$(cd "$olddir" && "$old" transpile "$f" --misra -o "$out/o/m_$n.c" 2>&1 | grep -v "Compiling\|Finished\|Transpiled" | norm)
  b=$(cd "$newdir" && "$new" transpile "$f" --misra -o "$out/n/m_$n.c" 2>&1 | grep -v "Compiling\|Finished\|Transpiled")
  [ -e "$out/o/m_$n.c" ] && rebase_paths "$out/o/m_$n.c"
  eo=0; [ -e "$out/o/m_$n.c" ] && eo=1; en=0; [ -e "$out/n/m_$n.c" ] && en=1
  if [ "$a" != "$b" ] || [ $eo != $en ] || { [ $eo = 1 ] && ! cmp -s "$out/o/m_$n.c" "$out/n/m_$n.c"; }; then
    echo "MISRA $f" >> "$r"; diff <(echo "$a") <(echo "$b") | head -20 >> "$r"
  else
    rm -f "$out/o/m_$n.c" "$out/n/m_$n.c"
  fi
}
export -f one onem
here=$(pwd)
awesome=${AWESOME_ZENC:-../awesome-zenc}
{
    bash tests/scripts/list_tests.sh | sed "s#^#$here/#"
    if [ -d "$awesome" ]; then
        find "$(cd "$awesome" && pwd)" -name '*.zc' | grep -v /std/
    else
        echo "($awesome not found: awesome-zenc examples skipped)" >&2
    fi
} | tr '\n' '\0' | xargs -0 -P 8 -I{} bash -c 'one "$1"' _ {}
ls "$here"/tests/misra/*.zc | tr '\n' '\0' | xargs -0 -P 8 -I{} bash -c 'onem "$1"' _ {}
cat "$out"/r/*.txt > "$out/report.txt"
c=$(grep -c '^CDIFF' "$out/report.txt")
d=$(grep -c '^BUILDDIAG\|^CHECKDIAG' "$out/report.txt")
m=$(grep -c '^MISRA' "$out/report.txt")
echo "C differ: $c  diagnostics differ: $d  MISRA differ: $m  (report $out/report.txt)"
rm -rf "${out:?}/r" "${WORK:?}/cache_m"
echo "kept: $(du -sh "$out" | cut -f1)"
