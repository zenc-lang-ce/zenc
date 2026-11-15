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

// The variable an assignment writes into: `v`, or, with `parts`, also a field or an element of
// it at any depth. NULL when it writes elsewhere (`*p = ...`, a call's result).
static const char *assigned_variable(ASTNode *target, int parts)
{
    while (parts && target && (target->kind == NODE_EXPR_MEMBER || target->kind == NODE_EXPR_INDEX))
    {
        target = target->kind == NODE_EXPR_MEMBER ? target->member.target : target->index.array;
    }
    return target && target->kind == NODE_EXPR_VAR ? target->var_ref.name : NULL;
}

// Whether `value` reads nothing but the variable `name`: it takes no address and names no other
// variable or function. Such a value (`curr.next`, `p + 1`) points at most where `name` already
// does. An f-string or a lambda does not count: their pieces are not all children.
static int reads_only(ASTNode *value, const char *name)
{
    for (; value; value = value->next)
    {
        if ((value->kind == NODE_EXPR_UNARY &&
             (value->unary.op == OP_ADDRESS || value->unary.op == OP_ADDRESS_RVALUE)) ||
            (value->kind == NODE_EXPR_VAR && strcmp(value->var_ref.name, name) != 0) ||
            value->kind == NODE_EXPR_FSTRING || value->kind == NODE_LAMBDA)
        {
            return 0;
        }
        ASTChildren children = ast_children(value);
        for (int i = 0; i < children.count; i++)
        {
            if (!reads_only(*children.slots[i], name))
            {
                return 0;
            }
        }
    }
    return 1;
}

// Whether a statement of the list that starts at `node`, or one nested in it, assigns to the
// variable `name`, or, with `parts`, to a field or an element of it, a value that may point
// somewhere new: one that reads only the variable itself (`curr = curr.next`) does not count. By
// name: a variable that shadows it counts too, which only makes the caller more cautious.
static int is_assigned_in(ASTNode *node, const char *name, int parts)
{
    for (; node; node = node->next)
    {
        if (node->kind == NODE_EXPR_BINARY && is_assignment_operator(node->binary.op))
        {
            const char *assigned = assigned_variable(node->binary.left, parts);
            if (assigned && strcmp(assigned, name) == 0 && !reads_only(node->binary.right, name))
            {
                return 1;
            }
        }
        ASTChildren children = ast_children(node);
        for (int i = 0; i < children.count; i++)
        {
            if (is_assigned_in(*children.slots[i], name, parts))
            {
                return 1;
            }
        }
    }
    return 0;
}

