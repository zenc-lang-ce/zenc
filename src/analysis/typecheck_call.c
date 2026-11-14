// SPDX-License-Identifier: MIT
#include "typecheck_internal.h"
#include "../utils/utils.h"

// Reports `value.method()` when the struct or enum of `value`, declared in Zen C, has no such
// method: there is no function for codegen to call. Reported in a build too, which would
// otherwise fail in the C compiler, on the mangled name.
static void report_missing_method(TypeChecker *tc, ASTNode *call)
{
    ASTNode *callee = call->call.callee;
    ASTNode *target = callee->member.target;
    // A field found is called through its value (a function pointer or a closure).
    if (!target || !target->type_info || !callee->type_info ||
        callee->type_info->kind != TYPE_UNKNOWN)
    {
        return;
    }
    // The receiver's type, as codegen takes it to name the method.
    Type *owner = target->type_info;
    if (owner->kind == TYPE_POINTER)
    {
        owner = owner->inner;
    }
    Type *t = get_inner_type(owner);
    if (!t || (t->kind != TYPE_STRUCT && t->kind != TYPE_ENUM) || !t->name)
    {
        return;
    }
    // A value of a trait calls through its table; a generic one is named for each instance.
    const char *name = type_def_name(t);
    ASTNode *def = find_struct_def(tc->pctx, name);
    if (!def || is_trait(name) || type_is_generic_dependent(tc->pctx, t) ||
        (def->kind == NODE_STRUCT && def->strct.is_template) ||
        (def->kind == NODE_ENUM && def->enm.is_template))
    {
        return;
    }
    const char *method_owner = NULL;
    char *method = NULL;
    if (find_method_call_target(tc->pctx, type_impl_name(owner), callee->member.field,
                                &method_owner, &method))
    {
        zfree(method);
        return;
    }
    char *spelled = zen_c_type_spelling(tc->pctx, t);
    char msg[MAX_SHORT_MSG_LEN];
    snprintf(msg, sizeof(msg), "No method '%s' on '%s'", callee->member.field, spelled);
    zfree(spelled);
    tc_ctrl_flow_error(tc, callee->token, msg, NULL);
}

