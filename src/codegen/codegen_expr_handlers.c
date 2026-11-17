// SPDX-License-Identifier: MIT
#include "../parser/parser.h"

#include "codegen.h"
#include "zprep.h"
#include "../constants.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../plugins/plugin_manager.h"
#include "ast.h"
#include "zprep_plugin.h"
#include "codegen_internal.h"

void handle_expr_struct_init(ParserContext *ctx, ASTNode *node)
{
    const char *struct_name = node->struct_init.struct_name;
    if (strcmp(struct_name, "Self") == 0 && ctx->cg.current_impl_type)
    {
        struct_name = ctx->cg.current_impl_type;
    }

    // The struct the literal builds, from its type: the name written in the C is its @link_name
    // if it has one.
    ASTNode *def = named_type_def(ctx, node->type_info);
    if (def && def->kind != NODE_STRUCT)
    {
        def = NULL;
    }
    const char *struct_c_name = def && def->link_name ? def->link_name : struct_name;
    int is_zen_struct = def != NULL;
    int is_union = def && def->strct.is_union;
    int is_vector = def && def->type_info && def->type_info->kind == TYPE_VECTOR;
    int vec_size = is_vector ? def->type_info->array_size : 0;

    int in_func = (ctx->cg.current_func_ret_type_info != NULL || ctx->cg.current_lambda != NULL);

    if (ctx->config->use_cpp)
    {
        if (in_func && !is_vector)
        {
            EMIT(ctx, "({ %s _s = %s; ", struct_c_name, ctx->config->use_cpp ? "{}" : "{0}");
            ASTNode *f = node->struct_init.fields;
            while (f)
            {
                int skip = 0;
                ASTNode *written = written_expr(f->var_decl.init_expr);
                if (written && written->kind == NODE_EXPR_LITERAL &&
                    written->literal.kind == LITERAL_INT && written->literal.int_val == 0)
                {
                    skip = 1;
                }
                if (!skip)
                {
                    if (f->var_decl.init_expr &&
                        f->var_decl.init_expr->kind == NODE_EXPR_ARRAY_LITERAL)
                    {
                        ASTNode *elem = f->var_decl.init_expr->array_literal.elements;
                        int idx = 0;
                        while (elem)
                        {
                            EMIT(ctx, "_s.%s[%d] = ", c_name(ctx, f->var_decl.name), idx++);
                            codegen_expression(ctx, elem);
                            EMIT(ctx, "; ");
                            elem = elem->next;
                        }
                    }
                    else
                    {
                        ASTNode *value = f->var_decl.init_expr;
                        Type *field_type = f->type_info ? resolve_alias(f->type_info) : NULL;
                        if (ctx->config->use_cpp && field_type && field_type->kind == TYPE_ARRAY &&
                            value->kind == NODE_EXPR_LITERAL &&
                            (value->literal.kind == LITERAL_STRING ||
                             value->literal.kind == LITERAL_RAW_STRING))
                        {
                            // C++ assigns no array, nor casts to one: a string fills a char
                            // array field by copy, cut to the field's size.
                            const char *field = c_name(ctx, f->var_decl.name);
                            EMIT(ctx, "strncpy(_s.%s, ", field);
                            codegen_expression(ctx, value);
                            EMIT(ctx, ", sizeof(_s.%s))", field);
                        }
                        else if (ctx->config->use_cpp)
                        {
                            const char *field = c_name(ctx, f->var_decl.name);
                            EMIT(ctx, "_s.%s = (__typeof__(_s.%s))(", field, field);
                            codegen_expression_with_move(ctx, f->var_decl.init_expr);
                            EMIT(ctx, ")");
                        }
                        else
                        {
                            EMIT(ctx, "_s.%s = ", c_name(ctx, f->var_decl.name));
                            codegen_expression_with_move(ctx, f->var_decl.init_expr);
                        }
                        EMIT(ctx, "; ");
                    }
                }
                f = f->next;
            }
            EMIT(ctx, "_s; })");
        }
        else
        {
            EMIT(ctx, "%s { ", struct_c_name);
            ASTNode *f = node->struct_init.fields;
            int field_count = 0;
            ASTNode *tmp = f;
            while (tmp)
            {
                field_count++;
                tmp = tmp->next;
            }

            if (is_vector && field_count == 1 && vec_size > 1)
            {
                for (int i = 0; i < vec_size; i++)
                {
                    if (i > 0)
                    {
                        EMIT(ctx, ", ");
                    }
                    codegen_expression(ctx, f->var_decl.init_expr);
                }
            }
            else
            {
                int first = 1;
                while (f)
                {
                    if (!first)
                    {
                        EMIT(ctx, ", ");
                    }
                    if (is_vector)
                    {
                        codegen_expression(ctx, f->var_decl.init_expr);
                    }
                    else
                    {
                        EMIT(ctx, ".%s = ", c_name(ctx, f->var_decl.name));
                        codegen_expression_with_move(ctx, f->var_decl.init_expr);
                    }
                    first = 0;
                    f = f->next;
                }
            }
            EMIT(ctx, " }");
        }
    }
    else
    {
        if (is_vector)
        {
            EMIT(ctx, "(%s){", struct_c_name);
        }
        else if (is_union)
        {
            EMIT(ctx, "(union %s){", struct_c_name);
        }
        else if (is_zen_struct)
        {
            EMIT(ctx, "(struct %s){", struct_c_name);
        }
        else
        {
            EMIT(ctx, "(%s){", struct_c_name);
        }

        ASTNode *f = node->struct_init.fields;
        int field_count = 0;
        ASTNode *tmp = f;
        while (tmp)
        {
            field_count++;
            tmp = tmp->next;
        }

        if (is_vector && field_count == 1 && vec_size > 1)
        {
            for (int i = 0; i < vec_size; i++)
            {
                if (i > 0)
                {
                    EMIT(ctx, ", ");
                }
                codegen_expression(ctx, f->var_decl.init_expr);
            }
        }
        else
        {
            int first = 1;
            while (f)
            {
                int skip = 0;
                ASTNode *written = written_expr(f->var_decl.init_expr);
                if (written && written->kind == NODE_EXPR_LITERAL &&
                    written->literal.kind == LITERAL_INT && written->literal.int_val == 0)
                {
                    skip = 1;
                }
                if (!skip)
                {
                    if (!first)
                    {
                        EMIT(ctx, ", ");
                    }
                    if (!is_vector)
                    {
                        EMIT(ctx, ".%s = ", c_name(ctx, f->var_decl.name));
                    }
                    codegen_expression_with_move(ctx, f->var_decl.init_expr);
                    first = 0;
                }
                f = f->next;
            }
            if (first)
            {
                EMIT(ctx, "0");
            }
        }
        EMIT(ctx, "}");
    }
}

