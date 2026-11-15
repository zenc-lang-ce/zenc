#!/bin/bash
# Measures how far the compiler is from its target architecture: one
# typechecker for every command, a parser that only parses, a backend that
# only writes, and data instead of text (see "Rules" in CONTRIBUTING.md).
# Every metric should go down to 0; a change should not make one go up.
#
# Usage (from the repository root): scripts/architecture_metrics.sh [--run [ZC]]
#   Without options it counts lines in src/ that match each pattern, which
#   takes a second. The counts are approximate: a line is counted whether it
#   is a call, a definition or a comment.
#   --run also runs scripts/check_vs_run.sh with ZC (default ./zc), which
#   compiles the whole suite and takes minutes.
cd "$(dirname "$0")/.." || exit 2

RUN_SUITE=0
ZC=./zc
if [ "$1" = "--run" ]; then
    RUN_SUITE=1
    [ -n "$2" ] && ZC=$2
fi

# Calls that look a declaration up by name, or work out a type again, instead
# of reading it from the AST.
LOOKUPS='infer_type|find_struct_def|find_concrete_struct_def|find_struct_field|find_func|find_func_template|find_function_definition|find_symbol[a-z_]*|add_symbol[a-z_]*|find_type_alias[a-z_]*|find_enum_variant[a-z_]*|find_trait_def|find_generic_template|find_known_generic|is_known_generic|find_instantiation|find_method_sig|find_method_call_target|find_selective_import|find_module'
DIAGNOSTICS='z(error|panic|warn)[a-z_]*'

# count PATTERN DIR...: lines of the .c files under DIR... that match PATTERN.
count() {
    local pattern=$1
    shift
    grep -rE --include='*.c' -- "$pattern" "$@" | wc -l | tr -d ' '
}

row() {
    printf '%-58s %7s   %s\n' "$1" "$2" "$3"
}

printf '%-58s %7s   %s\n' "Metric" "Count" "Stage"

if [ $RUN_SUITE -eq 1 ]; then
    report=$(scripts/check_vs_run.sh "$ZC")
    passing=$(printf '%s\n' "$report" | sed -n "s/^Tests passing with 'zc run': //p")
    rejected=$(printf '%s\n' "$report" | sed -n "s/^Of those, rejected by 'zc check': //p")
    row "Tests that 'zc run' passes and 'zc check' rejects" "$rejected" "1 (of $passing)"
fi

# Stage 1: the typechecker runs, and decides, in every command.
row "References to move_checks_only" "$(count 'move_checks_only' src)" 1
row "tc_ctrl_flow_error (type errors that stop a build)" "$(count '\btc_ctrl_flow_error\(' src)" 1
row "Diagnostics in src/codegen" "$(count "\b$DIAGNOSTICS\(" src/codegen)" 1

# Stage 2: elaboration wraps nodes instead of rewriting them.
row "Nodes rewritten with 'kind = NODE_' in src/analysis" "$(count '\bkind = NODE_' src/analysis)" 2

# Stage 3: data instead of text.
row "strcmp of a name with a literal: src/codegen" "$(count 'strcmp\([^;]*"[A-Za-z_][A-Za-z0-9_]*"' src/codegen)" 3
row "strcmp of a name with a literal: src/parser" "$(count 'strcmp\([^;]*"[A-Za-z_][A-Za-z0-9_]*"' src/parser)" 3
row "strcmp of a name with a literal: src/analysis" "$(count 'strcmp\([^;]*"[A-Za-z_][A-Za-z0-9_]*"' src/analysis)" 3
row "NODE_RAW_STMT created by the parser" "$(count 'ast_create\(NODE_RAW_STMT' src/parser)" 3

# Stage 4: references are links, and the backend reads them.
row "Lookups and type inference in src/codegen" "$(count "\b($LOOKUPS)\(" src/codegen)" 4

# Stage 6: semantics leaves the parser.
row "Lookups and type inference in src/parser" "$(count "\b($LOOKUPS)\(" src/parser)" 6
row "instantiate_* calls in src/parser" "$(count '\binstantiate_[a-z_]+\(' src/parser)" 6
row "Diagnostics in src/parser (syntax ones stay)" "$(count "\b$DIAGNOSTICS\(" src/parser)" 6
row "Assignments to type_info in src/parser" "$(count 'type_info = ' src/parser)" 6
