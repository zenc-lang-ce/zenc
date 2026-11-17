// SPDX-License-Identifier: MIT
#include "typecheck_internal.h"
#include "../utils/utils.h"

int check_match_binding_move(TypeChecker *tc, ASTNode *rvalue)
{
    if (!rvalue || rvalue->kind != NODE_EXPR_VAR || !rvalue->type_info ||
        is_type_copy(tc->pctx, rvalue->type_info))
    {
        return 0;
    }
    ZenSymbol *sym = tc_lookup(tc, rvalue->var_ref.name);
    if (!sym || !sym->is_borrowed)
    {
        return 0;
    }
    const char *hints[] = {"The matched value keeps ownership of its payload and drops it",
                           "Use the binding in place, or bind it with 'ref' to borrow it", NULL};
    tc_move_error_with_hints(tc, rvalue->token, "Cannot move out of a match binding", hints, NULL);
    return 1;
}

void check_move_for_rvalue(TypeChecker *tc, ASTNode *rvalue)
{
    if (!rvalue || !rvalue->type_info)
    {
        return;
    }

    if (is_type_copy(tc->pctx, rvalue->type_info))
    {
        return;
    }

    if (rvalue->kind == NODE_EXPR_VAR)
    {
        ZenSymbol *sym = tc_lookup(tc, rvalue->var_ref.name);
        if (sym && !check_match_binding_move(tc, rvalue))
        {
            mark_symbol_moved(tc->pctx, sym, rvalue);
        }
    }
    else if (rvalue->kind == NODE_EXPR_UNARY && rvalue->unary.op == OP_DEREF)
    {
        const char *hints[] = {"This type owns resources and cannot be implicitly copied",
                               "Consider borrowing value via references or implementing Copy",
                               NULL};
        tc_move_error_with_hints(tc, rvalue->token, "Cannot move out of a borrowed reference",
                                 hints, NULL);
    }
    else if (rvalue->kind == NODE_EXPR_MEMBER)
    {
        // Now allowed, but will be tracked by path
        mark_symbol_moved(tc->pctx, NULL, rvalue);
    }
    else if (rvalue->kind == NODE_EXPR_INDEX)
    {
        const char *hints[] = {"Cannot move an element out of an array or slice.", NULL};
        tc_move_error_with_hints(tc, rvalue->token, "Cannot move out of an index expression", hints,
                                 NULL);
    }
}

// What an arithmetic or bitwise operator works on: the element of a vector (it applies lane by
// lane), or, for an opaque alias of a number, that number in the file that defines it.
static Type *operand_scalar(TypeChecker *tc, Type *t, Token at)
{
    Type *vector = vector_type_of(tc->pctx, t);
    return vector ? vector->inner : type_seen_from(t, at.file);
}

// The pointer `t` is in arithmetic: itself if it is a pointer (or a string), the pointer to its
// first element if it is an array, which decays to it as in C. NULL for anything else.
static Type *decayed_pointer(Type *t)
{
    if (is_pointer_type(t))
    {
        return t;
    }
    Type *resolved = resolve_alias(t);
    if (resolved && resolved->kind == TYPE_ARRAY && resolved->inner)
    {
        Type *pointer = type_new_ptr(resolved->inner);
        pointer->lifetime_depth = t->lifetime_depth;
        return pointer;
    }
    return NULL;
}

// An array of bytes C lets a string literal initialize: of char, signed or unsigned.
static int is_byte_array(Type *t)
{
    if (!t || t->kind != TYPE_ARRAY || !t->inner)
    {
        return 0;
    }
    switch (resolve_alias(t->inner)->kind)
    {
    case TYPE_CHAR:
    case TYPE_C_CHAR:
    case TYPE_C_UCHAR:
    case TYPE_I8:
    case TYPE_U8:
    case TYPE_BYTE:
        return 1;
    default:
        return 0;
    }
}

// `void*`, the generic pointer: it converts to and from any pointer, and compares with one.
static int is_void_pointer(Type *t)
{
    t = resolve_alias(t);
    return t && t->kind == TYPE_POINTER && t->inner && resolve_alias(t->inner)->kind == TYPE_VOID;
}

void check_expr_unary(TypeChecker *tc, ASTNode *node, int depth)
{
    // `&&label` (GNU C) is the address of a label, a void*: its operand names a label, not a value.
    if (node->unary.op == OP_PREFIX_AND_AND)
    {
        Type *void_pointer = type_new(TYPE_POINTER);
        void_pointer->inner = type_new(TYPE_VOID);
        node->type_info = void_pointer;
        return;
    }

    check_node(tc, node->unary.operand, depth + 1);

    Type *operand_type = node->unary.operand->type_info;
    OperatorKind unop = node->unary.op;

    if (!operand_type)
    {
        return;
    }

    // Logical NOT: !
    if (unop == OP_NOT)
    {
        node->type_info = type_new(TYPE_BOOL);
        return;
    }

    // Numeric negation: -
    if (unop == OP_NEG)
    {
        Type *scalar = operand_scalar(tc, operand_type, node->token);
        if (!is_integer_type(scalar) && !is_float_type(scalar))
        {
            const char *hints[] = {"Negation requires a numeric operand", NULL};
            tc_error_with_hints(tc, node->token, "Cannot negate non-numeric type", hints);
        }
        else
        {
            node->type_info = operand_type;
        }
        return;
    }

    // Dereference: *
    if (unop == OP_DEREF)
    {
        if (operand_type->kind == TYPE_UNKNOWN)
        {
            node->type_info = type_new(TYPE_UNKNOWN);
            return;
        }

        Type *pointee = pointee_of(operand_type);
        if (!is_pointer_type(operand_type))
        {
            const char *hints[] = {"Only pointers can be dereferenced", NULL};
            tc_error_with_hints(tc, node->token, "Cannot dereference non-pointer type", hints);
        }
        else if (pointee)
        {
            node->type_info = pointee;
            CompilerConfig *cfg = &tc->pctx->compiler->config;
            if (cfg->misra_mode)
            {
                misra_check_file_dereference(tc->pctx, operand_type, node->token);
            }
        }
        return;
    }

    // Bitwise NOT: ~
    if (unop == OP_BIT_NOT)
    {
        if (!is_integer_type(operand_scalar(tc, operand_type, node->token)))
        {
            const char *hints[] = {"Bitwise NOT requires an integer operand", NULL};
            tc_error_with_hints(tc, node->token, "Cannot apply ~ to non-integer type", hints);
        }
        else
        {
            misra_check_bitwise_operand(tc->pctx, node->unary.operand->type_info, node->token);
            node->type_info = operand_type;
        }
        return;
    }

    if (unop == OP_PRE_INC || unop == OP_PRE_DEC || unop == OP_POST_INC || unop == OP_POST_DEC)
    {
        misra_check_inc_dec_result_used(tc->pctx, node->token);
        // Track as a write
        if (node->unary.operand->kind == NODE_EXPR_VAR)
        {
            ZenSymbol *s = tc_lookup(tc, node->unary.operand->var_ref.name);
            if (s)
            {
                s->is_written_to = 1;
            }
        }
        else if (node->unary.operand->kind == NODE_EXPR_UNARY &&
                 node->unary.operand->unary.op == OP_DEREF)
        {
            ASTNode *inner = node->unary.operand->unary.operand;
            if (inner->kind == NODE_EXPR_VAR)
            {
                ZenSymbol *s = tc_lookup(tc, inner->var_ref.name);
                if (s)
                {
                    s->is_written_to = 1;
                }
            }
        }
        if (!is_integer_type(operand_type) && !is_float_type(operand_type) &&
            !is_pointer_type(operand_type))
        {
            tc_error(tc, node->token, "Increment/decrement requires a numeric or pointer operand");
        }
        node->type_info = operand_type;
        return;
    }

    // Address-of: &
    if (unop == OP_ADDRESS || unop == OP_ADDRESS_RVALUE)
    {
        node->type_info = type_new_ptr(operand_type);
        // The address points where its operand lives.
        if (unop == OP_ADDRESS)
        {
            node->type_info->lifetime_depth = tc_storage_depth(tc, node->unary.operand);
        }
        else
        {
            // R-values are temporaries, their life is the current code block
            if (node->unary.operand->type_info)
            {
                node->type_info->lifetime_depth = node->unary.operand->type_info->lifetime_depth;
            }
            else
            {
                node->type_info->lifetime_depth = tc->current_depth;
            }
        }
        return;
    }
}