// `left = right` with fixed-size arrays. C cannot assign an array, so `right` is copied into a
// temporary and the temporary's bytes into `left`; an array literal or a string literal
// initializes the temporary, since C copies only what the literal has and fills the rest with
// zeros (a `memcpy` of the array's size would read past the end of a shorter string). An array of
// values that need dropping is moved, in the order an assignment of a struct with Drop follows: the
// source is marked as moved out, and a variable's old value is dropped before it takes the new one
// (so `a = a` keeps its value).
static void emit_array_assignment(ParserContext *ctx, ASTNode *left, ASTNode *right)
{
    Type *t = left->type_info;
    char *c_type = c_type_spelling(ctx, t);
    int drops = type_needs_drop(ctx, t);

    EMIT(ctx, "({ ");
    emit_var_decl_type(ctx, c_type, "_z_src");
    if (right->kind == NODE_EXPR_ARRAY_LITERAL || is_string_literal(right))
    {
        EMIT(ctx, " = ");
        codegen_expression(ctx, right);
        EMIT(ctx, "; ");
    }
    else
    {
        EMIT(ctx, "; memcpy(_z_src, ");
        codegen_expression(ctx, right);
        EMIT(ctx, ", sizeof(%s)); ", c_type);
        if (drops && emit_moved_out(ctx, right))
        {
            EMIT(ctx, "; ");
        }
    }

    const char *flag_name = (drops && left->kind == NODE_EXPR_VAR) ? left->var_ref.name : NULL;
    if (flag_name)
    {
        EMIT(ctx, "if (__z_drop_flag_%s) { %s } ", flag_name,
             drop_value_stmt(t, c_name(ctx, flag_name), 0));
    }
    EMIT(ctx, "memcpy(");
    codegen_expression(ctx, left);
    EMIT(ctx, ", _z_src, sizeof(%s)); ", c_type);
    if (flag_name)
    {
        EMIT(ctx, "__z_drop_flag_%s = 1; ", flag_name);
    }
    EMIT(ctx, "})");
}

