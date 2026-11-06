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
        char *tname = infer_type(ctx, node->member.target);
        if (tname)
        {
            if (is_simple_enum(ctx, tname))
            {
                codegen_expression(ctx, node->member.target);
                zfree(tname);
                return;
            }
            zfree(tname);
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
        // When X is the name of an enum, `X.y` is the variant y of that enum (`Color.Red`).
        // But when X has a type, X is a variable or parameter that happens to share its name
        // with an enum, and `X.y` reads its field y: the variable hides the enum.
        if (node->member.target->kind == NODE_EXPR_VAR && !node->member.target->type_info)
        {
            ASTNode *def = find_struct_def(ctx, node->member.target->var_ref.name);
            if (def && def->kind == NODE_ENUM)
            {
                const char *enum_c_name =
                    def->link_name ? def->link_name : node->member.target->var_ref.name;
                EMIT(ctx, "%s", mangle_variant_symbol(enum_c_name, node->member.field));
                return;
            }
        }

        codegen_expression(ctx, node->member.target);
        char *lt = infer_type(ctx, node->member.target);
        int actually_ptr = 0;
        if (lt && (lt[strlen(lt) - 1] == '*' || strstr(lt, "*")))
        {
            actually_ptr = 1;
        }
        if (lt)
        {
            zfree(lt);
        }

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
        char *base_type = infer_type(ctx, node->index.array);
        char *struct_name = NULL;
        char *method_name = NULL;

        if (base_type && !strchr(base_type, '*'))
        {
            const char *clean = str_strip_struct_prefix(base_type);
            method_name = mangle_method_symbol(clean, NULL, "get");
            if (find_func(ctx, method_name))
            {
                struct_name = xstrdup(clean);
            }
        }

        if (struct_name)
        {
            FuncSig *sig = find_func(ctx, method_name);
            int needs_addr =
                (sig && sig->total_args > 0 && sig->arg_types[0]->kind == TYPE_POINTER);

            EMIT(ctx, "%s(", method_name);
            if (needs_addr)
            {
                EMIT(ctx, "&");
            }
            codegen_expression(ctx, node->index.array);
            EMIT(ctx, ", ");
            codegen_expression(ctx, node->index.index);
            ASTNode *extra = node->index.extra_indices;
            while (extra)
            {
                EMIT(ctx, ", ");
                codegen_expression(ctx, extra);
                extra = extra->next;
            }
            EMIT(ctx, ")");
            zfree(struct_name);
        }
        else
        {
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
}

void handle_expr_slice(ParserContext *ctx, ASTNode *node)
{
    Type *array_type = node->slice.array->type_info;
    int known_size = array_type && array_type->kind == TYPE_ARRAY ? array_type->array_size : -1;
    int is_slice_struct = slice_element(array_type) != NULL;

    char *tname = "unknown";
    if (slice_element(node->type_info))
    {
        tname = c_type_spelling(ctx, node->type_info);
    }

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
    if (tname && strcmp(tname, "unknown") != 0)
    {
        zfree(tname);
    }
}