// MISRA Rule 12.4: a constant `+`, `-` or `*`, the binary `node`, must not wrap around in the
// unsigned type `target` it is evaluated in.
static void misra_check_constant_wrap(TypeChecker *tc, ASTNode *node, Type *target)
{
    OperatorKind op = node->binary.op;
    if (!tc->pctx->config->misra_mode || !target || !is_integer_type(target) ||
        (op != OP_ADD && op != OP_SUB && op != OP_MUL))
    {
        return;
    }
    long long lval, rval;
    if (!eval_const_int_expr(node->binary.left, tc->pctx, &lval) ||
        !eval_const_int_expr(node->binary.right, tc->pctx, &rval))
    {
        return;
    }
    long long res = op == OP_ADD ? lval + rval : op == OP_SUB ? lval - rval : lval * rval;
    misra_check_unsigned_wrap(tc->pctx, op, lval, rval, res, target, node->token);
}

// The variable whose own storage an assignment to `place` writes: `v`, or `v` for a field or an
// element of it, at any depth. NULL when the place is reached through a pointer (`*p`, `p.f`
// with a pointer `p`), which writes where the pointer points.
static ASTNode *written_variable(ASTNode *place)
{
    while (place && (place->kind == NODE_EXPR_MEMBER || place->kind == NODE_EXPR_INDEX))
    {
        ASTNode *holder =
            place->kind == NODE_EXPR_MEMBER ? place->member.target : place->index.array;
        if (!holder || is_pointer_type(holder->type_info))
        {
            return NULL;
        }
        place = holder;
    }
    return place && place->kind == NODE_EXPR_VAR ? place : NULL;
}

// `v[i] = x` on a struct with a `set` method (index_method) is the call `v.set(i, x)`, the
// operator overload: the node becomes the call, which keeps the assignment as written. 0 when the
// assignment is not one.
static int rewrite_index_set(TypeChecker *tc, ASTNode *node, int depth)
{
    ASTNode *index = node->binary.left;
    check_node(tc, index->index.array, depth + 1);
    if (!index_method(tc->pctx, index->index.array->type_info, "set"))
    {
        return 0;
    }

    ASTNode *callee = ast_create(NODE_EXPR_MEMBER);
    callee->token = node->token;
    callee->member.target = index->index.array;
    callee->member.field = xstrdup("set");

    // Arguments: the index, the extra ones (`v[i, j] = x`), then the value. The index nodes are
    // copied into the list: `v[i] += x` is `v[i] = v[i] + x`, whose right side shares them and
    // chains them into its own call to `get`.
    ASTNode *args = NULL;
    ASTNode **tail = &args;
    ASTNode *first_extra = index->index.extra_indices;
    for (ASTNode *i = index->index.index; i; i = (i == index->index.index) ? first_extra : i->next)
    {
        ASTNode *copy = ast_create(i->kind);
        *copy = *i;
        copy->next = NULL;
        *tail = copy;
        tail = &copy->next;
    }
    *tail = node->binary.right;
    node->binary.right->next = NULL;

    ASTNode *written = ast_turn_into(node, NODE_EXPR_CALL);
    node->call.callee = callee;
    node->call.args = args;
    node->call.written = written;
    check_expr_call(tc, node, depth + 1);
    return 1;
}

