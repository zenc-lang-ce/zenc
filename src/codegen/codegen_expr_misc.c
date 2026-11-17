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

// An array viewed as a slice: its elements and their count. An array literal's elements live
// in a compound literal, as long as the enclosing block.
static void emit_array_as_slice(ParserContext *ctx, ASTNode *node)
{
    ASTNode *array = node->cast.expr;
    Type *elem = slice_element(node->type_info);
    char *slice_name = c_type_spelling(ctx, node->type_info);
    // A pointer to the first element, as a cast C writes for any element type.
    char *elem_pointer = c_declaration(ctx, type_new_ptr(elem), "");
    int len = array->kind == NODE_EXPR_ARRAY_LITERAL ? array->array_literal.count
                                                     : array->type_info->array_size;
#if FF_COMPATIBILITY_0_5
    // A slice has a `cap`, its `len`. An array literal returned as a slice is copied to the
    // heap, and never freed, as in 0.5.1.
    char cap[48];
    char cap_positional[32];
    snprintf(cap, sizeof(cap), ", .cap = %d", len);
    snprintf(cap_positional, sizeof(cap_positional), ", %d", len);
    if (array->kind == NODE_EXPR_ARRAY_LITERAL && array->array_literal.returned_as_slice)
    {
        // The elements' array, as C declares it for any element type (`int32_t __tmp[]`,
        // `int32_t (*__tmp[])(int32_t)`).
        char *tmp_array = c_declaration(ctx, elem, "__tmp[]");
        char *heap = c_declaration(ctx, type_new_ptr(elem), "__heap");
        EMIT(ctx, "({ %s = ", tmp_array);
        codegen_expression(ctx, array);
        EMIT(ctx,
             "; %s = (%s)malloc(sizeof(__tmp)); memcpy(__heap, __tmp, sizeof(__tmp)); "
             "(%s){__heap, %d, %d}; })",
             heap, elem_pointer, slice_name, len, len);
        return;
    }
#else
    const char *cap = "";
    const char *cap_positional = "";
#endif
    if (array->kind != NODE_EXPR_ARRAY_LITERAL)
    {
        EMIT(ctx, "(%s){.data = ", slice_name);
        codegen_expression(ctx, array);
        EMIT(ctx, ", .len = %d%s}", len, cap);
    }
    else if (ctx->config->use_cpp && ctx->cg.current_func_ret_type_info)
    {
        // C++ has no array compound literals: the elements go in a static array, of const
        // elements.
        Type *const_elem = type_clone(elem);
        const_elem->is_const = 1;
        char *static_array = c_declaration(ctx, const_elem, "__tmp[]");
        EMIT(ctx, "({ static %s = ", static_array);
        zfree(static_array);
        codegen_expression(ctx, array);
        EMIT(ctx, "; (%s){(%s)__tmp, %d%s}; })", slice_name, elem_pointer, len, cap_positional);
    }
    else
    {
        char *literal = c_declaration(ctx, elem, "[]");
        EMIT(ctx, "(%s){.data = (%s)", slice_name, literal);
        codegen_expression(ctx, array);
        EMIT(ctx, ", .len = %d%s}", len, cap);
    }
}

// The typechecker's implicit conversion: C converts the value itself, so the expression is
// emitted as written.
// Whether converting `value` to `to` (an implicit conversion or a cast) builds a trait object,
// and of which trait: `to` itself, or what it points to for a pointer to a trait object. NULL
// when it builds none: `value` is not a pointer to a struct, or `to` is not a trait.
static Type *trait_to_build(ParserContext *ctx, Type *to, ASTNode *value)
{
    Type *from = value ? get_inner_type(value->type_info) : NULL;
    if (!to || !from || from->kind != TYPE_POINTER || !from->inner ||
        get_inner_type(from->inner)->kind != TYPE_STRUCT)
    {
        return NULL;
    }
    Type *trait = get_inner_type(to);
    if (trait->kind == TYPE_POINTER && trait->inner)
    {
        trait = get_inner_type(trait->inner);
    }
    return trait->name && is_trait(ctx, type_def_name(trait)) ? trait : NULL;
}

