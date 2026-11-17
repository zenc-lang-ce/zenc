// SPDX-License-Identifier: MIT
#include "typecheck_internal.h"

void check_block(TypeChecker *tc, ASTNode *block, int depth)
{
    tc_enter_scope(tc);
    ASTNode *stmt = block->block.statements;
    int reported = 0;

    while (stmt)
    {
        // A label can be reached by a jump, so it makes the code after it reachable again.
        if (stmt->kind == NODE_LABEL)
        {
            tc->is_unreachable = 0;
            reported = 0;
        }
        else if (tc->is_unreachable && !reported)
        {
            tc_unreachable_code(tc, stmt->token);
            reported = 1; // Only once per unreachable stretch
        }

        if (tc->pctx->config->misra_mode)
        {
            // Rule 2.2: There shall be no dead code (expressions with no effect)
            // We ignore call expressions here as they are handled by Rule 17.7
            if (stmt->kind >= NODE_EXPR_BINARY && stmt->kind <= NODE_EXPR_SLICE &&
                stmt->kind != NODE_EXPR_CALL)
            {
                if (!tc_expr_has_side_effects(stmt))
                {
                    tc_error(tc, stmt->token, "MISRA Rule 2.2: expression statement has no effect");
                }
            }
        }

        int old_stmt_ctx = tc->is_stmt_context;
        tc->is_stmt_context = 1;
        check_node(tc, stmt, depth + 1);
        tc->is_stmt_context = old_stmt_ctx;

        stmt = stmt->next;
    }
    tc_exit_scope(tc);
}

// The initial value `*value` of a declaration whose type is `declared` (NULL when it is inferred).
// A value with no type of its own takes the declared one, and a lambda the declared function
// type (adopt_context_type); any other value keeps its type and is compared with the declared one,
// where a constant without a suffix takes it (check_type_compatibility).
static void check_initial_value(TypeChecker *tc, Type *declared, ASTNode **value, Token t,
                                int depth)
{
    if (declared && !(*value)->type_info)
    {
        (*value)->type_info = declared;
    }
    adopt_context_type(*value, declared);
    int old_stmt_ctx = tc->is_stmt_context;
    tc->is_stmt_context = 0;
    check_node(tc, *value, depth + 1);
    tc->is_stmt_context = old_stmt_ctx;

    if (declared && (*value)->type_info)
    {
        apply_implicit_struct_pointer_conversions(tc, value, declared);
        // A variable points where its value points, so a declaration cannot make its value
        // escape; where the variable lives is its symbol's scope depth (tc_storage_depth).
        declared->lifetime_depth = (*value)->type_info->lifetime_depth;
        check_type_compatibility(tc, declared, (*value)->type_info, t, *value, 0);
    }
}

// A `def`: its value is checked as a `let`'s is. The node keeps the declared type in type_info.
void check_def(TypeChecker *tc, ASTNode *node, int depth)
{
    if (node->var_decl.init_expr)
    {
        check_initial_value(tc, node->type_info, &node->var_decl.init_expr, node->token, depth);
    }
}