// `a ?? b`: `a` unless it is null or zero.
static void emit_coalesce(ParserContext *ctx, ASTNode *node)
{
    EMIT(ctx, "({ ");
    emit_auto_type(ctx, node->binary.left, node->token);
    EMIT(ctx, " _l = (");
    codegen_expression(ctx, node->binary.left);
    EMIT(ctx, "); _l ? _l : (");
    codegen_expression(ctx, node->binary.right);
    EMIT(ctx, "); })");
}

// `==` and `!=`: a struct compares through its `eq` (or `neq`), anything else as in C.
static void emit_equality(ParserContext *ctx, ASTNode *node)
{
    Type *t = node->binary.left->type_info;
    if (t && t->kind == TYPE_UNKNOWN)
    {
        t = NULL;
    }
    // An opaque alias keeps its name, and its `eq` is the alias's (`UserId__eq`); the type under
    // it decides whether there is one.
    Type *base = t;
    const char *opaque_name = NULL;
    while (base && base->kind == TYPE_ALIAS && base->inner)
    {
        if (!opaque_name && base->alias.is_opaque_alias)
        {
            opaque_name = base->name;
        }
        base = base->inner;
    }
    ASTNode *def = named_type_def(ctx, t);

    int is_simple_enum = 0;
    if (def && def->kind == NODE_ENUM)
    {
        is_simple_enum = 1;
        for (ASTNode *v = def->enm.variants; v; v = v->next)
        {
            if (v->variant.payload)
            {
                is_simple_enum = 0;
                break;
            }
        }
    }

    // A struct, or an enum with a payload, is compared with its `eq`; a simple enum, a number or
    // a pointer with C's `==`.
    int calls_eq = (def || opaque_name) && base &&
                   (base->kind == TYPE_STRUCT || base->kind == TYPE_ENUM) && !is_simple_enum;
    Type *right = node->binary.right->type_info;
    if (calls_eq)
    {
        char *spelling = opaque_name ? xstrdup(opaque_name) : c_type_spelling(ctx, t);
        const char *base_name = str_strip_struct_prefix(spelling);

        if (node->binary.op == OP_NE)
        {
            EMIT(ctx, "(!");
        }
        // `eq` of the Eq trait, or an inherent `eq`.
        char *trait_eq = mangle_method_symbol(base_name, "Eq", "eq");
        ZenSymbol *sym = find_symbol_in_all(ctx, trait_eq);
        if (!sym || !sym->data.sig)
        {
            sym = find_symbol_in_all(ctx, mangle_method_symbol(base_name, NULL, "eq"));
        }
        FuncSig *sig = sym ? sym->data.sig : NULL;
        const char *call_name = sig ? sig->name : trait_eq;

        EMIT(ctx, "%s(", (sig && sig->link_name) ? sig->link_name : call_name);

        if (expr_has_address(ctx, node->binary.left))
        {
            EMIT(ctx, "&");
            codegen_expression(ctx, node->binary.left);
        }
        else
        {
            emit_rvalue_address(ctx, node->binary.left);
        }

        EMIT(ctx, ", ");

        int needs_ptr = 1;
        if (sig)
        {
            needs_ptr = (sig->total_args > 1 && sig->arg_types[1]->kind == TYPE_POINTER);
        }

        if (needs_ptr && expr_has_address(ctx, node->binary.right))
        {
            EMIT(ctx, "&");
            codegen_expression(ctx, node->binary.right);
        }
        else if (needs_ptr)
        {
            emit_rvalue_address(ctx, node->binary.right);
        }
        else
        {
            codegen_expression(ctx, node->binary.right);
        }

        EMIT(ctx, ")");
        if (node->binary.op == OP_NE)
        {
            EMIT(ctx, ")");
        }
    }
    else if (t && t->kind == TYPE_STRING && right && right->kind == TYPE_STRING)
    {
        // Two strings compare their characters.
        EMIT(ctx, "(strcmp(");
        codegen_expression(ctx, node->binary.left);
        EMIT(ctx, ", ");
        codegen_expression(ctx, node->binary.right);
        EMIT(ctx, node->binary.op == OP_EQ ? ") == 0)" : ") != 0)");
    }
    else
    {
        EMIT(ctx, "(");
        codegen_expression(ctx, node->binary.left);
        EMIT(ctx, " %s ", operator_spelling(node->binary.op));
        codegen_expression(ctx, node->binary.right);
        EMIT(ctx, ")");
    }
}

