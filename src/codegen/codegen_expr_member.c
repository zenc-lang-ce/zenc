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

void handle_expr_member(ParserContext *ctx, ASTNode *node)
{
    if (strcmp(node->member.field, "len") == 0)
    {
        if (node->member.target->type_info && node->member.target->type_info->kind == TYPE_ARRAY &&
            node->member.target->type_info->array_size > 0)
        {
            EMIT(ctx, "%d", node->member.target->type_info->array_size);
            return;
        }
    }

    if (node->member.target->type_info && node->member.target->type_info->kind == TYPE_VECTOR)
    {
        codegen_expression(ctx, node->member.target);
        return;
    }

    if (strcmp(node->member.field, "tag") == 0)
    {
        // The tag of an enum without payloads is the value itself.
        const char *tname = type_def_name(node->member.target->type_info);
        if (tname && is_simple_enum(ctx, tname))
        {
            codegen_expression(ctx, node->member.target);
            return;
        }
    }

    if (node->member.is_pointer_access == 2)
    {
        EMIT(ctx, "({ ");
        emit_auto_type(ctx, node->member.target, node->token);
        EMIT(ctx, " _t = (");
        codegen_expression(ctx, node->member.target);
        char *field = node->member.field;
        if (field && field[0] >= '0' && field[0] <= '9')
        {
            EMIT(ctx, "); _t ? _t->v%s : 0; })", field);
        }
        else
        {
            EMIT(ctx, "); _t ? _t->%s : 0; })", field);
        }
    }
    else
    {
        // `Color.Red` never gets here: the parser reads it as `Color::Red` (next_is_dot_variant).
        codegen_expression(ctx, node->member.target);
        Type *target_type = node->member.target->type_info;
        int actually_ptr = target_type && get_inner_type(target_type)->kind == TYPE_POINTER;

        char *field = node->member.field;
        if (field && field[0] >= '0' && field[0] <= '9')
        {
            EMIT(ctx, "%sv%s", actually_ptr ? "->" : ".", field);
        }
        else
        {
            EMIT(ctx, "%s%s", actually_ptr ? "->" : ".", c_name(ctx, field));
        }
    }
}

void handle_expr_index(ParserContext *ctx, ASTNode *node)
{
    Type *array_type = node->index.array->type_info;
    int is_slice_struct = slice_element(array_type) != NULL;

    if (is_slice_struct)
    {
        if (node->index.array->kind == NODE_EXPR_VAR)
        {
            codegen_expression(ctx, node->index.array);
            EMIT(ctx, ".data[_z_check_bounds(");
            codegen_expression(ctx, node->index.index);
            EMIT(ctx, ", ");
            codegen_expression(ctx, node->index.array);
            EMIT(ctx, ".len)]");
        }
        else
        {
            codegen_expression(ctx, node->index.array);
            EMIT(ctx, ".data[");
            codegen_expression(ctx, node->index.index);
            EMIT(ctx, "]");
        }
    }
    else
    {
        // An overloaded index (`get`) reaches the codegen as the call the typechecker made of it:
        // what is left is an array, a pointer or a vector.
        int fixed_size = -1;
        if (node->index.array->type_info && (node->index.array->type_info->kind == TYPE_ARRAY ||
                                             node->index.array->type_info->kind == TYPE_VECTOR))
        {
            fixed_size = node->index.array->type_info->array_size;
        }

        codegen_expression(ctx, node->index.array);
        EMIT(ctx, "[");
        if (fixed_size > 0)
        {
            EMIT(ctx, "_z_check_bounds(");
        }
        codegen_expression(ctx, node->index.index);
        if (fixed_size > 0)
        {
            EMIT(ctx, ", %d)", fixed_size);
        }
        EMIT(ctx, "]");
    }
}

void handle_expr_slice(ParserContext *ctx, ASTNode *node)
{
    Type *array_type = node->slice.array->type_info;
    int known_size = array_type && array_type->kind == TYPE_ARRAY ? array_type->array_size : -1;
    int is_slice_struct = slice_element(array_type) != NULL;

    // The slice's type, as C spells it ("unknown" while its type is not known).
    char *tname =
        slice_element(node->type_info) ? c_type_spelling(ctx, node->type_info) : xstrdup("unknown");

    // A slice is copied (it is a view); an array or a pointer is held as a pointer to its
    // first element, so the new slice points into it.
    EMIT(ctx, "({ ");
    if (is_slice_struct)
    {
        emit_auto_type(ctx, node->slice.array, node->token);
        EMIT(ctx, " _arr = ");
    }
    else
    {
        char *elem = c_type_spelling(ctx, slice_element(node->type_info));
        EMIT(ctx, "%s *_arr = ", elem);
        zfree(elem);
    }
    codegen_expression(ctx, node->slice.array);
    EMIT(ctx, "; size_t _start = ");
    if (node->slice.start)
    {
        codegen_expression(ctx, node->slice.start);
    }
    else
    {
        EMIT(ctx, "0");
    }
    EMIT(ctx, "; size_t _len = ");

    if (node->slice.end)
    {
        codegen_expression(ctx, node->slice.end);
        EMIT(ctx, " - _start; ");
    }
    else
    {
        if (known_size > 0)
        {
            EMIT(ctx, "%d - _start; ", known_size);
        }
        else if (is_slice_struct)
        {
            EMIT(ctx, "_arr.len - _start; ");
        }
        else
        {
            EMIT(ctx, "0; ");
        }
    }

    if (is_slice_struct)
    {
        EMIT(ctx, "(%s){ .data = _arr.data + _start, .len = _len", tname);
    }
    else
    {
        EMIT(ctx, "(%s){ .data = _arr + _start, .len = _len", tname);
    }
#if FF_COMPATIBILITY_0_5
    EMIT(ctx, ", .cap = _len");
#endif
    EMIT(ctx, " }; })");
    zfree(tname);
}