void check_expr_binary(TypeChecker *tc, ASTNode *node, int depth)
{
    OperatorKind binop = node->binary.op;
    const char *op = operator_spelling(binop);
    OperatorClass op_class = operator_class(binop);

    if (binop == OP_ASSIGN && node->binary.left->kind == NODE_EXPR_INDEX &&
        rewrite_index_set(tc, node, depth))
    {
        return;
    }

    if (is_assignment_operator(binop))
    {
        if (!tc->is_stmt_context)
        {
            misra_check_assignment_result_used(tc->pctx, node->token);
        }

        int old_is_assign_lhs = tc->is_assign_lhs;
        int old_stmt_ctx = tc->is_stmt_context;
        tc->is_assign_lhs = 1;
        tc->is_stmt_context = 0;
        check_node(tc, node->binary.left, depth + 1);
        tc->is_assign_lhs = old_is_assign_lhs;

        if (node->binary.left->type_info && node->binary.right->kind == NODE_LAMBDA)
        {
            node->binary.right->type_info = node->binary.left->type_info;
        }
        check_node(tc, node->binary.right, depth + 1);
        tc->is_stmt_context = old_stmt_ctx;

        // Rule 13.2: Side effect collision detection for assignments
        if (tc->pctx->config->misra_mode)
        {
            SymbolSet l_reads = {0}, l_writes = {0};
            SymbolSet r_reads = {0}, r_writes = {0};
            collect_symbols(node->binary.left, &l_reads, &l_writes);
            collect_symbols(node->binary.right, &r_reads, &r_writes);

            // Treat LHS of assignment as a write
            if (node->binary.left->kind == NODE_EXPR_VAR)
            {
                int already_in = 0;
                for (int k = 0; k < l_writes.count; k++)
                {
                    if (l_writes.syms[k] == node->binary.left->var_ref.symbol)
                    {
                        already_in = 1;
                        break;
                    }
                }
                if (!already_in && l_writes.count < 32)
                {
                    l_writes.syms[l_writes.count++] = node->binary.left->var_ref.symbol;
                }
            }

            // Double write collision: i = i++
            int match = 0;
            for (int i = 0; i < l_writes.count; i++)
            {
                ZenSymbol *s = l_writes.syms[i];
                if (!s)
                {
                    continue;
                }
                for (int j = 0; j < r_writes.count; j++)
                {
                    if (s == r_writes.syms[j])
                    {
                        tc_error(tc, node->token,
                                 "MISRA Rule 13.2: symbol modified multiple times in assignment");
                        match = 1;
                        break;
                    }
                }
                if (match)
                {
                    break;
                }

                // Also check if LHS write conflicts with RHS reads (if RHS also writes it)
                // e.g. i = i++ is already covered by double write.
                // But what about i = i + (i++)?
                // r_writes={i}, r_reads={i, i}.
            }
        }
    }
    else
    {
        int old_stmt_ctx = tc->is_stmt_context;
        tc->is_stmt_context = 0;
        check_node(tc, node->binary.left, depth + 1);
        check_node(tc, node->binary.right, depth + 1);
        tc->is_stmt_context = old_stmt_ctx;

        // Rule 13.2: Side effect collision detection for binary operators
        if (binop != OP_AND && binop != OP_OR)
        {
            check_side_effect_collision(tc, node->binary.left, node->binary.right, node->token);
        }
        else if (tc->pctx->config->misra_mode && (binop == OP_AND || binop == OP_OR))
        {
            // Rule 13.5: The RHS of && or || shall not contain persistent side effects
            SymbolSet r_reads = {0}, r_writes = {0};
            collect_symbols(node->binary.right, &r_reads, &r_writes);
            if (r_writes.count > 0)
            {
                tc_error(tc, node->binary.right->token,
                         "MISRA Rule 13.5: persistent side effect in logical RHS");
            }
        }
    }

    // Rule 19.1: Check for self-assignment (x = x)
    if (tc->pctx->config->misra_mode && binop == OP_ASSIGN)
    {
        misra_check_assignment_overlap(tc->pctx, node->binary.left, node->binary.right,
                                       node->token);
    }

    // Malformed input can produce binary expressions with NULL children
    if (!node->binary.left || !node->binary.right)
    {
        return;
    }
    Type *left_type = node->binary.left->type_info;
    Type *right_type = node->binary.right->type_info;

    // Assignment Logic for Moves (and type compatibility)
    if (binop == OP_ASSIGN)
    {
        // Check type compatibility for assignment
        if (left_type && right_type)
        {
            apply_implicit_struct_pointer_conversions(tc, &node->binary.right, left_type);
            right_type = node->binary.right->type_info;
            // The value must not point into a scope shorter than the place it is stored in.
            Type *place_type = type_clone(left_type);
            place_type->lifetime_depth = tc_storage_depth(tc, node->binary.left);
            check_type_compatibility(tc, place_type, right_type, node->binary.right->token,
                                     node->binary.right, 0);
            // A variable points as deep as any value assigned to it, also to a field or an
            // element of it that is part of it (not reached through a pointer). Its symbol gets
            // its own copy of the type: a parameter's is the signature's.
            ASTNode *written = written_variable(node->binary.left);
            if (written && right_type)
            {
                ZenSymbol *var = tc_lookup(tc, written->var_ref.name);
                if (var && var->type_info &&
                    right_type->lifetime_depth > var->type_info->lifetime_depth)
                {
                    var->type_info = type_clone(var->type_info);
                    var->type_info->lifetime_depth = right_type->lifetime_depth;
                }
            }

            // Mark LHS as written to
            if (node->binary.left->kind == NODE_EXPR_VAR)
            {
                ZenSymbol *s = tc_lookup(tc, node->binary.left->var_ref.name);
                if (s)
                {
                    s->is_written_to = 1;
                }
            }
            else if (node->binary.left->kind == NODE_EXPR_UNARY &&
                     node->binary.left->unary.op == OP_DEREF)
            {
                ASTNode *inner = node->binary.left->unary.operand;
                if (inner->kind == NODE_EXPR_VAR)
                {
                    ZenSymbol *s = tc_lookup(tc, inner->var_ref.name);
                    if (s)
                    {
                        s->is_written_to = 1;
                    }
                }
            }
            // Also handle array indexing as write
            else if (node->binary.left->kind == NODE_EXPR_INDEX)
            {
                ASTNode *arr = node->binary.left->index.array;
                if (arr->kind == NODE_EXPR_VAR)
                {
                    ZenSymbol *s = tc_lookup(tc, arr->var_ref.name);
                    if (s)
                    {
                        s->is_written_to = 1;
                    }
                }
            }
            // Also handle member access as write
            else if (node->binary.left->kind == NODE_EXPR_MEMBER)
            {
                ASTNode *target = node->binary.left->member.target;
                // Follow the member chain to the base variable
                while (target && target->kind == NODE_EXPR_MEMBER)
                {
                    target = target->member.target;
                }
                if (target && target->kind == NODE_EXPR_VAR)
                {
                    ZenSymbol *s = tc_lookup(tc, target->var_ref.name);
                    if (s)
                    {
                        s->is_written_to = 1;
                    }
                }
            }
        }

        // If RHS is moving a non-copy value, check validity and mark moved
        check_move_for_rvalue(tc, node->binary.right);

        // LHS is being (re-)initialized, so it becomes Valid.
        if (node->binary.left->kind == NODE_EXPR_VAR)
        {
            ZenSymbol *lhs_sym = tc_lookup(tc, node->binary.left->var_ref.name);
            if (lhs_sym)
            {
                if (tc->pctx->config->misra_mode && node->binary.left &&
                    node->binary.left->kind == NODE_EXPR_VAR)
                {
                    misra_check_param_modified(tc->current_func, node->binary.left,
                                               node->binary.left->token);
                }
                if (lhs_sym->is_immutable)
                {
                    tc_error(tc, node->binary.left->token, "Cannot assign to immutable variable");
                }

                if (tc->pctx->config->misra_mode)
                {
                    misra_check_pointer_conversion(tc->pctx, left_type, right_type, node->token);
                }
                mark_symbol_valid(tc->pctx, lhs_sym, node->binary.left);
            }
        }

        // Result type is same as LHS
        node->type_info = left_type;
        return;
    }

    // Arithmetic operators: +, -, *, /, %
    if (binop == OP_ADD || binop == OP_SUB || binop == OP_MUL || binop == OP_DIV || binop == OP_MOD)
    {
        // Division by a literal zero (`0` or `0.0`). The operation decides: with a float operand
        // C divides as floats, also by an integer `0`, and the result is defined (IEEE 754:
        // infinity or NaN, as `1.0 / 0.0` spells infinity); between integers it is undefined.
        if ((binop == OP_DIV || binop == OP_MOD) && node->binary.right &&
            node->binary.right->kind == NODE_EXPR_LITERAL)
        {
            ASTNode *divisor = node->binary.right;
            int is_zero = (divisor->literal.kind == LITERAL_INT && divisor->literal.int_val == 0) ||
                          (divisor->literal.kind == LITERAL_FLOAT &&
                           divisor->literal.float_val >= -0.0 && divisor->literal.float_val <= 0.0);
            int is_float_operation = (left_type && is_float_type(resolve_alias(left_type))) ||
                                     (right_type && is_float_type(resolve_alias(right_type)));
            if (is_zero && is_float_operation)
            {
                tc_warning(tc, divisor->token,
                           "Float division by zero: the result is infinity or NaN");
            }
            else if (is_zero)
            {
                const char *hints[] = {"Division by zero is undefined behavior", NULL};
                tc_error_with_hints(tc, divisor->token, "Division by zero detected", hints);
            }
        }

        if (left_type && right_type)
        {
            Type *lhs_resolved = resolve_alias(left_type);
            Type *rhs_resolved = resolve_alias(right_type);

            // Pointer Arithmetic. An array is the pointer to its first element it decays to.
            Type *lhs_pointer = decayed_pointer(left_type);
            Type *rhs_pointer = decayed_pointer(right_type);
            if (lhs_pointer)
            {
                misra_check_pointer_arithmetic(tc->pctx, lhs_resolved, node->token);

                // Ptr - Ptr -> isize
                if (binop == OP_SUB && rhs_pointer)
                {
                    node->type_info = type_new(TYPE_ISIZE);
                    return;
                }
                // Ptr + Int -> Ptr
                // Ptr - Int -> Ptr
                if ((binop == OP_ADD || binop == OP_SUB) && is_integer_type(rhs_resolved))
                {
                    node->type_info = lhs_pointer;
                    return;
                }
            }
            // Int + Ptr -> Ptr
            if (binop == OP_ADD && is_integer_type(lhs_resolved) && rhs_pointer)
            {
                misra_check_pointer_arithmetic(tc->pctx, rhs_resolved, node->token);
                node->type_info = rhs_pointer;
                return;
            }

            Type *left_scalar = operand_scalar(tc, left_type, node->token);
            Type *right_scalar = operand_scalar(tc, right_type, node->token);
            Type *left_vector = vector_type_of(tc->pctx, left_type);
            Type *right_vector = vector_type_of(tc->pctx, right_type);
            int left_numeric = is_integer_type(left_scalar) || is_float_type(left_scalar);
            int right_numeric = is_integer_type(right_scalar) || is_float_type(right_scalar);

            if (!left_numeric || !right_numeric)
            {
                if (left_type->kind == TYPE_UNKNOWN || right_type->kind == TYPE_UNKNOWN)
                {
                    node->type_info = type_new(TYPE_UNKNOWN);
                    return;
                }

                char msg[MAX_SHORT_MSG_LEN];
                snprintf(msg, sizeof(msg), "Operator '%s' requires numeric operands", op);
                const char *hints[] = {
                    "Arithmetic operators can only be used with integer, float, or vector types",
                    NULL};
                tc_error_with_hints(tc, node->token, msg, hints);
            }
            else if (left_vector || right_vector)
            {
                if (left_vector != right_vector)
                {
                    tc_error(tc, node->token,
                             "Vector operation requires operands of same vector type");
                }
                node->type_info = left_type;
                return;
            }
            else
            {
                // Rule 10.4: Balancing
                misra_check_binary_op_essential_types(tc->pctx, node->binary.left,
                                                      node->binary.right, node->token);

                // MISRA Rule 12.4, in the operands' type; in the type of its context, where the
                // constant takes it (check_type_compatibility).
                misra_check_constant_wrap(tc, node, left_type);

                // Rule 10.2: Character arithmetic
                misra_check_char_arithmetic(tc->pctx, left_type, right_type, binop, node->token);

                // Result type: Only infer if not already set by context
                if (!node->type_info)
                {
                    if (is_float_type(left_type) || is_float_type(right_type))
                    {
                        node->type_info = type_new(TYPE_F64);
                    }
                    else if (is_integer_type(left_type) && is_integer_type(right_type))
                    {
                        node->type_info =
                            integer_operation_type(node->binary.left, node->binary.right);
                    }
                    else
                    {
                        node->type_info = left_type;
                    }
                }
            }
        }
        return;
    }

    // Comparison operators: ==, !=, <, >, <=, >=
    if (op_class == OPERATOR_COMPARISON)
    {
        // Result is always bool
        node->type_info = type_new(TYPE_BOOL);

        // Operands should be comparable
        if (left_type && right_type)
        {
            // Rule 10.4: Balancing
            misra_check_binary_op_essential_types(tc->pctx, node->binary.left, node->binary.right,
                                                  node->token);

            // Rule 18.3: Relational operators on pointers (>, <, >=, <=)
            if (tc->pctx->config->misra_mode &&
                (binop == OP_LT || binop == OP_GT || binop == OP_LE || binop == OP_GE))
            {
                Type *l_resolved = resolve_alias(left_type);
                Type *r_resolved = resolve_alias(right_type);
                if (l_resolved->kind == TYPE_POINTER || r_resolved->kind == TYPE_POINTER)
                {
                    tc_error(
                        tc, node->token,
                        "MISRA Rule 18.3: relational operator shall not be applied to pointers");
                }
            }

            misra_check_string_compare(tc->pctx, left_type, right_type, node->token);

            if (!type_eq(left_type, right_type))
            {
                // Allow comparison between numeric types, an opaque alias of a number
                // among them in the file that defines it.
                Type *left_seen = type_seen_from(left_type, node->token.file);
                Type *right_seen = type_seen_from(right_type, node->token.file);
                int left_numeric = is_integer_type(left_seen) || is_float_type(left_seen);
                int right_numeric = is_integer_type(right_seen) || is_float_type(right_seen);
                Type *left_resolved = resolve_alias(left_type);
                Type *right_resolved = resolve_alias(right_type);
                int pointer_and_void_pointer =
                    (is_void_pointer(left_type) && right_resolved->kind == TYPE_POINTER) ||
                    (is_void_pointer(right_type) && left_resolved->kind == TYPE_POINTER);

                if ((!left_numeric || !right_numeric) && !pointer_and_void_pointer)
                {
                    if ((left_type && left_type->kind == TYPE_UNKNOWN) ||
                        (right_type && right_type->kind == TYPE_UNKNOWN))
                    {
                        node->type_info = type_new(TYPE_BOOL);
                        return;
                    }
                    char msg[MAX_SHORT_MSG_LEN];
                    snprintf(msg, sizeof(msg), "Cannot compare '%s' with incompatible types", op);
                    const char *hints[] = {"Ensure both operands have the same or compatible types",
                                           NULL};
                    tc_error_with_hints(tc, node->token, msg, hints);
                }
            }
        }
        return;
    }

    // Logical operators: &&, ||
    if (op_class == OPERATOR_LOGICAL)
    {
        node->type_info = type_new(TYPE_BOOL);
        // Could validate that operands are boolean-like, but C is lax here
        return;
    }

    // Bitwise operators: &, |, ^, <<, >>
    if (op_class == OPERATOR_BITWISE || op_class == OPERATOR_SHIFT)
    {
        // A constant shift amount must be below the width of what is shifted, as C requires: C
        // shifts a type narrower than int as an int. MISRA Rule 12.2 counts the type's own width.
        long long shift_amt;
        int width = integer_type_width(get_inner_type(left_type));
        int promoted = !tc->pctx->config->misra_mode && width > 0 && width < 32;
        if (promoted)
        {
            width = 32;
        }
        if (op_class == OPERATOR_SHIFT && width > 0 &&
            eval_const_int_expr(node->binary.right, tc->pctx, &shift_amt) &&
            (shift_amt < 0 || shift_amt >= width))
        {
            if (tc->pctx->config->misra_mode)
            {
                misra_check_shift_amount(tc->pctx, shift_amt, width, node->token);
            }
            else
            {
                char msg[MAX_SHORT_MSG_LEN];
                snprintf(msg, sizeof(msg), "Shift amount %lld is out of range for '%s' (%s%d bits)",
                         shift_amt, type_to_string(left_type), promoted ? "shifted as int, " : "",
                         width);
                const char *hints[] = {"Shifting by a negative amount or by the width of the type "
                                       "or more is undefined behavior",
                                       NULL};
                tc_error_with_hints(tc, node->binary.right->token, msg, hints);
            }
        }

        if (left_type && right_type)
        {
            Type *left_vector = vector_type_of(tc->pctx, left_type);
            Type *right_vector = vector_type_of(tc->pctx, right_type);
            if (!is_integer_type(operand_scalar(tc, left_type, node->token)) ||
                !is_integer_type(operand_scalar(tc, right_type, node->token)))
            {
                char msg[MAX_SHORT_MSG_LEN];
                snprintf(msg, sizeof(msg),
                         "Bitwise operator '%s' requires integers or vectors of integers", op);
                const char *hints[] = {
                    "Bitwise operators only work on integers and vectors of integers", NULL};
                tc_error_with_hints(tc, node->token, msg, hints);
            }
            else if (left_vector || right_vector)
            {
                if (left_vector != right_vector)
                {
                    tc_error(tc, node->token, "Vector bitwise operation requires same vector type");
                }
                node->type_info = left_type;
            }
            else
            {
                if (tc->pctx->config->misra_mode)
                {
                    misra_check_bitwise_operand(tc->pctx, left_type, node->token);
                    misra_check_bitwise_operand(tc->pctx, right_type, node->token);
                    // Rule 10.4: Balancing for &, |, ^
                    if (op_class == OPERATOR_BITWISE)
                    {
                        misra_check_binary_op_essential_types(tc->pctx, node->binary.left,
                                                              node->binary.right, node->token);
                    }
                }
                node->type_info = left_type;
            }
        }
        return;
    }
}