// `a ** b`, through _zc_pow.
static void emit_pow(ParserContext *ctx, ASTNode *node)
{
    EMIT(ctx, "(_zc_pow((double)(");
    codegen_expression(ctx, node->binary.left);
    EMIT(ctx, "), (double)(");
    codegen_expression(ctx, node->binary.right);
    EMIT(ctx, ")))");
}

// The other binary operators and assignments, written as in C; an assignment drops the old
// value of a variable with Drop, and copies a fixed-size array.
static void emit_plain_binary(ParserContext *ctx, ASTNode *node)
{
    int is_assignment = is_assignment_operator(node->binary.op);

    // Assigning to a variable that holds a value with Drop drops the old value first: the
    // glue of a struct, enum or tuple, or the drop of a closure that owns its context.
    char *drop_old = NULL;
    if (is_assignment && node->binary.op == OP_ASSIGN && node->binary.left->kind == NODE_EXPR_VAR)
    {
        char *glue = value_drop_struct(ctx, node->binary.left);
        if (glue)
        {
            size_t sz = strlen(glue) + 32;
            drop_old = xmalloc(sz);
            snprintf(drop_old, sz, "%s__Drop__glue(_z_dest);", glue);
        }
        else if (is_owning_closure(node->binary.left->type_info))
        {
            drop_old = drop_value_stmt(node->binary.left->type_info, "(*_z_dest)", 0);
        }
    }

    if (is_assignment && node->binary.op == OP_ASSIGN &&
        is_fixed_size_array(node->binary.left->type_info))
    {
        emit_array_assignment(ctx, node->binary.left, node->binary.right);
    }
    else if (drop_old)
    {
        // The variable's type, not ZC_AUTO: tcc has no __auto_type.
        EMIT(ctx, "({ __typeof__((");
        codegen_expression(ctx, node->binary.left);
        EMIT(ctx, ")) _z_tmp = (");
        codegen_expression_with_move(ctx, node->binary.right);
        EMIT(ctx, "); ");

        EMIT(ctx, "__typeof__((");
        codegen_expression(ctx, node->binary.left);
        EMIT(ctx, "))* _z_dest = &(");
        codegen_expression(ctx, node->binary.left);
        EMIT(ctx, "); ");

        EMIT(ctx, "if (__z_drop_flag_%s) %s ", node->binary.left->var_ref.name, drop_old);
        EMIT(ctx, "*_z_dest = _z_tmp; ");
        EMIT(ctx, "__z_drop_flag_%s = 1; ", node->binary.left->var_ref.name);

        EMIT(ctx, "*_z_dest; })");
    }
    else
    {
        EMIT(ctx, "(");
        if (is_assignment)
        {
            codegen_expression(ctx, node->binary.left);
        }
        else
        {
            codegen_expression_with_move(ctx, node->binary.left);
        }

        EMIT(ctx, " %s ", operator_spelling(node->binary.op));
        if (ctx->config->misra_mode ||
            (ctx->config->use_cpp && is_assignment && node->binary.op == OP_ASSIGN))
        {
            int should_cast = ctx->config->misra_mode;
            if (!should_cast && node->binary.left->type_info)
            {
                TypeKind k = node->binary.left->type_info->kind;
                if (k == TYPE_POINTER || k == TYPE_ENUM ||
                    is_enum_type(ctx, node->binary.left->type_info))
                {
                    should_cast = 1;
                }
            }

            if (should_cast)
            {
                char *c_type = NULL;
                if (node->binary.left->type_info)
                {
                    c_type = c_type_spelling(ctx, node->binary.left->type_info);
                }
                if (c_type && strcmp(c_type, "unknown") != 0 && strcmp(c_type, "void") != 0)
                {
                    EMIT(ctx, "(%s)(", c_type);
                    codegen_expression_with_move(ctx, node->binary.right);
                    EMIT(ctx, ")");
                    zfree(c_type);
                }
                else
                {
                    if (c_type)
                    {
                        zfree(c_type);
                    }
                    codegen_expression_with_move(ctx, node->binary.right);
                }
            }
            else
            {
                codegen_expression_with_move(ctx, node->binary.right);
            }
        }
        else
        {
            codegen_expression_with_move(ctx, node->binary.right);
        }
        EMIT(ctx, ")");
    }
}

