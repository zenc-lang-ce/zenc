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

/// A lambda whose body waits for its parameters' types: `(x, y) => x + y` with no type to take
/// them from gets them at its first call, and its body is checked then.
typedef struct WaitingLambda
{
    ASTNode *lambda;
    Scope *scope;    ///< The lambda's own scope: its parameters and captures.
    int scope_depth; ///< tc->current_depth inside that scope.
    int node_depth;  ///< The recursion depth the body is checked at.
    ASTNode *func;   ///< The function it is written in.
} WaitingLambda;

/// The C name a local got (tc_declare_local), by where it is declared and its name: a loop's
/// second pass finds the same one.
typedef struct LocalCName
{
    const char *site; ///< The declaration's token (Token.start).
    const char *name;
    const char *c_name;
    ZenSymbol *symbol; ///< Its symbol in the check under way.
} LocalCName;

/// The locals of one C function: a function, or a lambda, which is a C function of its own.
typedef struct CFunctionLocals
{
    ASTNode *function;
    LocalCName *locals;
    int count;
    int capacity;
    struct CFunctionLocals *next;
} CFunctionLocals;

/**
 * @brief How the program uses a symbol, as one analysis finds it. It is the analysis's, not the
 *        symbol's: checking the program again starts from none.
 */
typedef struct SymbolUse
{
    int is_written_to;         ///< The value, or what it points to, is modified.
    int is_dereferenced;       ///< A struct whose fields are accessed (MISRA Rule 2.3).
    ASTNode *first_using_func; ///< MISRA Rule 8.9: the first function that uses a global.
    int multi_func_use;        ///< More than one function uses it.
} SymbolUse;

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
    LoopFrame *current_loop;       ///< Innermost loop being checked; NULL outside loops.
    const char **escapes_reported; ///< Where an escape was reported (Token.start), so a loop's
                                   ///< second pass does not report it again.
    int escapes_reported_count;
    WaitingLambda *waiting_lambdas; ///< Lambdas whose body waits for a call (WaitingLambda).
    int waiting_lambda_count;
    CFunctionLocals *c_functions; ///< The locals of every C function checked (tc_declare_local).
    CFunctionLocals *c_function;  ///< Those of the one being checked; NULL at the top level.
    zmap_UseBySymbol symbol_uses; ///< How the program uses each symbol (tc_symbol_use).

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
 * @brief How the program uses `sym`, as this analysis has found it so far; created empty the first
 *        time. tc_symbol_use_found does not create it: NULL when nothing was found.
 */
SymbolUse *tc_symbol_use(TypeChecker *tc, const struct ZenSymbol *sym);
const SymbolUse *tc_symbol_use_found(const TypeChecker *tc, const struct ZenSymbol *sym);

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
/**
 * @brief Everything between parsing a program and generating its code: the Drop traits, the
 *        validation of its types and function definitions, and the typechecker (twice with
 *        `--check-twice`). Codegen relies on what the typechecker settles, so every path that
 *        generates code runs this first.
 * @return 0 on success; -1 if the types or function definitions are invalid (the typechecker
 *         did not run); otherwise check_program's result.
 */
int analyze_program(ParserContext *ctx, ASTNode *root);

// Error helpers available to move_check.c
void tc_error(TypeChecker *tc, Token t, const char *msg);
void tc_warning(TypeChecker *tc, Token t, const char *msg);

/**
 * @brief Reports code that no path reaches: a warning, or an error under MISRA (Rule 2.1).
 */
void tc_unreachable_code(TypeChecker *tc, Token t);

/**
 * @brief Reports a pointer that escapes, once per place: also in a loop's second pass, which
 *        reads what the end of the body assigned and so can find an escape the first could not.
 */
void tc_escape_error(TypeChecker *tc, Token t, const char *msg, const char *const *hints);
int tc_file_in_std(TypeChecker *tc, const char *file);
/**
 * @brief Where the place `place` lives, for escape analysis: the scope depth of a variable (0 for
 *        a global); for a field or an element, that of the value holding it, or, through a
 *        pointer, the depth the pointer points to. Anything else is a temporary of the current
 *        scope. A type's lifetime_depth is the other half: where a value points.
 */
int tc_storage_depth(TypeChecker *tc, ASTNode *place);
void tc_error_with_hints(TypeChecker *tc, Token t, const char *msg, const char *const *hints);

/**
 * @brief Reports what 0.6 makes an error: in 0.5 (FF_COMPATIBILITY_0_5) a warning that says
 *        so, in `zc check` and in a build, since the program was valid; from 0.6 an error.
 */
void tc_error_from_0_6(TypeChecker *tc, Token t, const char *msg, const char *const *hints);
/**
 * @brief Reports an error that stops `zc build` too, in 0.5 as well: one the compiler cannot
 *        write C for. `note`, if not NULL, points at another place.
 */
void tc_error_stopping_build(TypeChecker *tc, Token t, const char *msg, const char *const *hints,
                             const DiagnosticNote *note);
void tc_ctrl_flow_error(TypeChecker *tc, Token t, const char *msg, const char *const *hints);
void tc_move_error_with_hints(TypeChecker *tc, Token t, const char *msg, const char *const *hints,
                              const DiagnosticNote *note);

// Exported helpers for modularization
int tc_expr_has_side_effects(ASTNode *node);

#endif // TYPECHECK_H