// Whether two struct types have the same name: `Vec<T>` and `Vec<int>` are both `Vec`.
static int is_struct_base_match(Type *base, Type *instantiated)
{
    return base && base->name && instantiated && instantiated->name &&
           strcmp(base->name, instantiated->name) == 0;
}

void apply_implicit_struct_pointer_conversions(TypeChecker *tc, ASTNode **expr_ptr,
                                               Type *expected_type)
{
    if (!expr_ptr || !*expr_ptr || !expected_type)
    {
        return;
    }
    ASTNode *expr = *expr_ptr;
    Type *actual_type = expr->type_info;
    if (!actual_type)
    {
        return;
    }

    Type *e_res = get_inner_type(expected_type);
    Type *a_res = get_inner_type(actual_type);

    // T* (actual) -> T (expected) => Implicit Dereference *
    // This allows `return self` to return the struct value when self is a pointer.
    // We only do this if the type is Copy, otherwise it would trigger a
    // move-from-borrowed-reference error.
    if (a_res->kind == TYPE_POINTER && a_res->inner &&
        (a_res->inner->kind == TYPE_STRUCT || a_res->inner->kind == TYPE_ENUM) &&
        (type_eq(a_res->inner, e_res) || is_struct_base_match(a_res->inner, e_res)) &&
        is_type_copy(tc->pctx, a_res->inner))

    {
        ASTNode *deref = ast_create(NODE_EXPR_UNARY);
        deref->unary.op = OP_DEREF;
        deref->unary.operand = expr;
        deref->type_info = a_res->inner;
        deref->token = expr->token;
        deref->next = expr->next;
        expr->next = NULL;
        *expr_ptr = deref;
    }
    // T (actual) -> T* (expected) => Implicit Address-Of &
    else if (e_res->kind == TYPE_POINTER && e_res->inner &&
             (a_res->kind == TYPE_STRUCT || a_res->kind == TYPE_ENUM) &&
             (type_eq(a_res, e_res->inner) || is_struct_base_match(a_res, e_res->inner)))
    {
        ASTNode *addr = ast_create(NODE_EXPR_UNARY);
        int is_rvalue = expr_is_rvalue(tc->pctx, expr);
        addr->unary.op = is_rvalue ? OP_ADDRESS_RVALUE : OP_ADDRESS;
        addr->unary.operand = expr;
        addr->type_info = e_res;
        addr->token = expr->token;
        addr->next = expr->next;
        expr->next = NULL;
        *expr_ptr = addr;
    }
}

// Wraps `node` in a conversion to `to`, in place: the node becomes the conversion and what it was
// moves to its child, so whoever points at it now points at the conversion. The child keeps its
// type, so checking the tree again gives the same tree.
static void wrap_in_conversion(ASTNode *node, Type *to)
{
    ASTNode *expr = ast_turn_into(node, NODE_IMPLICIT_CONVERSION);
    node->conversion.expr = expr;
    node->type_info = to;
}

// Views the array `node` as a slice of type `slice`: the node becomes a cast to the slice, whose
// operand is the array, so that codegen writes the conversion.
static void view_array_as_slice(ASTNode *node, Type *slice)
{
    ASTNode *array = ast_turn_into(node, NODE_EXPR_CAST);
    node->cast.expr = array;
    node->type_info = slice;
}

// Whether the operator `op` makes a constant that a `target` can hold: arithmetic for any number,
// bitwise operators and shifts for an integer.
static int is_constant_operator(OperatorKind op, Type *target)
{
    OperatorClass c = operator_class(op);
    if (c == OPERATOR_ARITHMETIC)
    {
        return 1;
    }
    return (c == OPERATOR_BITWISE || c == OPERATOR_SHIFT) && is_integer_type(target);
}