void handle_expr_binary(ParserContext *ctx, ASTNode *node)
{
    if (ctx->config->misra_mode)
    {
        EMIT(ctx, "(");
    }
    switch (node->binary.op)
    {
    case OP_COALESCE:
        emit_coalesce(ctx, node);
        break;
    case OP_EQ:
    case OP_NE:
        emit_equality(ctx, node);
        break;
    case OP_POW:
        emit_pow(ctx, node);
        break;
    default:
        emit_plain_binary(ctx, node);
        break;
    }
    if (ctx->config->misra_mode)
    {
        EMIT(ctx, ")");
    }
}

// Struct with Drop held by an argument that references a temporary the call owns.
static char *owned_temp_drop_type(ParserContext *ctx, ASTNode *arg)
{
    if (arg->kind != NODE_EXPR_UNARY || !arg->unary.owns_temp || arg->unary.temp_id > 0)
    {
        return NULL;
    }
    return value_drop_struct(ctx, arg->unary.operand);
}

// Emits a call that borrows temporaries with Drop (e.g. `a + make()` on an overloaded
// operator): each temporary gets a name, and is dropped once the call has returned.
static int emit_call_dropping_temporaries(ParserContext *ctx, ASTNode *node)
{
    int has_temp = 0;
    ASTNode *arg = node->call.args;
    while (arg)
    {
        if (owned_temp_drop_type(ctx, arg))
        {
            has_temp = 1;
        }
        arg = arg->next;
    }
    if (!has_temp || !node->type_info)
    {
        return 0;
    }

    int returns_value = node->type_info->kind != TYPE_VOID;
    EMIT(ctx, "({ ");
    arg = node->call.args;
    while (arg)
    {
        char *drop_type = owned_temp_drop_type(ctx, arg);
        if (drop_type)
        {
            int id = ctx->cg.tmp_counter++;
            EMIT(ctx, "%s _z_temp_%d = ", drop_type, id);
            codegen_expression(ctx, arg->unary.operand);
            EMIT(ctx, "; ");
            arg->unary.temp_id = id + 1;
        }
        arg = arg->next;
    }

    // ZC_AUTO_INIT, not ZC_AUTO: tcc has no __auto_type, so there it is __typeof__.
    int result_id = ctx->cg.tmp_counter++;
    if (returns_value)
    {
        EMIT(ctx, "ZC_AUTO_INIT(_z_result_%d, ", result_id);
    }
    handle_expr_call(ctx, node);
    EMIT(ctx, "%s", returns_value ? "); " : "; ");

    arg = node->call.args;
    while (arg)
    {
        if (arg->kind == NODE_EXPR_UNARY && arg->unary.temp_id > 0)
        {
            int id = arg->unary.temp_id - 1;
            arg->unary.temp_id = 0;
            EMIT(ctx, "%s__Drop__glue(&_z_temp_%d); ", owned_temp_drop_type(ctx, arg), id);
        }
        arg = arg->next;
    }
    if (returns_value)
    {
        EMIT(ctx, "_z_result_%d; ", result_id);
    }
    EMIT(ctx, "})");
    return 1;
}