void check_expr_call(TypeChecker *tc, ASTNode *node, int depth)
{
    check_node(tc, node->call.callee, depth + 1);

    const char *func_name = NULL;
    FuncSig *sig = NULL;

    // Check if the function exists (for simple direct calls)
    if (node->call.callee && node->call.callee->kind == NODE_EXPR_VAR)
    {
        func_name = node->call.callee->var_ref.name;

        // Look up function signature
        sig = find_func(tc->pctx, func_name);

        if (tc->pctx->config->misra_mode)
        {
            Token t = node->call.callee->token;
            if (t.line == 0)
            {
                t = node->token;
            }
            misra_check_banned_function(tc->pctx, func_name, t);
        }

        if (tc->pctx->config->misra_mode && tc->current_func)
        {
            if (strcmp(func_name, tc->current_func->func.name) == 0)
            {
                Token t = node->call.callee->token;
                if (t.line == 0)
                {
                    t = node->token;
                }
                misra_check_recursion(tc->pctx, t);
            }
        }

        if (!sig)
        {
            // A variant without a payload called as `Enum::Variant()`: it has no constructor
            // function, and check_expr_var already gave the callee the enum's type.
            Type *callee_type = node->call.callee->type_info;
            if (callee_type && callee_type->kind == TYPE_ENUM)
            {
                node->type_info = node->call.callee->type_info;
                return;
            }

            // panic() is the compiler's own: codegen calls __zenc_panic, which does not return.
            if (strcmp(func_name, "panic") == 0)
            {
                check_node(tc, node->call.args, depth + 1);
                node->type_info = type_new(TYPE_VOID);
                return;
            }

            // Check if it's a built-in macro injected by the compiler
            if (strcmp(func_name, "_z_str") == 0)
            {
                // _z_str is a generic format macro from ZC_C_GENERIC_STR
                check_node(tc, node->call.args, depth + 1); // Still check the argument
                node->type_info = type_new(TYPE_STRING);
                return;
            }

            // Check local scope first, then global symbols
            ZenSymbol *sym = tc_lookup(tc, func_name);

            // A C function, undeclared or one of the built-ins: the return type its header
            // declares, when it is known.
            if ((!sym || !sym->is_local) && should_suppress_undef_warning(tc->pctx, func_name))
            {
                Type *c_return = c_function_return_type(tc->pctx, func_name);
                if (c_return)
                {
                    node->type_info = c_return;
                }
            }

            if (!sym)
            {
                ZenSymbol *global_sym = find_symbol_in_all(tc->pctx, func_name);
                if (!global_sym && !should_suppress_undef_warning(tc->pctx, func_name))
                {
                    char msg[MAX_SHORT_MSG_LEN];
                    if (tc->pctx->config->misra_mode)
                    {
                        snprintf(msg, sizeof(msg), "Undefined function '%s' (MISRA Rule 17.3)",
                                 func_name);
                    }
                    else
                    {
                        snprintf(msg, sizeof(msg), "Undefined function '%s'", func_name);
                    }
                    const char *hints[] = {"Check if the function is defined or imported", NULL};
                    tc_error_with_hints(tc, node->call.callee->token, msg, hints);
                }
            }
        }
    }
    else if (node->call.callee && node->call.callee->kind == NODE_EXPR_MEMBER)
    {
        if (node->call.callee->type_info && node->call.callee->type_info->name)
        {
            func_name = node->call.callee->type_info->name;
            sig = find_func(tc->pctx, func_name);
        }

        // Trait method resolution fallback
        if (!sig && node->call.callee->member.target && node->call.callee->member.target->type_info)
        {
            Type *target_type = get_inner_type(node->call.callee->member.target->type_info);
            if (target_type->name && is_trait(type_def_name(target_type)))
            {
                ASTNode *trait_def = find_trait_def(tc->pctx, type_def_name(target_type));
                if (trait_def)
                {
                    ASTNode *method = trait_def->trait.methods;
                    while (method)
                    {
                        if (strcmp(method->func.name, node->call.callee->member.field) == 0)
                        {
                            // Correctly resolve return type for trait method
                            node->type_info = method->func.ret_type_info;
                            break;
                        }
                        method = method->next;
                    }
                }
            }
        }
        report_missing_method(tc, node);
    }

    // Count arguments
    int count = 0;
    ASTNode *arg = node->call.args;
    while (arg)
    {
        count++;
        arg = arg->next;
    }

    // Member call (a.b()) counts as +1 arg (the receiver)
    if (node->call.callee && node->call.callee->kind == NODE_EXPR_MEMBER)
    {
        count++;
    }

    // Enforce @pure constraint
    if (tc->current_func && tc->current_func->func.pure)
    {
        if (!sig || !sig->is_pure)
        {
            // Allow _z_str? Wait, _z_str is a compiler macro, it's not strictly "pure", but it's
            // safe.
            if (!func_name || strcmp(func_name, "_z_str") != 0)
            {
                char msg[MAX_SHORT_MSG_LEN];
                snprintf(msg, sizeof(msg),
                         "Pure function '%s' cannot call non-pure or dynamic function '%s'",
                         tc->current_func->func.name, func_name ? func_name : "unknown");
                const char *hints[] = {
                    "Mark the called function as @pure, or remove @pure from the caller", NULL};
                tc_error_with_hints(tc, node->call.callee->token, msg, hints);
            }
        }
    }

    // Validate argument count, against the registered function or, for a closure or function
    // pointer held in a variable, against the parameters of its type.
    int min_args = -1;
    int max_args = 0;
    int is_varargs = 0;
    Type *callee_type = node->call.callee ? node->call.callee->type_info : NULL;
    if (sig)
    {
        max_args = sig->total_args;
        is_varargs = sig->is_varargs;
        min_args = sig->total_args;
        if (sig->default_values)
        {
            min_args = 0;
            for (int i = 0; i < sig->total_args; i++)
            {
                if (!sig->default_values[i])
                {
                    min_args++;
                }
            }
        }
    }
    else if (node->call.callee && node->call.callee->kind == NODE_EXPR_VAR && callee_type &&
             callee_type->kind == TYPE_FUNCTION)
    {
        min_args = max_args = callee_type->count;
        is_varargs = callee_type->is_varargs;
    }

    if (min_args >= 0 && count < min_args)
    {
        char msg[MAX_SHORT_MSG_LEN];
        snprintf(msg, sizeof(msg), "Too few arguments: '%s' expects at least %d, got %d", func_name,
                 min_args, count);

        const char *hints[] = {"Check the function signature for required parameters", NULL};
        tc_ctrl_flow_error(tc, node->token, msg, hints);
    }
    else if (min_args >= 0 && count > max_args && !is_varargs)
    {
        char msg[MAX_SHORT_MSG_LEN];
        snprintf(msg, sizeof(msg), "Too many arguments: '%s' expects %d, got %d", func_name,
                 max_args, count);

        const char *hints[] = {
            "Remove extra arguments or check if you meant to call a different function", NULL};
        tc_ctrl_flow_error(tc, node->token, msg, hints);
    }

    // Check argument types
    arg = node->call.args;
    int sig_arg_idx = 0;

    // For member calls, the first signature argument is the receiver
    if (node->call.callee && node->call.callee->kind == NODE_EXPR_MEMBER)
    {
        if (sig && sig->total_args > 0 && sig->arg_types && sig->arg_types[0])
        {
            Type *expected_rec = sig->arg_types[0];
            Type *actual_rec = node->call.callee->member.target->type_info;

            // Allow T to T* for method receivers
            if (expected_rec->kind == TYPE_POINTER && actual_rec &&
                actual_rec->kind != TYPE_POINTER && type_eq(expected_rec->inner, actual_rec))
            {
                // OK: Compiler will take address
            }
            else
            {
                check_type_compatibility(tc, expected_rec, actual_rec, node->call.callee->token,
                                         NULL, 1);
            }
        }
        sig_arg_idx = 1;
    }

    // Method calls are lowered to direct calls with the receiver as the first
    // argument. .forget() is allowed on an already-moved receiver, so suppress
    // the use-after-move report for that call's arguments.
    size_t fn_len = func_name ? strlen(func_name) : 0;
    int forget_receiver_arg =
        func_name && ((fn_len >= 8 && strcmp(func_name + fn_len - 8, "__forget") == 0) ||
                      strcmp(func_name, "forget") == 0);
    if (forget_receiver_arg && node->call.callee && node->call.callee->kind != NODE_EXPR_MEMBER &&
        node->call.args)
    {
        tc->is_forget_receiver = 1;
    }

    while (arg)
    {
        Type *expected = NULL;
        if (sig && sig_arg_idx < sig->total_args && sig->arg_types && sig->arg_types[sig_arg_idx])
        {
            expected = sig->arg_types[sig_arg_idx];
        }
        else if (!sig && node->call.callee->type_info)
        {
            Type *callee_t = get_inner_type(node->call.callee->type_info);
            if (callee_t->kind == TYPE_FUNCTION && sig_arg_idx < callee_t->count && callee_t->args)
            {
                expected = callee_t->args[sig_arg_idx];
            }
        }

        // Propagate expected type to lambda for inference
        if (arg->kind == NODE_LAMBDA && expected)
        {
            arg->type_info = expected;
        }

        check_node(tc, arg, depth + 1);

        // Validate type against signature
        Type *actual = arg->type_info;
        if (expected && actual)
        {
            Type *e_resolved = get_inner_type(expected);
            Type *a_resolved = get_inner_type(actual);

            if (e_resolved->kind == TYPE_UNKNOWN && a_resolved->kind != TYPE_UNKNOWN)
            {
                // Backward type inference: we passed an actual type to a lambda taking unknown
                *e_resolved = *a_resolved;
            }
            else if (e_resolved->kind == TYPE_FUNCTION && a_resolved->kind == TYPE_FUNCTION)
            {
                for (int j = 0; j < e_resolved->count && j < a_resolved->count; j++)
                {
                    if (a_resolved->args && a_resolved->args[j] &&
                        a_resolved->args[j]->kind == TYPE_UNKNOWN && e_resolved->args &&
                        e_resolved->args[j] && e_resolved->args[j]->kind != TYPE_UNKNOWN)
                    {
                        *a_resolved->args[j] = *e_resolved->args[j];
                    }
                }
                if (a_resolved->inner && a_resolved->inner->kind == TYPE_UNKNOWN &&
                    e_resolved->inner)
                {
                    *a_resolved->inner = *e_resolved->inner;
                }
            }

            // Rule 17.5: Array parameter sizes must match.
            if (tc->pctx->config->misra_mode && e_resolved->kind == TYPE_ARRAY &&
                a_resolved->kind == TYPE_ARRAY)
            {
                if (e_resolved->array_size != a_resolved->array_size)
                {
                    misra_check_array_param_size(tc->pctx, e_resolved->array_size,
                                                 a_resolved->array_size, arg->token);
                }
            }

            check_type_compatibility(tc, expected, actual, arg->token, arg, 1);
        }

        // If argument is passed by VALUE, check if it can be moved.
        check_move_for_rvalue(tc, arg);

        arg = arg->next;
        sig_arg_idx++;
    }
    tc->is_forget_receiver = 0;

    // Propagate return type from function signature
    if (sig && sig->ret_type)
    {
        if (!node->type_info)
        {
            // Deep clone return type to ensure caller doesn't modify callee's metadata
            node->type_info = type_clone(sig->ret_type);
        }

        // Apply Lifetime Elision
        if (sig->elide_from_idx != -1 && node->type_info->kind == TYPE_POINTER)
        {
            int target_depth = 0; // Default to escaping if not found
            if (node->call.callee && node->call.callee->kind == NODE_EXPR_MEMBER &&
                sig->elide_from_idx == 0)
            {
                // `self` is the receiver's address: the result may point into the receiver
                // (where it lives) or where the receiver points.
                ASTNode *receiver = node->call.callee->member.target;
                if (receiver->type_info)
                {
                    target_depth = receiver->type_info->lifetime_depth;
                    Type *receiver_type = get_inner_type(receiver->type_info);
                    int lives_at = tc_storage_depth(tc, receiver);
                    if (receiver_type->kind != TYPE_POINTER && lives_at > target_depth)
                    {
                        target_depth = lives_at;
                    }
                }
            }
            else
            {
                int current_idx =
                    (node->call.callee && node->call.callee->kind == NODE_EXPR_MEMBER) ? 1 : 0;
                ASTNode *a = node->call.args;
                while (a)
                {
                    if (current_idx == sig->elide_from_idx)
                    {
                        if (a->type_info)
                        {
                            target_depth = a->type_info->lifetime_depth;
                        }
                        break;
                    }
                    current_idx++;
                    a = a->next;
                }
            }
            node->type_info->lifetime_depth = target_depth;
        }
        else
        {
            // Function results always have depth 0 (static/heap/escaping) by default
            node->type_info->lifetime_depth = 0;
        }
    }
    else if (!node->type_info && node->call.callee && node->call.callee->type_info)
    {
        Type *callee_t = get_inner_type(node->call.callee->type_info);
        if (callee_t->kind == TYPE_FUNCTION && callee_t->inner)
        {
            node->type_info = callee_t->inner;
        }
        else if (node->call.callee->kind == NODE_EXPR_MEMBER && callee_t->kind != TYPE_UNKNOWN)
        {
            // A method call: checking the member already gave it the method's return type.
            node->type_info = type_clone(node->call.callee->type_info);
        }
    }

    // Rule 17.7: Unused return values
    if (tc->pctx->config->misra_mode && tc->is_stmt_context && node->type_info)
    {
        misra_check_function_return_usage(tc->pctx, node);
    }

    // Rule 13.2: Side effect collision detection in arguments
    ASTNode *receiver = (node->call.callee && node->call.callee->kind == NODE_EXPR_MEMBER)
                            ? node->call.callee->member.target
                            : NULL;
    check_all_args_side_effects(tc, receiver, node->call.args, node->token);

    // Evaluation order check: function call arguments should not have conflicting side effects
    if (tc->pctx->config->misra_mode)
    {
        misra_check_evaluation_order(tc->pctx, node);
    }
}

// STATEMENT / BLOCK CHECKERS