// Whether `node` is made only of untyped literals (`5`, `0.5`) under arithmetic and bitwise
// operators, and `target` can hold it: an integer literal any integer or float type, a float
// literal any float type.
static int is_untyped_constant(ASTNode *node, Type *target)
{
    switch (node->kind)
    {
    case NODE_EXPR_LITERAL:
        if (!node->literal.is_untyped)
        {
            return 0;
        }
        if (node->literal.kind == LITERAL_INT)
        {
            return is_integer_type(target) || is_float_type(target);
        }
        return node->literal.kind == LITERAL_FLOAT && is_float_type(target);
    case NODE_EXPR_UNARY:
        return is_constant_operator(node->unary.op, target) &&
               is_untyped_constant(node->unary.operand, target);
    case NODE_EXPR_BINARY:
        return is_constant_operator(node->binary.op, target) &&
               is_untyped_constant(node->binary.left, target) &&
               is_untyped_constant(node->binary.right, target);
    default:
        return 0;
    }
}

// Each element of the array literal `literal` against the element type `element`, one by one.
static int check_elements_against(TypeChecker *tc, Type *element, ASTNode *literal, int is_call_arg)
{
    for (ASTNode *elem = literal->array_literal.elements; elem; elem = elem->next)
    {
        if (!check_type_compatibility(tc, element, elem->type_info, elem->token, elem, is_call_arg))
        {
            return 0;
        }
    }
    return 1;
}

// An array, `value`, where the slice `target` is expected: each element must fit the slice's
// element type (an array literal's, one by one).
static int check_array_as_slice(TypeChecker *tc, Type *target, Type *value, Token t,
                                ASTNode *value_node, int is_call_arg)
{
    Type *slice_elem = slice_element(resolve_alias(target));
    if (value_node && value_node->kind == NODE_EXPR_ARRAY_LITERAL)
    {
        if (!check_elements_against(tc, slice_elem, value_node, is_call_arg))
        {
            return 0;
        }
        // Its elements took the slice's element type: so does the literal, an array of it.
        if (value_node->array_literal.elements)
        {
            Type *array = type_new(TYPE_ARRAY);
            array->inner = slice_elem;
            array->array_size = resolve_alias(value)->array_size;
            value_node->type_info = array;
        }
    }
    else if (!check_type_compatibility(tc, slice_elem, resolve_alias(value)->inner, t, NULL,
                                       is_call_arg))
    {
        return 0;
    }
    if (value_node)
    {
        view_array_as_slice(value_node, target);
    }
    return 1;
}

// Whether the integer constant `value` keeps its value in the integer type `t`.
// A type of 64 bits or more, or of unknown width, takes any constant: the folded
// value is a long long, which cannot tell -1 from 0xFFFFFFFFFFFFFFFF.
static int integer_constant_fits(long long value, Type *t)
{
    int width = integer_type_width(t);
    if (width <= 0 || width >= 64)
    {
        return 1;
    }
    if (is_signed_type(t))
    {
        long long limit = 1LL << (width - 1);
        return value >= -limit && value < limit;
    }
    return value >= 0 && value < (1LL << width);
}

// An integer `value` converted to the integer or float type `target`: it keeps its value when
// the target holds it, as a constant (by its value) or for every value of its type. When it may
// not, a warning says how it changes; MISRA reports Rule 10.3 instead.
static void check_integer_conversion(TypeChecker *tc, Type *target, Type *value,
                                     ASTNode *value_node, Token t)
{
    if (tc->pctx->config->misra_mode)
    {
        misra_check_implicit_conversion(tc->pctx, target, value, value_node, t);
        return;
    }
    Type *to = resolve_alias(target);
    int to_float = is_float_type(to);
    int target_width = integer_type_width(to);
    int value_width = integer_type_width(resolve_alias(value));
    long long constant;
    int is_constant = value_node && eval_const_int_expr(value_node, tc->pctx, &constant);
    int keeps =
        to_float
            ? (is_constant ? float_holds_integer(target, constant)
                           : float_holds_integer_type(target, value))
            : (is_constant ? integer_constant_fits(constant, to)
                           : !(target_width > 0 && value_width > 0 && target_width < value_width));
    if (keeps)
    {
        return;
    }

    char *t_str = zen_c_type_spelling(tc->pctx, target);
    char *v_str = zen_c_type_spelling(tc->pctx, value);
    char msg[MAX_SHORT_MSG_LEN];
    if (to_float)
    {
        snprintf(msg, sizeof(msg), "Implicit conversion from '%s' to '%s' rounds values above 2^%d",
                 v_str, t_str, float_significand_bits(target));
    }
    else if (is_constant)
    {
        snprintf(msg, sizeof(msg), "Integer constant %lld does not fit in '%s' (%d-bit)", constant,
                 t_str, target_width);
    }
    else
    {
        snprintf(msg, sizeof(msg),
                 "Implicit narrowing conversion from '%s' (%d-bit) to '%s' (%d-bit)", v_str,
                 value_width, t_str, target_width);
    }
    tc_warning(tc, t, msg);
    zfree(t_str);
    zfree(v_str);
}

