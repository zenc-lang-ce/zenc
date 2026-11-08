#!/bin/bash
# Compares the C++ that two zc binaries generate with --cpp.
#
# Usage (from the repository root): scripts/cpp_cmp.sh REF_ZC NEW_ZC
#
# Transpiles every test and awesome-zenc example ($AWESOME_ZENC, default
# ../awesome-zenc, skipped if missing) with both binaries and lists the ones
# whose C++ differs, with the first lines of the difference. Deletes the
# generated C++ when it finishes.
if [ $# -ne 2 ]; then
    echo "Usage: $0 REF_ZC NEW_ZC"
    exit 2
fi
WORK=${ZC_WORK:-${TMPDIR:-/tmp}/zc-work}
mkdir -p "$WORK"
O=${WORK:?}/cpp_o
N=${WORK:?}/cpp_n
rm -rf "$O" "$N"
mkdir -p "$O" "$N"
OD=$(cd "$(dirname "$1")" && pwd)
ND=$(cd "$(dirname "$2")" && pwd)
OLD=$OD/$(basename "$1")
NEW=$ND/$(basename "$2")
HERE=$PWD
export OLD NEW O N OD ND HERE
one() {
  f=$1; t=$(echo "$f" | tr "/.'" '___')
  (cd "$OD" && "$OLD" transpile --cpp "$f" -o "$O/$t.cpp" >/dev/null 2>&1)
  (cd "$ND" && "$NEW" transpile --cpp "$f" -o "$N/$t.cpp" >/dev/null 2>&1)
  if [ -f "$O/$t.cpp" ]; then
    sed "s#$OD#$ND#g" "$O/$t.cpp" > "$O/$t.tmp" && mv "$O/$t.tmp" "$O/$t.cpp"
  fi
  if [ -f "$O/$t.cpp" ] && [ -f "$N/$t.cpp" ]; then
    cmp -s "$O/$t.cpp" "$N/$t.cpp" || { echo "DIFF $f"; diff "$O/$t.cpp" "$N/$t.cpp" | grep "^[<>]" | head -4; }
  elif [ -f "$O/$t.cpp" ] || [ -f "$N/$t.cpp" ]; then
    echo "ONLY-ONE $f"
  fi
}
export -f one
awesome=${AWESOME_ZENC:-../awesome-zenc}
{
    bash tests/scripts/list_tests.sh | sed "s#^#$HERE/#"
    if [ -d "$awesome" ]; then
        find "$(cd "$awesome" && pwd)" -name '*.zc' | grep -v /std/
    fi
} | tr '\n' '\0' | xargs -0 -P 8 -I{} bash -c 'one "$1"' _ {} | sort
rm -rf "$O" "$N"