void check_var_decl(TypeChecker *tc, ASTNode *node, int depth)
{
    if (node->var_decl.type_info)
    {
        misra_check_pointer_nesting(tc->pctx, node->var_decl.type_info, node->token);
    }

    // MISRA: Mark type as used
    mark_type_as_used(tc, node->var_decl.type_info);

    // Escape analysis: where a local variable points (one in a function or in a `test` block, as
    // the depth its symbol gets says; 0 is global). A variable with a value points where the
    // value points (below); one without a value is taken to point here. An assignment makes it
    // point as deep as what it assigns (check_expr_binary); in a loop, the second pass reads
    // what the end of the body assigned.
    int in_local_scope = tc->current_depth > 0;
    if (node->var_decl.type_info && !node->var_decl.init_expr && in_local_scope)
    {
        node->var_decl.type_info->lifetime_depth = tc->current_depth;
    }

    if (node->var_decl.init_expr)
    {
        check_initial_value(tc, node->var_decl.type_info, &node->var_decl.init_expr, node->token,
                            depth);

        Type *decl_type = node->var_decl.type_info;
        Type *init_type = node->var_decl.init_expr->type_info;

        if (decl_type && init_type)
        {
            if (tc->pctx->config->misra_mode)
            {
                misra_check_pointer_conversion(tc->pctx, decl_type, init_type, node->token);

                // Rule 9.3: Arrays shall not be partially initialized.
                if (decl_type->kind == TYPE_ARRAY && init_type->kind == TYPE_ARRAY)
                {
                    if (node->var_decl.init_expr->kind == NODE_EXPR_ARRAY_LITERAL)
                    {
                        if (decl_type->array_size != init_type->array_size)
                        {
                            tc_error(tc, node->token, "MISRA Rule 9.3");
                        }
                    }
                }
            }
        }

        // Move Analysis: Check if the initializer moves a non-copy value.
        check_move_for_rvalue(tc, node->var_decl.init_expr);
    }

    if (node->type_info)
    {
        misra_check_pointer_nesting(tc->pctx, node->var_decl.type_info, node->token);
    }

    // If type is not explicit, we should ideally infer it from init_expr.
    Type *t = node->type_info;
    if (!t && node->var_decl.init_expr)
    {
        // A clone of the value's type, which keeps where the value points.
        t = type_clone(node->var_decl.init_expr->type_info);
        node->type_info = t;
        node->var_decl.type_info = t;
    }

    misra_check_reserved_identifier(tc->pctx, node->var_decl.name, node->token);
    tc_add_symbol(tc, node->var_decl.name, t, node->token, 0);
    ZenSymbol *new_sym = tc_lookup(tc, node->var_decl.name);
    if (new_sym)
    {
        new_sym->is_static = node->var_decl.is_static;
        mark_symbol_valid(tc->pctx, new_sym, node);
    }

    if (tc->pctx->config->misra_mode && t && (t->kind == TYPE_ARRAY || slice_element(t)))
    {
        // Rule 8.11: Array with external linkage shall have explicit size
        ZenSymbol *existing = tc_lookup(tc, node->var_decl.name);
        int is_static = (existing && existing->is_static) || (node->var_decl.is_static);
        int is_local = (existing && existing->is_local) || (tc->current_func != NULL);

        misra_check_external_array_size(tc->pctx, t, node->token, is_static, is_local);

        // Rule 18.8: No variable length arrays
        // In Zen C, all [T; N] arrays have constant size N, so Rule 18.8 is satisfied.
        // We only report if Zen somehow allowed non-constant sizes (which it doesn't in fixed-size
        // arrays).
    }
}