int check_type_compatibility(TypeChecker *tc, Type *target, Type *value, Token t,
                             ASTNode *value_node, int is_call_arg)
{
    if (!target || !value)
    {
        return 1; // Can't check incomplete types
    }

    Type *resolved_target = resolve_alias(target);
    Type *resolved_value = resolve_alias(value);

    // A constant without a suffix (`5`, `-5`, `0.5`, `255 + 1`) takes the expected type when that
    // type can hold it, as C converts its value. Every context compares its value here, after
    // checking it, so the rule holds for all of them and no check of an expression has to keep a
    // type given beforehand. Its literals keep the type C computes them in (`1 << 40` is still a
    // shift of an `int`, which the checks of the operator saw).
    if (value_node && is_untyped_constant(value_node, get_inner_type(target)))
    {
        if (value_node->kind == NODE_EXPR_BINARY)
        {
            misra_check_constant_wrap(tc, value_node, target);
        }
        // Only when its representation changes: an `i32` constant where an `i32` (or an alias of
        // one) is expected is not converted.
        if (!value_node->type_info ||
            resolve_alias(value_node->type_info)->kind != resolve_alias(target)->kind)
        {
            wrap_in_conversion(value_node, target);
        }
        value = target;
        resolved_value = resolved_target;
    }

    // MISRA Pointer & Constant Checks (Rules 11.5, 11.9, etc.)
    if (tc->pctx->config->misra_mode && resolved_target->kind == TYPE_POINTER)
    {
        misra_check_null_pointer_constant(tc->pctx, value_node, t);
        misra_check_void_ptr_cast(tc->pctx, target, value, t);
        misra_check_pointer_conversion(tc->pctx, target, value, t);
    }

    // A literal 0 is a null pointer constant, as in C. MISRA mode reports it
    // above (Rule 11.9); otherwise it is accepted for any pointer.
    if (resolved_target->kind == TYPE_POINTER && value_node)
    {
        ASTNode *expr = value_node;
        while (expr && expr->kind == NODE_EXPR_CAST)
        {
            expr = expr->cast.expr;
        }
        if (expr && expr->kind == NODE_EXPR_LITERAL && expr->literal.kind == LITERAL_INT &&
            expr->literal.int_val == 0)
        {
            return 1;
        }
    }

    // An integer converts to an integer or a float without a cast, as in C (a float to an integer
    // does not: it drops the fraction, so it needs one). Before the type_eq fast path, which is
    // lax for integers.
    if (is_integer_type(resolved_value) &&
        (is_integer_type(resolved_target) || is_float_type(resolved_target)))
    {
        check_integer_conversion(tc, target, value, value_node, t);
        return 1;
    }

    if (!is_call_arg && resolved_target->kind == TYPE_POINTER &&
        resolved_value->kind == TYPE_POINTER)
    {
        // Higher depth = shorter scope.
        if (resolved_target->lifetime_depth < resolved_value->lifetime_depth)
        {
            char msg[MAX_ERROR_MSG_LEN];
            snprintf(msg, sizeof(msg),
                     "Escape analysis error: pointer assigned to a location that outlives it");
            const char *hints[] = {"The source pointer belongs to a child scope and will become "
                                   "invalid when that scope ends.",
                                   "Consider copying the value instead of taking a reference.",
                                   NULL};
            tc_escape_error(tc, t, msg, hints);
            return 0;
        }
    }

    // Fast path: exact match (the MISRA pointer checks ran above)
    if (type_eq(target, value))
    {
        return 1;
    }

    // In the file that defines it, an opaque alias converts to and from its underlying type. Two
    // opaque aliases are the same type only when they are the same alias, which type_eq tells.
    Type *target_inner = get_inner_type(target);
    Type *value_inner = get_inner_type(value);
    int both_opaque = target_inner->kind == TYPE_ALIAS && target_inner->alias.is_opaque_alias &&
                      value_inner->kind == TYPE_ALIAS && value_inner->alias.is_opaque_alias;
    if (!both_opaque && type_eq(type_seen_from(target, t.file), type_seen_from(value, t.file)))
    {
        return 1;
    }

    if (tc->pctx->config->misra_mode)
    {
        // Remaining pointer nesting and array param size checks are modularized
        // in their respective node visitors.
    }

    // String types: str, string, *char are compatible
    if ((target->kind == TYPE_STRING || (target->name && strcmp(target->name, "str") == 0)) &&
        (value->kind == TYPE_STRING || (value->name && strcmp(value->name, "string") == 0)))
    {
        return 1;
    }

    if (is_void_pointer(resolved_target) || is_void_pointer(resolved_value))
    {
        return 1;
    }

    // A string literal fills an array of characters it fits in, with its terminating NUL, as in
    // C (`let name: char[8] = "abc";`).
    if (value_node && is_string_literal(value_node) && is_byte_array(resolved_target))
    {
        char *text = string_literal_text(value_node);
        size_t needed = strlen(text) + 1;
        if (resolved_target->array_size > 0 && needed <= (size_t)resolved_target->array_size)
        {
            return 1;
        }
        char msg[MAX_SHORT_MSG_LEN];
        snprintf(msg, sizeof(msg), "A string of %zu bytes (with its NUL) does not fit in '%s'",
                 needed, type_to_string(target));
        const char *hints[] = {"Make the array larger, or the string shorter", NULL};
        tc_error_with_hints(tc, t, msg, hints);
        return 0;
    }

    // Array decay: Array[T] -> T* (a `string` is a char*).
    // This allows passing a fixed-size array where a pointer is expected.
    Type *target_pointee = pointee_of(resolved_target);
    if (target_pointee && resolved_value->kind == TYPE_ARRAY && resolved_value->inner)
    {
        // Recursive check for inner types (e.g. char* <- char[10])
        if (type_eq(target_pointee, resolved_value->inner))
        {
            return 1;
        }
        // Allow char* <- char[N] explicitly if type_eq is too strict
        if (is_char_type(target_pointee) && is_char_type(resolved_value->inner))
        {
            return 1;
        }
    }

    // An array literal where an array of its size is expected: each element against the element
    // type (a constant takes it), and the literal takes the array type.
    if (value_node && value_node->kind == NODE_EXPR_ARRAY_LITERAL &&
        resolved_target->kind == TYPE_ARRAY && resolved_value->kind == TYPE_ARRAY &&
        resolved_target->inner && resolved_target->array_size == resolved_value->array_size)
    {
        if (!check_elements_against(tc, resolved_target->inner, value_node, is_call_arg))
        {
            return 0;
        }
        value_node->type_info = target;
        return 1;
    }

    // Array to slice: a fixed-size array or an array literal where a slice is expected.
    Type *slice_elem = slice_element(resolved_target);
    if (slice_elem && resolved_value->kind == TYPE_ARRAY && resolved_value->inner)
    {
        return check_array_as_slice(tc, target, value, t, value_node, is_call_arg);
    }

    if (is_integer_type(resolved_target) && is_integer_type(resolved_value))
    {
        return 1;
    }

    // Float compatibility
    if (is_float_type(resolved_target) && is_float_type(resolved_value))
    {
        return 1;
    }

    // Trait object compatibility: Struct -> Trait
    if (resolved_target->name && is_trait(tc->pctx, type_def_name(resolved_target)) &&
        resolved_value->kind == TYPE_STRUCT && resolved_value->name)
    {
        if (check_impl(tc->pctx, type_def_name(resolved_target), type_def_name(resolved_value)))
        {
            return 1;
        }
    }

    // Type mismatch - report error
    char *t_str = zen_c_type_spelling(tc->pctx, target);
    char *v_str = zen_c_type_spelling(tc->pctx, value);

    char msg[MAX_ERROR_MSG_LEN];
    snprintf(msg, sizeof(msg), "Type mismatch: expected '%s', but found '%s'", t_str, v_str);

    const char *hints[] = {
        "Check if you need an explicit cast",
        "Ensure the types match exactly (no implicit conversions for strict types)", NULL};

    tc_error_with_hints(tc, t, msg, hints);
    zfree(t_str);
    zfree(v_str);
    return 0;
}

// A name no scope declares and that is not of C: an undefined variable or, where a call calls
// it, an undefined function.
static void report_undefined_name(TypeChecker *tc, ASTNode *node)
{
    const char *name = node->var_ref.name;
    char msg[MAX_SHORT_MSG_LEN];
    char help[MAX_SHORT_MSG_LEN];
    const char *hints[] = {NULL, NULL};
    snprintf(msg, sizeof(msg), "Undefined %s '%s'%s", tc->is_callee ? "function" : "variable", name,
             tc->pctx->config->misra_mode ? " (MISRA Rule 17.3)" : "");
    if (node->var_ref.suggestion)
    {
        snprintf(help, sizeof(help), "Did you mean '%s'?", node->var_ref.suggestion);
        hints[0] = help;
    }
    else if (tc->is_callee)
    {
        hints[0] = "Check if the function is defined or imported";
    }
    tc_error_with_hints(tc, node->token, msg, hints);
}

void check_expr_var(TypeChecker *tc, ASTNode *node)
{
    // A variant is written with its enum (`Color::Red`), and the parser records it in
    // var_ref.variant. A bare name (`Red`) is never looked up here as a variant: the parser
    // rejects a bare variant outside constant expressions, and reads the name as the variable,
    // parameter or function it names. A variant without payload is a value of the enum, not
    // a call to a constructor.
    TypeMember *variant = &node->var_ref.variant;
    EnumVariantReg *enum_variant = NULL;
    if (variant->member)
    {
        enum_variant =
            find_enum_variant_of(tc->pctx, type_def_name(variant->owner), variant->member);
    }
    if (enum_variant)
    {
        Type *enum_type = type_new_enum(enum_variant->enum_name);
        // Naming a variant uses its enum, as declaring something of its type does (MISRA
        // Rule 2.4 reports the types a program declares and never uses).
        mark_type_as_used(tc, enum_type);

        FuncSig *sig = find_func(tc->pctx, node->var_ref.name);
        // If it's a no-payload variant, treat it as an enum value (TYPE_ENUM)
        // If it has payloads, it's a constructor function (TYPE_FUNCTION)
        if (!sig || sig->total_args == 0)
        {
            node->type_info = enum_type;
            return;
        }
    }

    ZenSymbol *sym = tc_lookup(tc, node->var_ref.name);
    node->var_ref.symbol = sym; // Store for MISRA audits

    if (sym && sym->type_info)
    {
        // Clone the type to keep metadata (like lifetime_depth) isolated per usage site
        node->type_info = type_clone(sym->type_info);

        // Rule 8.9 tracking: Identify globals used in only one function
        if (!sym->is_local && sym->kind == SYM_VARIABLE && tc->current_func)
        {
            ZenSymbol *orig = sym->original ? sym->original : sym;
            if (orig->first_using_func == NULL)
            {
                orig->first_using_func = tc->current_func;
            }
            else if (orig->first_using_func != tc->current_func)
            {
                orig->multi_func_use = 1;
            }
        }
    }
    else
    {
        // Fallback: Check if it's a mangled function name (e.g. from :: operator or enum
        // constructor)
        FuncSig *sig = find_func(tc->pctx, node->var_ref.name);
        if (sig)
        {
            Type *fn_type = type_new(TYPE_FUNCTION);
            fn_type->is_raw = 1;
            fn_type->inner = sig->ret_type ? sig->ret_type : type_new(TYPE_VOID);
            fn_type->count = sig->total_args;
            if (sig->total_args > 0)
            {
                fn_type->args = xmalloc(sizeof(Type *) * (size_t)(sig->total_args));
                for (int i = 0; i < sig->total_args; i++)
                {
                    fn_type->args[i] = sig->arg_types[i];
                }
            }
            node->type_info = fn_type;
        }
        else
        {
            // A name of C the program uses undeclared (`stdin`, or a function declared by an
            // included header): a variable has the type its header declares.
            int is_c_name = should_suppress_undef_warning(tc->pctx, node->var_ref.name);
            Type *c_type = is_c_name ? c_variable_type(tc->pctx, node->var_ref.name) : NULL;
            if (c_type)
            {
                node->type_info = c_type;
            }
            else if (!is_c_name && !sym && !find_symbol_in_all(tc->pctx, node->var_ref.name))
            {
                report_undefined_name(tc, node);
            }
        }
    }

    if (!tc->is_assign_lhs)
    {
        check_use_validity(tc, node);
    }
}