// Whether a local variable of type `t`, declared by `decl`, may point somewhere its value did not:
// something is assigned to it later in its block. A field or an element of it counts unless it is
// a pointer, whose fields and elements are where it points, not part of it.
static int is_reassigned(ASTNode *decl, Type *t)
{
    Type *resolved = resolve_alias(t);
    int parts = !(resolved && (resolved->kind == TYPE_POINTER || resolved->kind == TYPE_STRING));
    return is_assigned_in(decl->next, decl->var_decl.name, parts);
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
    // value points (below). One without a value, or one that something is assigned to later in
    // its block (is_reassigned), is taken to point here: the check runs once, in source order, so
    // it cannot follow an assignment that comes after a read, as in a loop.
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
    // A variable with a value that something is assigned to later in its block points here
    // (see above), declared or inferred.
    if (node->var_decl.init_expr && in_local_scope && node->var_decl.type_info &&
        is_reassigned(node, node->var_decl.type_info))
    {
        node->var_decl.type_info->lifetime_depth = tc->current_depth;
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

static ASTNode *loop_body(ASTNode *loop)
{
    switch (loop->kind)
    {
    case NODE_LOOP:
        return loop->loop_stmt.body;
    case NODE_WHILE:
        return loop->while_stmt.body;
    case NODE_FOR:
        return loop->for_stmt.body;
    case NODE_FOR_RANGE:
        return loop->for_range.body;
    default:
        return NULL;
    }
}

// Returns 1 if `node` contains a `break` that can exit the loop whose label is
// `loop_label` (NULL = unlabeled). `nested` is 1 while inside a loop nested
// within the loop we are testing, so unlabeled breaks there do not count.
static int stmt_has_exit_break(ASTNode *node, const char *loop_label, int nested)
{
    if (!node)
    {
        return 0;
    }

    switch (node->kind)
    {
    case NODE_BREAK:
        if (node->break_stmt.target_label)
        {
            return loop_label != NULL && strcmp(node->break_stmt.target_label, loop_label) == 0;
        }
        return !nested;

    case NODE_BLOCK:
        for (ASTNode *s = node->block.statements; s; s = s->next)
        {
            if (stmt_has_exit_break(s, loop_label, nested))
            {
                return 1;
            }
        }
        return 0;

    case NODE_IF:
        return stmt_has_exit_break(node->if_stmt.then_body, loop_label, nested) ||
               stmt_has_exit_break(node->if_stmt.else_body, loop_label, nested);

    case NODE_MATCH:
        for (ASTNode *c = node->match_stmt.cases; c; c = c->next)
        {
            if (stmt_has_exit_break(c, loop_label, nested))
            {
                return 1;
            }
        }
        return 0;

    case NODE_MATCH_CASE:
        return stmt_has_exit_break(node->match_case.body, loop_label, nested);

    case NODE_DEFER:
        return stmt_has_exit_break(node->defer_stmt.stmt, loop_label, nested);

    case NODE_LOOP:
    case NODE_WHILE:
    case NODE_FOR:
    case NODE_FOR_RANGE:
        // Nested loop: unlabeled breaks here target the nested loop, but a
        // labeled break naming our loop still exits ours.
        return stmt_has_exit_break(loop_body(node), loop_label, 1);

    case NODE_RETURN:
    case NODE_CONTINUE:
    default:
        return 0;
    }
}

// Returns 1 if `loop` (a NODE_LOOP) contains a `break` that can exit it.
static int stmt_loop_has_exit_break(ASTNode *loop)
{
    return stmt_has_exit_break(loop->loop_stmt.body, loop->loop_stmt.loop_label, 0);
}

int stmt_always_returns(ASTNode *stmt)
{
    if (!stmt)
    {
        return 0;
    }

    switch (stmt->kind)
    {
    case NODE_RETURN:
        return 1;

    case NODE_BLOCK:
        return block_always_returns(stmt);

    case NODE_IF:
        // Both branches must return for if to always return
        if (stmt->if_stmt.then_body && stmt->if_stmt.else_body)
        {
            return stmt_always_returns(stmt->if_stmt.then_body) &&
                   stmt_always_returns(stmt->if_stmt.else_body);
        }
        return 0;

    case NODE_MATCH:
    {
        if (!stmt->match_stmt.cases)
        {
            return 0;
        }

        int has_default = 0;
        ASTNode *case_node = stmt->match_stmt.cases;
        while (case_node)
        {
            if (case_node->kind == NODE_MATCH_CASE)
            {
                if (!stmt_always_returns(case_node->match_case.body))
                {
                    return 0;
                }
                if (case_node->match_case.is_default)
                {
                    has_default = 1;
                }
            }
            case_node = case_node->next;
        }

        return has_default;
    }

    case NODE_LOOP:
        // An infinite `loop` only leaves via `break` or `return`. With no
        // exit-break it can never fall through (it returns from within or
        // diverges), so it satisfies the always-returns requirement. This
        // mirrors the move analysis, which already treats a break-less
        // infinite loop as unreachable-after.
        return !stmt_loop_has_exit_break(stmt);

    default:
        return 0;
    }
}

int block_always_returns(ASTNode *block)
{
    if (!block || block->kind != NODE_BLOCK)
    {
        return 0;
    }

    ASTNode *stmt = block->block.statements;
    if (!stmt)
    {
        return 0;
    }

    // Walk all statements except the last
    while (stmt->next)
    {
        if (stmt_always_returns(stmt))
        {
            return 1;
        }
        stmt = stmt->next;
    }

    // The last statement is the implicit return value.
    // Check if it always returns (RETURN, IF with both branches, MATCH with default),
    // or if it's an expression (which serves as the function's return value).
    if (stmt_always_returns(stmt))
    {
        return 1;
    }
    if (stmt->kind == NODE_BLOCK)
    {
        return block_always_returns(stmt);
    }
    // Any expression type can serve as an implicit return value
    if (stmt->kind == NODE_EXPR_BINARY || stmt->kind == NODE_EXPR_UNARY ||
        stmt->kind == NODE_EXPR_LITERAL || stmt->kind == NODE_EXPR_VAR ||
        stmt->kind == NODE_EXPR_CALL || stmt->kind == NODE_EXPR_MEMBER ||
        stmt->kind == NODE_EXPR_INDEX || stmt->kind == NODE_EXPR_CAST ||
        stmt->kind == NODE_EXPR_SIZEOF || stmt->kind == NODE_EXPR_STRUCT_INIT ||
        stmt->kind == NODE_EXPR_ARRAY_LITERAL || stmt->kind == NODE_EXPR_TUPLE_LITERAL ||
        stmt->kind == NODE_EXPR_SLICE || stmt->kind == NODE_TERNARY || stmt->kind == NODE_MATCH)
    {
        return 1;
    }
    return 0;
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

    // Control flow analysis: Check if non-void function always returns
    Type *ret_type = node->func.ret_type_info;
    int is_void = !ret_type || ret_type->kind == TYPE_VOID;

    // Special case: 'main' is allowed to fall off the end (C99 implicit return 0)
    int is_main = node->func.name && strcmp(node->func.name, "main") == 0;

    if (is_main && is_void)
    {
        warn_void_main(node->token);
    }

    if (!is_void && !is_main && node->func.body)
    {
        if (!block_always_returns(node->func.body))
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
        int prev_move_checks_only = tc->move_checks_only;
        tc->move_checks_only = 1; // suppress type errors
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

        tc->move_checks_only = prev_move_checks_only;
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
