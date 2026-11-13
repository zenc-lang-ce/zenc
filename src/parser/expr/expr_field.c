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

Type *get_field_type(ParserContext *ctx, Type *struct_type, const char *field_name)
{
    if (!struct_type)
    {
        return NULL;
    }

    // `.len` of a fixed-size array; a slice's fields are those of its struct.
    if (struct_type->kind == TYPE_ARRAY && strcmp(field_name, "len") == 0)
    {
        return type_new(TYPE_INT);
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
