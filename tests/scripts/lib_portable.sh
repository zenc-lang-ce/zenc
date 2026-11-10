#!/bin/bash
# Helpers for test scripts that must also run on the bash 3.2 shipped with
# macOS. Source it with: . "$(dirname "$0")/lib_portable.sh"

# read_lines NAME < input
# Reads stdin into the array NAME, one element per line (empty input gives an
# empty array). Portable replacement for `mapfile -t NAME`, which needs bash 4.
read_lines() {
    local __read_lines_name=$1 __read_lines_line
    eval "$__read_lines_name=()"
    while IFS= read -r __read_lines_line || [ -n "$__read_lines_line" ]; do
        eval "$__read_lines_name+=(\"\$__read_lines_line\")"
    done
}
