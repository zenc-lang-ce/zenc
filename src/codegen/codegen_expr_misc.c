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
    char *slice_name = c_type_spelling(ctx, node->type_info);
    char *elem_name = c_type_spelling(ctx, slice_element(node->type_info));
    if (array->kind != NODE_EXPR_ARRAY_LITERAL)
    {
        EMIT(ctx, "(%s){.data = ", slice_name);
        codegen_expression(ctx, array);
        EMIT(ctx, ", .len = %d}", array->type_info->array_size);
    }
    else if (ctx->config->use_cpp && ctx->cg.current_func_ret_type_info)
    {
        // C++ has no array compound literals: the elements go in a static array.
        EMIT(ctx, "({ static const %s __tmp[] = ", elem_name);
        codegen_expression(ctx, array);
        EMIT(ctx, "; (%s){(%s *)__tmp, %d}; })", slice_name, elem_name, array->array_literal.count);
    }
    else
    {
        EMIT(ctx, "(%s){.data = (%s[])", slice_name, elem_name);
        codegen_expression(ctx, array);
        EMIT(ctx, ", .len = %d}", array->array_literal.count);
    }
}

void handle_expr_cast(ParserContext *ctx, ASTNode *node)
{
    Type *from = node->cast.expr->type_info;
    if (slice_element(node->type_info) &&
        (node->cast.expr->kind == NODE_EXPR_ARRAY_LITERAL || (from && from->kind == TYPE_ARRAY)))
    {
        emit_array_as_slice(ctx, node);
        return;
    }

    const char *mapped = map_to_c_type(c_type_spelling(ctx, node->type_info));

    EMIT(ctx, "((%s)(", mapped);
    Type *src_type = node->cast.expr->type_info;
    int cast_tag = 0;
    if (src_type && src_type->kind == TYPE_ENUM)
    {
        const char *clean_name = type_def_name(src_type);
        if (strncmp(clean_name, "struct ", 7) == 0)
        {
            clean_name += 7;
        }
        ASTNode *def = find_struct_def(ctx, clean_name);
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
    if (node->reflection.kind == 0)
    {
        char *s = c_type_spelling(ctx, t);
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
        ASTNode *def = find_struct_def(ctx, sname);
        if (!def)
        {
            EMIT(ctx, "((void*)0)");
            return;
        }

        EMIT(ctx,
             "({ static struct { char *name; char *type; unsigned long offset; } _fields_%s[] "
             "= {",
             sname);
        ASTNode *f = def->strct.fields;
        while (f)
        {
            if (f->kind == NODE_FIELD)
            {
                EMIT(ctx, "{ \"%s\", \"%s\", __builtin_offsetof(struct %s, %s) }, ", f->field.name,
                     type_to_string(f->type_info), sname, f->field.name);
            }
            f = f->next;
        }
        EMIT(ctx, "{ 0 } }; (void*)_fields_%s; })", sname);
    }
}
