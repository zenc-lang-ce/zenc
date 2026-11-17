#!/bin/bash
# Prints the .zc tests that the C-mode runner would execute on this machine,
# one per line: same exclusions as run_tests.sh (helper modules, backends,
# MISRA, diagnostic output tests, REQUIRE: CHECK/CUDA/OBJC, tests for another
# architecture, REQUIRE: NOT WINDOWS on Windows). Fails when it finds no test.
# Usage: tests/scripts/list_tests.sh [dir]   (default: tests)
TEST_DIR=${1:-tests}
arch=$(uname -m)
system=$(uname -s 2>/dev/null)

# A function, not inline in $(...): bash 3.2 cannot parse a `case` there.
list() {
    find "$TEST_DIR" -name "*.zc" -not -name "_*.zc" -not -path "*/backends/*" -not -path "*/misra/*" -not -path "*/diagnostics_output/*" | sort |
        while IFS= read -r f; do
            if grep -qE '// REQUIRE: (CHECK|CUDA|OBJC)' "$f"; then continue; fi
            case "$arch" in
                *86*|amd64) ;;
                *) case "$f" in *test_asm*|*test_intel*|*test_simd_x86*) continue ;; esac ;;
            esac
            case "$arch" in
                *arm64*|aarch64) ;;
                *) case "$f" in *_arm64.zc|*_aarch64.zc) continue ;; esac ;;
            esac
            case "$system" in
                MINGW*|MSYS*|CYGWIN*)
                    if grep -q '// REQUIRE: NOT WINDOWS' "$f"; then continue; fi ;;
            esac
            echo "$f"
        done
}

tests=$(list)
if [ -z "$tests" ]; then
    echo "list_tests.sh: no tests found in $TEST_DIR" >&2
    exit 1
fi
echo "$tests"