void check_function(TypeChecker *tc, ASTNode *node, int depth)
{
    if (!node)
    {
        return;
    }
    misra_check_param_nesting(tc->pctx, node);
    // Mark arg types as used
    for (int i = 0; i < node->func.count; i++)
    {
        mark_type_as_used(tc, node->func.arg_types[i]);
    }

    // Mark return type as used
    mark_type_as_used(tc, node->func.ret_type_info);

    // Rule Zen 1.4: Reserved identifiers
    misra_check_reserved_identifier(tc->pctx, node->func.name, node->token);

    tc->current_func = node;
    tc->returns_from = node;
    tc_enter_scope(tc);

    int prev_unreachable = tc->is_unreachable;
    tc->is_unreachable = 0;
    tc->func_return_count = 0;
    LoopFrame *prev_loop = tc->current_loop;
    tc->current_loop = NULL;

    MoveState *prev_move_state = tc->pctx->move_state;
    tc->pctx->move_state = move_state_create(NULL);

    for (int i = 0; i < node->func.count; i++)
    {
        if (node->func.param_names && node->func.param_names[i])
        {
            Type *param_type =
                (node->func.arg_types && node->func.arg_types[i]) ? node->func.arg_types[i] : NULL;

            misra_check_tuple_size(tc->pctx, param_type, node->token);
            misra_check_pointer_nesting(tc->pctx, param_type, node->token);
            misra_check_reserved_identifier(tc->pctx, node->func.param_names[i], node->token);
            tc_add_symbol(tc, node->func.param_names[i], param_type, node->token,
                          tc->pctx->config->misra_mode);
        }
    }

    if (node->func.ret_type_info)
    {
        misra_check_tuple_size(tc->pctx, node->func.ret_type_info, node->token);
        misra_check_pointer_nesting(tc->pctx, node->func.ret_type_info, node->token);

        // Lifetime Elision result was already computed in pre-pass for named functions.
        // For lambdas or if it somehow missed the pre-pass, ensure it's set.
        if (node->func.elide_from_idx == -1)
        {
            // Simple re-run if needed
        }
    }

    check_node(tc, node->func.body, depth + 1);
    // A lambda never called is checked with what it has, before leaving the function.
    check_uncalled_lambdas(tc, node);

    // Control flow analysis: Check if non-void function always returns
    Type *ret_type = node->func.ret_type_info;
    int is_void = !ret_type || ret_type->kind == TYPE_VOID;

    // Special case: 'main' is allowed to fall off the end (C99 implicit return 0)
    int is_main = node->func.name && strcmp(node->func.name, "main") == 0;

    if (is_main && is_void)
    {
        warn_void_main(node->token);
    }

    // Whether control flow reaches the end of the body (tc->is_unreachable, after checking it):
    // a function with a value must not fall off it. A last expression is not a return: C
    // returns nothing there.
    int end_reached = !tc->is_unreachable;
    if (!is_void && !is_main && node->func.body)
    {
        if (end_reached)
        {
            char msg[MAX_SHORT_MSG_LEN];
            snprintf(msg, sizeof(msg), "Function '%s' may not return a value on all code paths",
                     node->func.name);

            const char *hints[] = {"Ensure all execution paths return a value",
                                   "Consider adding a default return at the end of the function",
                                   NULL};
            tc_ctrl_flow_error(tc, node->token, msg, hints);
        }
    }

    move_state_free(tc->pctx->move_state);
    tc->pctx->move_state = prev_move_state;

    // MISRA audits before leaving function scope
    if (tc->pctx->config->misra_mode && tc->pctx->current_scope)
    {
        for (int i = 0; i < node->func.count; i++)
        {
            if (node->func.param_names && node->func.param_names[i])
            {
                ZenSymbol *psym =
                    symbol_lookup_local(tc->pctx->current_scope, node->func.param_names[i]);
                if (psym)
                {
                    // Rule 2.7: Unused parameter
                    if (!psym->is_used)
                    {
                        misra_check_unused_param(tc->pctx, psym->name, psym->decl_token);
                    }
                    // Rule 8.13: Pointer to const
                    if (psym->type_info && psym->type_info->kind == TYPE_POINTER)
                    {
                        Type *inner = resolve_alias(psym->type_info->inner);
                        if (inner && !inner->is_const && !psym->type_info->is_const &&
                            !psym->is_written_to)
                        {
                            misra_check_const_ptr_param(tc->pctx, psym->name, psym->decl_token);
                        }
                    }
                }
            }
        }
    }

    // MISRA Rule 15.5: A function shall have a single point of exit at the end.
    if (tc->pctx->config->misra_mode && tc->func_return_count > 1)
    {
        char msg[MAX_ERROR_MSG_LEN];
        snprintf(msg, sizeof(msg),
                 "MISRA Rule 15.5: function '%s' has %d return points (must have 1)",
                 node->func.name ? node->func.name : "anonymous", tc->func_return_count);
        tc_error(tc, node->token, msg);
    }

    tc->is_unreachable = prev_unreachable;
    tc->current_loop = prev_loop;
    tc_exit_scope(tc);
    tc->current_func = NULL;
    tc->returns_from = NULL;
}

void tc_check_trait(TypeChecker *tc, ASTNode *node, int depth)
{
    ASTNode *method = node->trait.methods;
    while (method)
    {
        check_node(tc, method, depth + 1);
        method = method->next;
    }
}

void tc_check_impl(TypeChecker *tc, ASTNode *node, int depth)
{
    ASTNode *method = node->impl.methods;
    while (method)
    {
        check_node(tc, method, depth + 1);
        method = method->next;
    }
}

void tc_check_impl_trait(TypeChecker *tc, ASTNode *node, int depth)
{
    node->impl_trait.trait = find_trait_def(tc->pctx, node->impl_trait.trait_name);
    ASTNode *method = node->impl_trait.methods;
    while (method)
    {
        check_node(tc, method, depth + 1);
        method = method->next;
    }
}

static const char *loop_label(ASTNode *loop)
{
    switch (loop->kind)
    {
    case NODE_WHILE:
        return loop->while_stmt.loop_label;
    case NODE_FOR:
        return loop->for_stmt.loop_label;
    case NODE_FOR_RANGE:
        return loop->for_range.loop_label;
    case NODE_LOOP:
        return loop->loop_stmt.loop_label;
    default:
        return NULL;
    }
}