void handle_expr_call(ParserContext *ctx, ASTNode *node)
{
    if (emit_call_dropping_temporaries(ctx, node))
    {
        return;
    }

    emit_source_mapping(ctx, node);

    if (node->call.callee->kind == NODE_EXPR_MEMBER)
    {
        Type *callee_ti = get_inner_type(node->call.callee->type_info);
        if (callee_ti && callee_ti->kind == TYPE_FUNCTION)
        {
            goto skip_method_mangling;
        }

        ASTNode *target = node->call.callee->member.target;
        char *method = node->call.callee->member.field;

        if (strcmp(method, "len") == 0)
        {
            if (target->type_info && target->type_info->kind == TYPE_ARRAY)
            {
                if (target->type_info->array_size > 0)
                {
                    EMIT(ctx, "%d", target->type_info->array_size);
                }
                else
                {
                    codegen_expression(ctx, target);
                    EMIT(ctx, ".len");
                }
                return;
            }
        }

        // The method's owner is the target's type, or the type it points to.
        Type *target_type = target->type_info;
        if (target_type && target_type->kind != TYPE_UNKNOWN)
        {
            int is_pointer = target_type->kind == TYPE_POINTER;
            char *base = type_impl_name(is_pointer ? target_type->inner : target_type);

            // The method the typechecker resolved (member.method): its C name, and its owner,
            // when a mixed-in struct's method takes a pointer to that struct. A trait object has
            // none: its call goes to the trait's dispatch function, `Trait__method`.
            FuncSig *resolved = node->call.callee->member.method;
            const char *call_base = resolved ? node->call.callee->member.method_owner : base;
            int need_cast = resolved && strcmp(call_base, base) != 0;
            if (resolved)
            {
                EMIT(ctx, "%s", resolved->link_name ? resolved->link_name : resolved->name);
            }
            else
            {
                char *dispatch = mangle_method_symbol(base, NULL, method);
                EMIT(ctx, "%s", dispatch);
                zfree(dispatch);
            }
            EMIT(ctx, "(");
            if (need_cast)
            {
                EMIT(ctx, "(%s*)", call_base);
            }
            if (!is_pointer &&
                (target->kind == NODE_EXPR_CALL || target->kind == NODE_EXPR_LITERAL ||
                 target->kind == NODE_EXPR_BINARY || target->kind == NODE_EXPR_UNARY ||
                 target->kind == NODE_EXPR_CAST || target->kind == NODE_EXPR_STRUCT_INIT))
            {
                // A temporary has no address: C names the instance just mangled;
                // C++ needs no type (_z_rval)
                if (ctx->config->use_cpp)
                {
                    emit_rvalue_address(ctx, target);
                }
                else
                {
                    EMIT(ctx, "((%s[]){", c_type_spelling(ctx, target_type));
                    codegen_expression(ctx, target);
                    EMIT(ctx, "})");
                }
            }
            else
            {
                if (!is_pointer)
                {
                    EMIT(ctx, "&");
                }
                codegen_expression(ctx, target);
            }
            ASTNode *arg = node->call.args;
            while (arg)
            {
                EMIT(ctx, ", ");
                codegen_expression_with_move(ctx, arg);
                arg = arg->next;
            }
            EMIT(ctx, ")");
            return;
        }
    }

skip_method_mangling:

    if (node->call.callee->kind == NODE_EXPR_VAR)
    {
        // `Point()`: the name of a struct, which the typechecker resolved (var_ref.symbol).
        ZenSymbol *sym = node->call.callee->var_ref.symbol;
        ASTNode *def = sym && sym->kind == SYM_STRUCT ? sym->data.node : NULL;
        if (def && def->kind == NODE_STRUCT)
        {
            EMIT(ctx, "(struct %s){0}", node->call.callee->var_ref.name);
            return;
        }
    }

    Type *callee_ti = get_inner_type(node->call.callee->type_info);
    if (callee_ti && callee_ti->kind == TYPE_FUNCTION && !callee_ti->is_raw)
    {
        EMIT(ctx, "({ z_closure_T _c = ");
        codegen_expression(ctx, node->call.callee);
        EMIT(ctx, "; ");

        Type *ft = callee_ti;
        char *ret = c_type_spelling(ctx, ft->inner);
        if (strcmp(ret, "string") == 0)
        {
            zfree(ret);
            ret = xstrdup("char*");
        }
        if (strcmp(ret, "unknown") == 0)
        {
            zfree(ret);
            ret = xstrdup("void*");
        }

        EMIT(ctx, "((%s (*)(void*", ret);
        for (int i = 0; i < ft->count; i++)
        {
            char *as = c_type_spelling(ctx, ft->args[i]);
            if (strcmp(as, "unknown") == 0)
            {
                zfree(as);
                as = xstrdup("void*");
            }
            EMIT(ctx, ", %s", as);
            zfree(as);
        }
        if (ft->is_varargs)
        {
            EMIT(ctx, ", ...");
        }
        EMIT(ctx, "))_c.func)(_c.ctx");

        ASTNode *arg = node->call.args;
        while (arg)
        {
            EMIT(ctx, ", ");
            codegen_expression_with_move(ctx, arg);
            arg = arg->next;
        }
        EMIT(ctx, "); })");
        zfree(ret);
        return;
    }

    if (node->call.callee->kind == NODE_EXPR_VAR &&
        strcmp(node->call.callee->var_ref.name, "panic") == 0)
    {
        EMIT(ctx, "__zenc_panic");
        goto skip_callee_gen;
    }
#if FF_COMPATIBILITY_0_5
    else if (node->call.callee->kind == NODE_EXPR_VAR &&
             emit_legacy_variant_name(ctx, node->call.callee->var_ref.name))
    {
        goto skip_callee_gen;
    }
#endif

    // Only a name can be a variant that gets its () from this call: a variant inside
    // a longer callee (`table[Shape::Empty.tag].run`) still needs its own.
    g_emitting_callee = node->call.callee->kind == NODE_EXPR_VAR;
    codegen_expression(ctx, node->call.callee);
    g_emitting_callee = 0;
skip_callee_gen:
    EMIT(ctx, "(");

    if (node->call.arg_names && node->call.callee->kind == NODE_EXPR_VAR)
    {
        ASTNode *arg = node->call.args;
        int first = 1;
        while (arg)
        {
            if (!first)
            {
                EMIT(ctx, ", ");
            }
            first = 0;
            codegen_expression_with_move(ctx, arg);
            arg = arg->next;
        }
    }
    else
    {
        // The function the typechecker resolved (call.function), for its parameter types.
        FuncSig *sig = node->call.function;

        ASTNode *arg = node->call.args;
        int arg_idx = 0;
        while (arg)
        {
            int handled = 0;
            if (sig && arg_idx < sig->total_args)
            {
                Type *param_t = sig->arg_types[arg_idx];

                if (param_t && param_t->kind == TYPE_TUPLE && sig->total_args == 1 &&
                    node->call.count > 1)
                {
                    EMIT(ctx, "(%s){", c_type_spelling(ctx, param_t));
                    ASTNode *curr = arg;
                    int first_field = 1;
                    while (curr)
                    {
                        if (!first_field)
                        {
                            EMIT(ctx, ", ");
                        }
                        first_field = 0;
                        codegen_expression_with_move(ctx, curr);
                        curr = curr->next;
                    }
                    EMIT(ctx, "}");
                    handled = 1;
                    arg = NULL;
                }
            }

            if (handled)
            {
                if (arg == NULL)
                {
                    break;
                }
            }
            else
            {
                if (arg->kind == NODE_RAW_STMT || arg->kind == NODE_EXPR_FSTRING)
                {
                    int is_fmt_arg = 0;
                    if (sig && sig->is_varargs && arg_idx == sig->total_args - 1)
                    {
                        is_fmt_arg = 1;
                    }
                    else if (node->call.callee->kind == NODE_EXPR_VAR)
                    {
                        const char *callee_name = node->call.callee->var_ref.name;
                        is_fmt_arg = (strcmp(callee_name, "printf") == 0 && arg_idx == 0) ||
                                     (strcmp(callee_name, "fprintf") == 0 && arg_idx == 1) ||
                                     (strcmp(callee_name, "sprintf") == 0 && arg_idx == 1) ||
                                     (strcmp(callee_name, "snprintf") == 0 && arg_idx == 2) ||
                                     (strcmp(callee_name, "dprintf") == 0 && arg_idx == 1);
                    }
                    if (is_fmt_arg)
                    {
                        EMIT(ctx, "\"%%s\", ");
                    }
                }
                if (ctx->config->use_cpp && sig && arg_idx < sig->total_args)
                {
                    Type *param_t = sig->arg_types[arg_idx];
                    if (param_t && (param_t->kind == TYPE_POINTER || param_t->kind == TYPE_ENUM))
                    {
                        char *c_type = c_type_spelling(ctx, param_t);
                        EMIT(ctx, "(%s)(", c_type);
                        codegen_expression_with_move(ctx, arg);
                        EMIT(ctx, ")");
                        zfree(c_type);
                    }
                    else
                    {
                        codegen_expression_with_move(ctx, arg);
                    }
                }
                else
                {
                    codegen_expression_with_move(ctx, arg);
                }
            }

            if (arg && arg->next)
            {
                EMIT(ctx, ", ");
            }
            if (arg)
            {
                arg = arg->next;
            }
            arg_idx++;
        }
    }
    EMIT(ctx, ")");
}
