// SPDX-License-Identifier: MIT
#ifndef TYPECHECK_INTERNAL_H
#ifndef ZC_ALLOW_INTERNAL
#error                                                                                             \
    "analysis/typecheck_internal.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define TYPECHECK_INTERNAL_H

#include "../constants.h"
#include "typecheck.h"
#include "move_check.h"
#include "platform/misra.h"

// Symbol set for side-effect collision detection
typedef struct
{
    ZenSymbol *syms[32];
    int count;
} SymbolSet;

int eval_const_int_expr(ASTNode *node, ParserContext *ctx, long long *out_val);
int is_expression_invariant(TypeChecker *tc, ASTNode *node, int *val);

// Main dispatch + program-level (defined in typecheck.c)
void check_node(TypeChecker *tc, ASTNode *node, int depth);

// Utility helpers (defined in typecheck_utils.c)
int get_asm_register_size(Type *t);
void collect_symbols(ASTNode *node, SymbolSet *reads, SymbolSet *writes);
void tc_enter_scope(TypeChecker *tc);
void tc_exit_scope(TypeChecker *tc);
ZenSymbol *tc_add_symbol(TypeChecker *tc, const char *name, Type *type, Token t, int is_immutable);
ZenSymbol *tc_lookup(TypeChecker *tc, const char *name);
void mark_type_as_used(TypeChecker *tc, Type *t);
void check_side_effect_collision(TypeChecker *tc, ASTNode *left, ASTNode *right, Token token);
void check_all_args_side_effects(TypeChecker *tc, ASTNode *receiver, ASTNode *args, Token token);

// Expression checkers (defined in typecheck_expr.c)
void check_move_for_rvalue(TypeChecker *tc, ASTNode *rvalue);
// Reports moving `rvalue` when it is a value bound by a match arm, a copy of a payload that the
// matched value still owns and drops. Returns 1 when it reported it.
int check_match_binding_move(TypeChecker *tc, ASTNode *rvalue);
void check_expr_unary(TypeChecker *tc, ASTNode *node, int depth);
void check_expr_binary(TypeChecker *tc, ASTNode *node, int depth);
void check_expr_var(TypeChecker *tc, ASTNode *node);
void check_expr_literal(TypeChecker *tc, ASTNode *node);
void adopt_context_type(ASTNode *value, Type *expected);
void check_struct_init(TypeChecker *tc, ASTNode *node, int depth);
void check_expr_lambda(TypeChecker *tc, ASTNode *node, int depth);
int check_type_compatibility(TypeChecker *tc, Type *target, Type *value, Token t,
                             ASTNode *value_node, int is_call_arg);
void apply_implicit_struct_pointer_conversions(TypeChecker *tc, ASTNode **expr_ptr,
                                               Type *expected_type);

// Call checker (defined in typecheck_call.c)
void check_expr_call(TypeChecker *tc, ASTNode *node, int depth);

// Statement checkers (defined in typecheck_stmt.c)
void check_block(TypeChecker *tc, ASTNode *block, int depth);
void check_var_decl(TypeChecker *tc, ASTNode *node, int depth);
void check_def(TypeChecker *tc, ASTNode *node, int depth);
void check_function(TypeChecker *tc, ASTNode *node, int depth);
void check_loop_passes(TypeChecker *tc, ASTNode *node, int depth);
void tc_check_trait(TypeChecker *tc, ASTNode *node, int depth);
void tc_check_impl(TypeChecker *tc, ASTNode *node, int depth);
void tc_check_impl_trait(TypeChecker *tc, ASTNode *node, int depth);
int block_always_returns(ASTNode *block);
int stmt_always_returns(ASTNode *stmt);

#endif
