// SPDX-License-Identifier: MIT
#ifndef TYPECHECK_H
#ifndef ZC_ALLOW_INTERNAL
#error "analysis/typecheck.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define TYPECHECK_H

#include "ast.h"
#include "parser.h"

struct MoveState; // Forward declaration

/// A loop being checked, so that a `break` can say it leaves it.
typedef struct LoopFrame
{
    const char *label;       ///< The loop's label, or NULL.
    int has_break;           ///< A `break` leaves this loop: the code after it is reachable.
    struct LoopFrame *outer; ///< The enclosing loop in the same function.
} LoopFrame;

// Type Checker Context
// Holds the state during the semantic analysis pass.
// Unlike the parser, this focuses on semantic validity (types, definitions).
/**
 * @brief Type Checker Context.
 *
 * Holds the state during the semantic analysis pass.
 * Unlike the parser, this focuses on semantic validity (types, definitions, correctness).
 */
typedef struct TypeChecker
{
    ParserContext *pctx;   ///< Reference to global parser context (for lookups).
    ASTNode *current_func; ///< Current function being checked.
    ASTNode *returns_from; ///< The function or lambda a `return` belongs to: the innermost one.
    int error_count;       ///< Number of type errors found.
    int warning_count;     // Number of recommendations/warnings.

    // Flow Analysis State
    struct MoveState *move_state; ///< Current state of moved variables.
    int is_unreachable;           ///< Path ends in break/return/continue
    struct MoveState *loop_break_state;
    struct MoveState *loop_continue_state;
    struct MoveState *loop_start_state;
    int in_loop_pass2;
    LoopFrame *current_loop; ///< Innermost loop being checked; NULL outside loops.

    // Configuration
    int rechecking; ///< Checking code a second time (a loop's second pass, the expressions of an
                    ///< f-string): only moves are reported.
    int type_errors_as_warnings; ///< `zc build` in 0.5: a type error is a warning; from 0.6 it
                                 ///< stops the build as in `zc check`.

    // Tracking
    int is_assign_lhs;      ///< If true, currently evaluating LHS of assignment.
    int is_stmt_context;    ///< If true, expression is a top-level statement.
    int is_forget_receiver; ///< If true, receiver of a .forget() call (use-after-move is expected).
    int is_callee;          ///< If true, the name being checked is the one a call calls.
    int loop_break_count;   ///< Count of breaks for Rule 15.4
    int func_return_count;  ///< Count of returns for Rule 15.5
    int current_depth;      ///< Current nesting level for escape analysis (0=global).
} TypeChecker;

/**
 * @brief Main Type Checking Entry Point.
 *
 * Performs semantic analysis on the entire AST.
 *
 * @param ctx Global parser context.
 * @param root Root AST node of the program.
 * @return 0 on success (no errors), non-zero if errors occurred.
 */
int check_program(ParserContext *ctx, ASTNode *root);


// Error helpers available to move_check.c
void tc_error(TypeChecker *tc, Token t, const char *msg);
void tc_warning(TypeChecker *tc, Token t, const char *msg);

/**
 * @brief Reports code that no path reaches: a warning, or an error under MISRA (Rule 2.1).
 */
void tc_unreachable_code(TypeChecker *tc, Token t);
int tc_file_in_std(TypeChecker *tc, const char *file);
/**
 * @brief Where the place `place` lives, for escape analysis: the scope depth of a variable (0 for
 *        a global); for a field or an element, that of the value holding it, or, through a
 *        pointer, the depth the pointer points to. Anything else is a temporary of the current
 *        scope. A type's lifetime_depth is the other half: where a value points.
 */
int tc_storage_depth(TypeChecker *tc, ASTNode *place);
void tc_error_with_hints(TypeChecker *tc, Token t, const char *msg, const char *const *hints);
void tc_ctrl_flow_error(TypeChecker *tc, Token t, const char *msg, const char *const *hints);
void tc_move_error_with_hints(TypeChecker *tc, Token t, const char *msg, const char *const *hints,
                              const DiagnosticNote *note);

// Exported helpers for modularization
int tc_expr_has_side_effects(ASTNode *node);

#endif // TYPECHECK_H
