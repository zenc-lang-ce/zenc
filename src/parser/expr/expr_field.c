// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include "zen/zen_facts.h"
#include "ast/ast.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_comparison_op(const char *op)
{
    return (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, "<") == 0 ||
            strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0);
}

Type *get_field_type(ParserContext *ctx, Type *struct_type, const char *field_name)
{
    if (!struct_type)
    {
        return NULL;
    }

    // Built-in Fields for Arrays/Slices
    if (struct_type->kind == TYPE_ARRAY)
    {
        if (strcmp(field_name, "len") == 0)
        {
            return type_new(TYPE_INT);
        }
        if (struct_type->array_size == 0)
        { // Slice
            if (strcmp(field_name, "cap") == 0)
            {
                return type_new(TYPE_INT);
            }
            if (strcmp(field_name, "data") == 0)
            {
                return type_new_ptr(struct_type->inner);
            }
        }
    }

    Type *tuple = tuple_of(struct_type);
    if (tuple)
    {
        return tuple_element_type(tuple, field_name);
    }

    // Use resolve_struct_name_from_type to handle Generics and Pointers correctly
    int is_ptr = 0;
    char *alloc_name = NULL;
    char *sname = resolve_struct_name_from_type(ctx, struct_type, &is_ptr, &alloc_name);

    if (!sname)
    {
        return NULL;
    }

    ASTNode *def = find_struct_def(ctx, sname);
    if (!def)
    {
        if (alloc_name)
        {
            zfree(alloc_name);
        }
        return NULL;
    }

    ASTNode *f = def->strct.fields;
    while (f)
    {
        if (strcmp(f->field.name, field_name) == 0)
        {
            if (alloc_name)
            {
                zfree(alloc_name);
            }
            return f->type_info;
        }
        f = f->next;
    }
    if (alloc_name)
    {
        zfree(alloc_name);
    }
    return NULL;
}

const char *get_operator_method(const char *op)
{
    // Arithmetic
    if (strcmp(op, "+") == 0)
    {
        return "add";
    }
    if (strcmp(op, "-") == 0)
    {
        return "sub";
    }
    if (strcmp(op, "*") == 0)
    {
        return "mul";
    }
    if (strcmp(op, "/") == 0)
    {
        return "div";
    }
    if (strcmp(op, "%") == 0)
    {
        return "rem";
    }
    if (strcmp(op, "**") == 0)
    {
        return "pow";
    }

    // Comparison
    if (strcmp(op, "==") == 0)
    {
        return "eq";
    }
    if (strcmp(op, "!=") == 0)
    {
        return "neq";
    }
    if (strcmp(op, "<") == 0)
    {
        return "lt";
    }
    if (strcmp(op, ">") == 0)
    {
        return "gt";
    }
    if (strcmp(op, "<=") == 0)
    {
        return "le";
    }
    if (strcmp(op, ">=") == 0)
    {
        return "ge";
    }

    if (strcmp(op, "&") == 0)
    {
        return "bitand";
    }
    if (strcmp(op, "|") == 0)
    {
        return "bitor";
    }
    if (strcmp(op, "^") == 0)
    {
        return "bitxor";
    }
    if (strcmp(op, "<<") == 0)
    {
        return "shl";
    }
    if (strcmp(op, ">>") == 0)
    {
        return "shr";
    }

    return NULL;
}

char *resolve_struct_name_from_type(ParserContext *ctx, Type *t, int *is_ptr_out,
                                    char **allocated_out)
{
    if (!t)
    {
        return NULL;
    }
    *allocated_out = NULL;
    *is_ptr_out = 0;

    if (t->kind == TYPE_POINTER && t->inner)
    {
        *is_ptr_out = 1;
        int inner_ptr = 0;
        char *inner_name = resolve_struct_name_from_type(ctx, t->inner, &inner_ptr, allocated_out);
        return inner_name;
    }

    // Opaque aliases, structs and enums have methods (`impl Value { fn as_hour(self) ... }`).
    if ((t->kind == TYPE_ALIAS && t->alias.is_opaque_alias) || t->kind == TYPE_STRUCT ||
        (t->kind == TYPE_ENUM && t->name))
    {
        return type_def_name(t);
    }
    return NULL;
}