// A lambda takes the function type `expected` before it is checked: check_expr_lambda reads it (a
// lambda for a `fn*` is emitted without a context). A constant takes its context's type in
// check_type_compatibility, after it is checked.
void adopt_context_type(ASTNode *value, Type *expected)
{
    if (value && expected && value->kind == NODE_LAMBDA &&
        get_inner_type(expected)->kind == TYPE_FUNCTION)
    {
        value->type_info = expected;
    }
}

void check_expr_literal(TypeChecker *tc, ASTNode *node)
{
    (void)tc;
    if (node->type_info && node->type_info->kind != TYPE_UNKNOWN)
    {
        return;
    }

    switch (node->literal.kind)
    {
    case LITERAL_INT:
        node->type_info = type_new(TYPE_I32); // Default to i32
        break;
    case LITERAL_FLOAT:
        node->type_info = type_new(TYPE_F64); // Default to f64
        break;
    case LITERAL_STRING:
    case LITERAL_RAW_STRING:
        node->type_info = type_new(TYPE_STRING);
        break;
    case LITERAL_CHAR:
        node->type_info = type_new(TYPE_CHAR);
        break;
    default:
        break;
    }
}

void check_struct_init(TypeChecker *tc, ASTNode *node, int depth)
{
    if (!node)
    {
        return;
    }
    RECURSION_GUARD_TOKEN(tc->pctx, node->token, );

    // MISRA: Mark struct type as used
    mark_type_as_used(tc, node->type_info);
    if (tc->pctx->config->misra_mode)
    {
        ZenSymbol *struct_sym =
            symbol_lookup_kind(tc->pctx->global_scope, node->struct_init.struct_name, SYM_STRUCT);
        if (struct_sym)
        {
            struct_sym->is_dereferenced = 1;
        }
    }

    // The struct the literal builds: the one its type names, also through an alias.
    Type *struct_type = resolve_alias(node->type_info);
    const char *struct_name = (struct_type && struct_type->name) ? type_def_name(struct_type)
                                                                 : node->struct_init.struct_name;
    ASTNode *def = find_struct_def(tc->pctx, struct_name);
    // A struct declared in a C header has no definition here: the scanner reads only its name,
    // so its fields are for the C compiler to check.
    int is_c_struct = !def && is_extern_symbol(tc->pctx, struct_name);
    if (!def && !is_c_struct)
    {
        char msg[MAX_SHORT_MSG_LEN];
        snprintf(msg, sizeof(msg), "Unknown struct '%s'", node->struct_init.struct_name);
        tc_error(tc, node->token, msg);
        RECURSION_EXIT(tc->pctx);
        return;
    }

    // The values of a vector literal are its lanes, whatever their names (`{v: x}` fills all).
    Type *vector = vector_type_named(tc->pctx, struct_name);

    // Iterate provided fields
    ASTNode *field_init = node->struct_init.fields;
    while (field_init)
    {
        // Rule 9.4: Double initialization check
        if (tc->pctx->config->misra_mode)
        {
            ASTNode *prev = node->struct_init.fields;
            while (prev != field_init)
            {
                if (strcmp(prev->var_decl.name, field_init->var_decl.name) == 0)
                {
                    misra_check_double_initialization(tc->pctx, field_init->var_decl.name,
                                                      field_init->token);
                    break;
                }
                prev = prev->next;
            }
        }

        // Find corresponding field in definition
        ASTNode *def_field = (def && !vector) ? def->strct.fields : NULL;
        Type *expected_type = vector ? vector->inner : NULL;
        int found = vector != NULL;

        while (def_field)
        {
            if (def_field->kind == NODE_FIELD &&
                strcmp(def_field->field.name, field_init->var_decl.name) == 0)
            {
                found = 1;
                expected_type = def_field->type_info;
                break;
            }
            def_field = def_field->next;
        }

        if (found)
        {
            field_init->type_info = expected_type;
        }

        if (found && expected_type && field_init->var_decl.init_expr->kind == NODE_LAMBDA)
        {
            field_init->var_decl.init_expr->type_info = expected_type;
        }

        // Check the initialization expression
        check_node(tc, field_init->var_decl.init_expr, depth + 1);

        if (!found)
        {
            if (!is_c_struct)
            {
                char msg[MAX_SHORT_MSG_LEN];
                snprintf(msg, sizeof(msg), "Struct '%s' has no field named '%s'",
                         node->struct_init.struct_name, field_init->var_decl.name);
                tc_error(tc, field_init->token, msg);
            }
        }
        else if (expected_type && field_init->var_decl.init_expr->type_info)
        {
            // Localize expected field type depth for initialization check.
            // A local struct's fields expect lifetimes compatible with the struct's own scope.
            Type *localized_expected = type_clone(expected_type);
            if (localized_expected && tc->current_depth > 0)
            {
                localized_expected->lifetime_depth = tc->current_depth;
            }
            check_type_compatibility(tc, localized_expected,
                                     field_init->var_decl.init_expr->type_info, field_init->token,
                                     field_init->var_decl.init_expr, 0);
        }

        // Move Analysis: Check if the initializer moves a non-copy value.
        check_move_for_rvalue(tc, field_init->var_decl.init_expr);

        field_init = field_init->next;
    }

    // A struct points where the deepest of its fields points. Its own copy of the type: the
    // struct's Type may be shared.
    int deepest = 0;
    for (ASTNode *fi = node->struct_init.fields; fi; fi = fi->next)
    {
        ASTNode *value = fi->var_decl.init_expr;
        if (value && value->type_info && value->type_info->lifetime_depth > deepest)
        {
            deepest = value->type_info->lifetime_depth;
        }
    }
    if (node->type_info && deepest != node->type_info->lifetime_depth)
    {
        node->type_info = type_clone(node->type_info);
        node->type_info->lifetime_depth = deepest;
    }

    // Fields left out of a struct literal are zero-initialised (the C designated
    // initializer the build emits), so they are only reported in MISRA mode,
    // which requires explicit initialization.
    ASTNode *def_field = (def && tc->pctx->config->misra_mode) ? def->strct.fields : NULL;
    while (def_field)
    {
        if (def_field->kind == NODE_FIELD && def_field->field.name)
        {
            int provided = 0;
            ASTNode *fi = node->struct_init.fields;
            while (fi)
            {
                if (fi->var_decl.name && strcmp(fi->var_decl.name, def_field->field.name) == 0)
                {
                    provided = 1;
                    break;
                }
                fi = fi->next;
            }
            if (!provided)
            {
                char msg[MAX_SHORT_MSG_LEN];
                snprintf(msg, sizeof(msg), "Missing field '%s' in initializer for struct '%s'",
                         def_field->field.name, node->struct_init.struct_name);
                const char *hints[] = {"All struct fields must be initialized", NULL};
                tc_error_with_hints(tc, node->token, msg, hints);
            }
        }
        def_field = def_field->next;
    }

    RECURSION_EXIT(tc->pctx);
}

// Whether every parameter of the lambda has a type: written, or taken from the function type the
// lambda is given or called with.
static int lambda_parameters_known(ASTNode *node)
{
    Type *t = get_inner_type(node->type_info);
    for (int i = 0; i < node->lambda.count; i++)
    {
        if (!t || t->kind != TYPE_FUNCTION || !t->args || i >= t->count || !t->args[i] ||
            get_inner_type(t->args[i])->kind == TYPE_UNKNOWN)
        {
            return 0;
        }
    }
    return 1;
}

static int lambda_is_waiting(TypeChecker *tc, ASTNode *node)
{
    for (int i = 0; i < tc->waiting_lambda_count; i++)
    {
        if (tc->waiting_lambdas[i].lambda == node)
        {
            return 1;
        }
    }
    return 0;
}

