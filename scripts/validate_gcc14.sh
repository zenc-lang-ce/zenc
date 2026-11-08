#!/bin/bash
# Runs the CI's Linux jobs locally, in a container, before pushing a change in C.
#
# Usage (from the repository root): scripts/validate_gcc14.sh [FILES...]
#
# Needs the docker image `zc-validate`:
#   docker build -t zc-validate -f scripts/validate.Dockerfile scripts
#
# Runs the gcc-14 builds of the CI (tsan, lsan, asan, analyzer), the test suite
# (C and --cpp), the diagnostic output tests and the Linux TCC job (zc built
# with tcc, the suite with --cc tcc). It builds in a tmpfs inside the container
# from the tracked and untracked (not ignored) files of the checkout, which is
# mounted read-only: nothing is written to the disk.
# Then it runs the CI's clang-format on FILES, in place, and says if it changed any.
#
# macOS's clang misses warnings that fail the CI (-Wformat-truncation,
# -Wsuggest-attribute=const, -Wnull-dereference at -O1, the static analyzer).
if ! docker image inspect zc-validate >/dev/null 2>&1; then
    echo "Missing docker image zc-validate. Build it with:"
    echo "  docker build -t zc-validate -f scripts/validate.Dockerfile scripts"
    exit 2
fi
WORK=${ZC_WORK:-${TMPDIR:-/tmp}/zc-work}
mkdir -p "$WORK"
LIST=$WORK/validate_files.txt
git ls-files -co --exclude-standard > "$LIST"
docker run --rm -v "$PWD":/src:ro -v "$LIST":/files.txt:ro --tmpfs /build:exec,size=3g zc-validate bash -c '
  mkdir -p /build/z && tar -C /src -cf - -T /files.txt | tar -C /build/z -xf - && cd /build/z
  for t in tsan lsan asan analyzer; do make clean >/dev/null 2>&1; j=1; [ $t = tsan -o $t = analyzer ] && j=0
    if make -k -j8 CC=gcc-14 $t ZC_HAS_JIT=$j > /build/$t.log 2>&1; then echo "$t OK"; else echo "$t FAIL"; grep -h "error:" /build/$t.log | sort -u | head -5; fi; done
  make clean >/dev/null; make -j8 CC=gcc-14 ZC_HAS_JIT=1 >/dev/null 2>&1 && ./tests/scripts/run_tests.sh -j 8 > /build/suite.log 2>&1
  grep -E "Passed|Failed:" /build/suite.log; grep -A3 "Failed tests" /build/suite.log | head -5
  ./tests/scripts/run_tests.sh --cpp --no-source -j 4 > /build/cpp.log 2>&1; grep -E "Passed|Failed:" /build/cpp.log
  ./tests/scripts/run_diagnostic_output_tests.sh 2>&1 | tail -1
  make clean >/dev/null; make -j8 CC=tcc ZC_HAS_JIT=1 > /build/tcc_build.log 2>&1 || { echo "tcc build FAIL"; grep -h "error" /build/tcc_build.log | head -5; }
  ./tests/scripts/run_tests.sh --cc tcc --no-source -j 8 > /build/tcc.log 2>&1; echo "tcc: $(grep -E "Passed|Failed:" /build/tcc.log | tr "\n" " ")"
  grep -E "error:" /build/tcc.log | sed -E "s/^.*error: //" | sort | uniq -c | sort -rn | head -5'
rm -f "$LIST"
if [ $# -gt 0 ]; then
    before=$(cksum "$@" 2>/dev/null)
    docker run --rm -v "$PWD":/src -w /src zc-validate clang-format -i "$@"
    after=$(cksum "$@" 2>/dev/null)
    if [ "$before" != "$after" ]; then
        echo "clang-format changed some of: $*"
    else
        echo "fmt OK"
    fi
fi