void check_loop_passes(TypeChecker *tc, ASTNode *node, int depth)
{
    if (!node)
    {
        return;
    }
    RECURSION_GUARD_TOKEN(tc->pctx, node->token, );
    if (depth > 1024)
    {
        tc_error(tc, node->token, "Expression too deep");
        RECURSION_EXIT(tc->pctx);
        return;
    }

    MoveState *prev_break = tc->loop_break_state;
    MoveState *prev_cont = tc->loop_continue_state;
    tc->loop_break_state = NULL;
    tc->loop_continue_state = NULL;

    MoveState *initial_state = tc->pctx->move_state;
    MoveState *loop_start = initial_state ? move_state_clone(initial_state) : NULL;
    MoveState *outer_start_state = tc->loop_start_state;
    tc->loop_start_state = loop_start;

    int outer_in_pass2 = tc->in_loop_pass2;
    tc->in_loop_pass2 = 0;

    LoopFrame frame = {loop_label(node), 0, tc->current_loop};
    tc->current_loop = &frame;

    int outer_break_count = tc->loop_break_count;
    tc->loop_break_count = 0;

    int initial_unreachable = tc->is_unreachable;

    // Pass 1: standard typecheck and move check
    tc->is_unreachable = 0; // The loop start is assumed reachable if we got here

    switch (node->kind)
    {
    case NODE_WHILE:
    {
        misra_check_compound_body(tc->pctx, node->while_stmt.body, "while");
        int old_stmt_ctx = tc->is_stmt_context;
        tc->is_stmt_context = 0;
        check_node(tc, node->while_stmt.condition, depth + 1);
        tc->is_stmt_context = old_stmt_ctx;

        if (node->while_stmt.condition && node->while_stmt.condition->type_info)
        {
            Type *cond_type = resolve_alias(node->while_stmt.condition->type_info);
            if (tc->pctx->config->misra_mode)
            {
                if (cond_type->kind != TYPE_BOOL)
                {
                    misra_check_condition_boolean(tc->pctx, node->while_stmt.condition->type_info,
                                                  node->while_stmt.condition->token);
                }
                int inv;
                if (is_expression_invariant(tc, node->while_stmt.condition, &inv))
                {
                    misra_check_invariant_condition(tc->pctx, node->while_stmt.condition->token);
                }
            }
            else if (cond_type->kind != TYPE_BOOL && !is_integer_type(cond_type) &&
                     cond_type->kind != TYPE_POINTER && cond_type->kind != TYPE_STRING)
            {
                const char *hints[] = {"While conditions must be boolean, integer, or pointer",
                                       NULL};
                tc_error_with_hints(tc, node->while_stmt.condition->token,
                                    "Condition must be a truthy type", hints);
            }
        }
        check_node(tc, node->while_stmt.body, depth + 1);
        break;
    }

    case NODE_FOR:
        misra_check_compound_body(tc->pctx, node->for_stmt.body, "for");
        tc_enter_scope(tc); // For loop init variable is scoped
        check_node(tc, node->for_stmt.init, depth + 1);

        // Loop start is conceptually here for FOR
        if (loop_start)
        {
            move_state_free(loop_start);
        }
        loop_start = tc->pctx->move_state ? move_state_clone(tc->pctx->move_state) : NULL;
        tc->loop_start_state = loop_start;

        check_node(tc, node->for_stmt.condition, depth + 1);
        if (node->for_stmt.condition && node->for_stmt.condition->type_info)
        {
            Type *cond_type = resolve_alias(node->for_stmt.condition->type_info);
            if (tc->pctx->config->misra_mode)
            {
                if (cond_type->kind != TYPE_BOOL)
                {
                    misra_check_condition_boolean(tc->pctx, node->for_stmt.condition->type_info,
                                                  node->for_stmt.condition->token);
                }
                int inv;
                if (is_expression_invariant(tc, node->for_stmt.condition, &inv))
                {
                    misra_check_invariant_condition(tc->pctx, node->for_stmt.condition->token);
                }
            }
            else if (cond_type->kind != TYPE_BOOL && !is_integer_type(cond_type) &&
                     cond_type->kind != TYPE_POINTER && cond_type->kind != TYPE_STRING)
            {
                const char *hints[] = {"For conditions must be boolean, integer, or pointer", NULL};
                tc_error_with_hints(tc, node->for_stmt.condition->token,
                                    "Condition must be a truthy type", hints);
            }
        }
        check_node(tc, node->for_stmt.body, depth + 1);
        check_node(tc, node->for_stmt.step, depth + 1); // step happens after body

        if (tc->pctx->config->misra_mode && node->for_stmt.step)
        {
            if (node->for_stmt.step->kind == NODE_EXPR_BINARY)
            {
                if (writes_with_equals(node->for_stmt.step->binary.op) &&
                    node->for_stmt.step->binary.left->type_info)
                {
                    misra_check_loop_counter_float(tc->pctx,
                                                   node->for_stmt.step->binary.left->type_info,
                                                   node->for_stmt.step->token);
                }
            }
        }
        break;

    case NODE_FOR_RANGE:
        check_node(tc, node->for_range.start, depth + 1);
        check_node(tc, node->for_range.end, depth + 1);
        check_node(tc, node->for_range.step, depth + 1);

        // Loop start conceptually here
        if (loop_start)
        {
            move_state_free(loop_start);
        }
        loop_start = tc->pctx->move_state ? move_state_clone(tc->pctx->move_state) : NULL;
        tc->loop_start_state = loop_start;

        check_node(tc, node->for_range.body, depth + 1);
        break;

    case NODE_LOOP:
        check_node(tc, node->loop_stmt.body, depth + 1);
        break;

    default:
        break;
    }

    tc->loop_break_count = 0; // Reset for pass 2 (avoid double counts)

    // Determine next iter state based on continue and fallthrough
    MoveState *fallthrough_state = tc->pctx->move_state;
    int fallthrough_unreachable = tc->is_unreachable;

    MoveState *next_iter_state = NULL;
    if (!fallthrough_unreachable && fallthrough_state)
    {
        move_state_merge_into(&next_iter_state, fallthrough_state);
    }
    if (tc->loop_continue_state)
    {
        move_state_merge_into(&next_iter_state, tc->loop_continue_state);
    }

    // Pass 2: Re-run with next_iter_state to catch use-after-move across iterations
    if (next_iter_state)
    {
        int prev_rechecking = tc->rechecking;
        tc->rechecking = 1; // the first pass reported the type errors
        tc->in_loop_pass2 = 1;

        tc->pctx->move_state = move_state_clone(next_iter_state);
        tc->is_unreachable = 0;

        tc->loop_break_state = NULL;
        tc->loop_continue_state = NULL;

        // Re-run appropriate parts
        switch (node->kind)
        {
        case NODE_WHILE:
            check_node(tc, node->while_stmt.condition, depth + 1);
            check_node(tc, node->while_stmt.body, depth + 1);
            break;
        case NODE_FOR:
            check_node(tc, node->for_stmt.condition, depth + 1);
            check_node(tc, node->for_stmt.body, depth + 1);
            check_node(tc, node->for_stmt.step, depth + 1);
            break;
        case NODE_FOR_RANGE:
            check_node(tc, node->for_range.body, depth + 1);
            break;
        case NODE_LOOP:
            check_node(tc, node->loop_stmt.body, depth + 1);
            break;
        default:
            break;
        }

        if (tc->pctx->move_state)
        {
            move_state_free(tc->pctx->move_state);
        }
        if (tc->loop_break_state)
        {
            move_state_free(tc->loop_break_state);
        }
        if (tc->loop_continue_state)
        {
            move_state_free(tc->loop_continue_state);
        }

        tc->rechecking = prev_rechecking;
    }

    // Compute final move state exiting the loop
    MoveState *final_state = NULL;
    // Loops can exit via condition falsification (next_iter_state) or breaks
    if (next_iter_state)
    {
        // Assume infinite loops (like NODE_LOOP) don't exit naturally unless broken,
        // but for safety we'll merge next_iter_state for all, treating condition as maybe false.
        if (node->kind != NODE_LOOP)
        {
            move_state_merge_into(&final_state, next_iter_state);
        }
    }
    if (tc->loop_break_state)
    {
        move_state_merge_into(&final_state, tc->loop_break_state);
    }

    // Cleanup Pass 1 states
    if (tc->loop_break_state)
    {
        move_state_free(tc->loop_break_state);
    }
    if (tc->loop_continue_state)
    {
        move_state_free(tc->loop_continue_state);
    }

    tc->loop_break_count = outer_break_count;
    if (next_iter_state)
    {
        move_state_free(next_iter_state);
    }
    if (loop_start)
    {
        move_state_free(loop_start);
    }

    // Restore outer context
    if (node->kind == NODE_FOR)
    {
        tc_exit_scope(tc);
    }

    // Only a `break` leaves a `loop`; the other loops also end when their condition fails.
    tc->current_loop = frame.outer;
    tc->is_unreachable = initial_unreachable || (node->kind == NODE_LOOP && !frame.has_break);

    if (tc->pctx->move_state)
    {
        move_state_free(tc->pctx->move_state);
    }
    tc->pctx->move_state = final_state ? final_state : initial_state;

    tc->loop_break_state = prev_break;
    tc->loop_continue_state = prev_cont;
    tc->loop_start_state = outer_start_state;
    tc->in_loop_pass2 = outer_in_pass2;
    RECURSION_EXIT(tc->pctx);
}

// MAIN DISPATCH: check_node