// A lambda whose parameters take their types from its first call (`let add = (x, y) => x + y;`):
// from 0.6 a parameter needs a type where nothing at the lambda gives it one.
static void report_untyped_lambda_parameter(TypeChecker *tc, ASTNode *node)
{
    Type *t = get_inner_type(node->type_info);
    int i = 0;
    while (i < node->lambda.count - 1 && t && t->kind == TYPE_FUNCTION && t->args && i < t->count &&
           t->args[i] && get_inner_type(t->args[i])->kind != TYPE_UNKNOWN)
    {
        i++;
    }
    const char *name = node->lambda.param_names[i];
    Token where = node->lambda.param_tokens ? node->lambda.param_tokens[i] : node->token;
    char msg[MAX_SHORT_MSG_LEN];
    snprintf(msg, sizeof(msg), "Lambda parameter '%s' has no type", name);
    const char *hints[] = {"Write the type of each parameter, as in '(x: int) => x * 2', or give "
                           "the lambda a function type, as in 'let f: fn(int) -> int = ...'",
                           NULL};
    tc_error_from_0_6(tc, where, msg, hints);
}

// Checks a lambda's body, in the scope that holds its parameters and captures.
static void check_lambda_body(TypeChecker *tc, ASTNode *node, int depth)
{
    MoveState *prev_move_state = tc->pctx->move_state;
    tc->pctx->move_state = move_state_create(NULL);

    int prev_unreachable = tc->is_unreachable;
    tc->is_unreachable = 0;
    LoopFrame *prev_loop = tc->current_loop; // A `break` cannot leave the lambda.
    tc->current_loop = NULL;

    // A `return` in the body is the lambda's, not the enclosing function's.
    ASTNode *prev_returns_from = tc->returns_from;
    tc->returns_from = node;

    if (node->lambda.body)
    {
        if (node->lambda.body->kind == NODE_BLOCK)
        {
            check_block(tc, node->lambda.body, depth + 1);
        }
        else
        {
            check_node(tc, node->lambda.body, depth + 1);
        }
    }

    move_state_free(tc->pctx->move_state);
    tc->pctx->move_state = prev_move_state;

    tc->returns_from = prev_returns_from;
    tc->is_unreachable = prev_unreachable;
    tc->current_loop = prev_loop;
}

void check_expr_lambda(TypeChecker *tc, ASTNode *node, int depth)
{
    Type *expected = get_inner_type(node->type_info);
    if (expected && expected->kind == TYPE_FUNCTION && expected->is_raw)
    {
        if (node->lambda.capture_count == 0)
        {
            node->lambda.is_bare = 1;
        }
        else
        {
            const char *hints[] = {
                "Only non-capturing lambdas can be converted to raw function pointers", NULL};
            tc_error_with_hints(tc, node->token,
                                "Cannot convert capturing lambda to raw function pointer", hints);
        }
    }

    if (node->lambda.captured_vars)
    {
        for (int i = 0; i < node->lambda.capture_count; i++)
        {
            char *var_name = node->lambda.captured_vars[i];
            int mode = node->lambda.capture_modes ? node->lambda.capture_modes[i]
                                                  : node->lambda.default_capture_mode;

            ZenSymbol *sym = tc_lookup(tc, var_name);
            if (!sym)
            {
                continue;
            }

            check_path_validity(tc, var_name, node->token);

            if (mode == 0)
            {
                Type *t = sym->type_info;
                if (!is_type_copy(tc->pctx, t))
                {
                    mark_symbol_moved(tc->pctx, sym, node);
                }
            }
        }
    }

    tc_enter_scope(tc);

    for (int i = 0; i < node->lambda.count; i++)
    {
        char *pname = node->lambda.param_names[i];
        Type *ptype = NULL;
        Type *node_ti = get_inner_type(node->type_info);
        if (node_ti && node_ti->kind == TYPE_FUNCTION && node_ti->args)
        {
            if (i < node_ti->count)
            {
                ptype = node_ti->args[i];
            }
        }
        tc_add_symbol(tc, pname, ptype, node->token, 0);
    }

    // Add captured variables to the scope of the lambda. One captured by value is the closure's own
    // copy, which the body may change (the closure keeps it between calls); the original does not
    // change. One captured by reference is the original.
    if (node->lambda.captured_vars)
    {
        int saved_silent = tc->pctx->silent_warnings;
        tc->pctx->silent_warnings = 1;
        for (int i = 0; i < node->lambda.capture_count; i++)
        {
            char *var_name = node->lambda.captured_vars[i];

            // Lookup original symbol to get its type
            ZenSymbol *orig_sym = tc_lookup(tc, var_name);
            if (orig_sym)
            {
                // Shadow the original variable in the lambda scope.
                tc_add_symbol(tc, var_name, orig_sym->type_info, node->token, 0);
            }
        }
        tc->pctx->silent_warnings = saved_silent;
    }

    // Without types for its parameters (`let add = (x, y) => x + y;`), the body waits for the
    // first call, which gives them: checked now, it would read `x + y` with unknown types. A
    // loop's second pass finds the lambda waiting already.
    if (!lambda_parameters_known(node))
    {
        if (!lambda_is_waiting(tc, node))
        {
            report_untyped_lambda_parameter(tc, node);
            WaitingLambda w = {node, tc->pctx->current_scope, tc->current_depth, depth,
                               tc->current_func};
            tc->waiting_lambdas =
                xrealloc(tc->waiting_lambdas,
                         (size_t)(tc->waiting_lambda_count + 1) * sizeof(WaitingLambda));
            tc->waiting_lambdas[tc->waiting_lambda_count++] = w;
        }
        tc_exit_scope(tc);
        return;
    }

    check_lambda_body(tc, node, depth);
    tc_exit_scope(tc);
}

// Checks the body of a lambda whose waiting has ended, in the lambda's own scope (its parameters
// and captures), wherever the checker is: the body sees the globals, but not what the enclosing
// scopes declared after the lambda.
static void check_waiting_lambda(TypeChecker *tc, WaitingLambda w)
{
    Scope *prev_scope = tc->pctx->current_scope;
    int prev_depth = tc->current_depth;
    ASTNode *prev_func = tc->current_func;
    int prev_rechecking = tc->rechecking;
    int prev_pass2 = tc->in_loop_pass2;
    int prev_assign_lhs = tc->is_assign_lhs;
    int prev_stmt = tc->is_stmt_context;
    int prev_forget = tc->is_forget_receiver;
    int prev_callee = tc->is_callee;

    // Every local of an enclosing scope the body names is a capture (the parser lists them), so
    // the lambda's scope needs only the globals behind it.
    w.scope->parent = tc->pctx->global_scope;
    tc->pctx->current_scope = w.scope;
    tc->current_depth = w.scope_depth;
    tc->current_func = w.func;
    tc->rechecking = 0; // The body's first check, even if the call is in a loop's second pass.
    tc->in_loop_pass2 = 0;
    tc->is_assign_lhs = 0;
    tc->is_stmt_context = 0;
    tc->is_forget_receiver = 0;
    tc->is_callee = 0;

    check_lambda_body(tc, w.lambda, w.node_depth);

    tc->pctx->current_scope = prev_scope;
    tc->current_depth = prev_depth;
    tc->current_func = prev_func;
    tc->rechecking = prev_rechecking;
    tc->in_loop_pass2 = prev_pass2;
    tc->is_assign_lhs = prev_assign_lhs;
    tc->is_stmt_context = prev_stmt;
    tc->is_forget_receiver = prev_forget;
    tc->is_callee = prev_callee;
}

// Checks the waiting lambdas that a call has given their parameters (`called`), or else those
// written in `func` (all of them if NULL). Checking a body can add or end other waits (a lambda
// in it, a call in it), so the search starts again after each one.
static void check_waiting_lambdas(TypeChecker *tc, int called, ASTNode *func)
{
    int i = 0;
    while (i < tc->waiting_lambda_count)
    {
        WaitingLambda w = tc->waiting_lambdas[i];
        int ready = called ? lambda_parameters_known(w.lambda) : (!func || w.func == func);
        if (!ready)
        {
            i++;
            continue;
        }
        tc->waiting_lambda_count--;
        memmove(&tc->waiting_lambdas[i], &tc->waiting_lambdas[i + 1],
                (size_t)(tc->waiting_lambda_count - i) * sizeof(WaitingLambda));
        check_waiting_lambda(tc, w);
        i = 0;
    }
}

void check_called_lambdas(TypeChecker *tc)
{
    check_waiting_lambdas(tc, 1, NULL);
}

void check_uncalled_lambdas(TypeChecker *tc, ASTNode *func)
{
    check_waiting_lambdas(tc, 0, func);
}

// INFERENCE & ENTRY POINTS