// `(Trait){.self=(void*)p, .vtable=&Struct__Trait__VTable}`: the trait object of the struct the
// pointer `value` points to.
static void emit_trait_object(ParserContext *ctx, Type *trait, ASTNode *value)
{
    const char *trait_name = type_def_name(trait);
    Type *pointee = get_inner_type(get_inner_type(value->type_info)->inner);
    char *vtable = mangle_vtable_symbol(type_def_name(pointee), trait_name);
    EMIT(ctx, "(%s){.self=(void*)", trait_name);
    if (value->kind == NODE_EXPR_UNARY && value->unary.op == OP_ADDRESS)
    {
        EMIT(ctx, "&");
        codegen_expression(ctx, value->unary.operand);
    }
    else
    {
        codegen_expression(ctx, value);
    }
    EMIT(ctx, ", .vtable=&%s}", vtable);
}

// The trait object of the pointer `value` converted to `to`, or its address when `to` is a
// pointer to a trait object: a compound literal's in C, a static temporary's in C++ (which does
// not take the address of a compound literal).
static void emit_trait_object_as(ParserContext *ctx, Type *to, Type *trait, ASTNode *value)
{
    if (get_inner_type(to)->kind != TYPE_POINTER)
    {
        emit_trait_object(ctx, trait, value);
    }
    else if (ctx->config->use_cpp)
    {
        EMIT(ctx, "({ static %s _ztrait = ", type_def_name(trait));
        emit_trait_object(ctx, trait, value);
        EMIT(ctx, "; &_ztrait; })");
    }
    else
    {
        EMIT(ctx, "(&");
        emit_trait_object(ctx, trait, value);
        EMIT(ctx, ")");
    }
}

void handle_implicit_conversion(ParserContext *ctx, ASTNode *node)
{
    Type *trait = trait_to_build(ctx, node->type_info, node->conversion.expr);
    if (trait)
    {
        emit_trait_object_as(ctx, node->type_info, trait, node->conversion.expr);
        return;
    }
    codegen_expression(ctx, node->conversion.expr);
}

void handle_expr_cast(ParserContext *ctx, ASTNode *node)
{
    Type *from = node->cast.expr->type_info;
    Type *trait = trait_to_build(ctx, node->type_info, node->cast.expr);
    if (trait)
    {
        emit_trait_object_as(ctx, node->type_info, trait, node->cast.expr);
        return;
    }
    if (slice_element(node->type_info) &&
        (node->cast.expr->kind == NODE_EXPR_ARRAY_LITERAL || (from && from->kind == TYPE_ARRAY)))
    {
        emit_array_as_slice(ctx, node);
        return;
    }

    const char *mapped = c_type_spelling(ctx, node->type_info);

    EMIT(ctx, "((%s)(", mapped);
    Type *src_type = node->cast.expr->type_info;
    int cast_tag = 0;
    if (src_type && src_type->kind == TYPE_ENUM)
    {
        ASTNode *def = named_type_def(ctx, src_type);
        if (def && def->kind == NODE_ENUM)
        {
            ASTNode *v = def->enm.variants;
            while (v)
            {
                if (v->variant.payload)
                {
                    cast_tag = 1;
                    break;
                }
                v = v->next;
            }
        }
    }

    if (cast_tag)
    {
        codegen_expression(ctx, node->cast.expr);
        EMIT(ctx, ".tag");
    }
    else
    {
        codegen_expression(ctx, node->cast.expr);
    }
    EMIT(ctx, "))");
}

void handle_reflection(ParserContext *ctx, ASTNode *node)
{
    Type *t = node->reflection.target_type;
    // The program reads types by the names it writes: `i32`, a @link_name struct's own name.
    if (node->reflection.kind == 0)
    {
        char *s = type_to_string(t);
        EMIT(ctx, "\"%s\"", s);
        zfree(s);
    }
    else
    {
        if (t->kind != TYPE_STRUCT || !t->name)
        {
            EMIT(ctx, "((void*)0)");
            return;
        }
        char *sname = type_def_name(t);
        ASTNode *def = named_type_def(ctx, t);
        if (!def)
        {
            EMIT(ctx, "((void*)0)");
            return;
        }

        EMIT(ctx,
             "({ static struct { char *name; char *type; unsigned long offset; } _fields_%s[] "
             "= {",
             sname);
        // offsetof names the struct as C does: by its @link_name, if it has one.
        const char *struct_c_name = str_strip_struct_prefix(c_type_spelling(ctx, t));
        ASTNode *f = def->strct.fields;
        while (f)
        {
            if (f->kind == NODE_FIELD)
            {
                EMIT(ctx, "{ \"%s\", \"%s\", __builtin_offsetof(struct %s, %s) }, ", f->field.name,
                     type_to_string(f->type_info), struct_c_name, c_name(ctx, f->field.name));
            }
            f = f->next;
        }
        EMIT(ctx, "{ 0 } }; (void*)_fields_%s; })", sname);
    }
}
