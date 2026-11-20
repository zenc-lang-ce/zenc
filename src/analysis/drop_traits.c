// SPDX-License-Identifier: MIT
// Which types have Drop: decided from their declarations (an `impl Drop`, a field or a payload
// that needs dropping) by propagate_drop_traits, and read by the rest of the compiler.
#include "drop_traits.h"
#include "plugins/plugin_manager.h"
#include "parser.h"
#include "utils/utils.h"
#include "constants.h"

int type_has_drop(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return 0;
    }
    if (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM)
    {
        ASTNode *def = t->name ? named_type_def(ctx, t) : NULL;
        if (def && def->type_info)
        {
            return def->type_info->traits.has_drop;
        }
    }
    return t->traits.has_drop;
}

int type_needs_drop(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return 0;
    }
    if (is_owning_closure(t))
    {
        return 1;
    }
    if (t->kind == TYPE_ARRAY && t->array_size > 0)
    {
        return type_needs_drop(ctx, t->inner);
    }
    if (t->kind == TYPE_TUPLE)
    {
        for (int i = 0; i < t->count; i++)
        {
            if (type_needs_drop(ctx, t->args[i]))
            {
                return 1;
            }
        }
        return 0;
    }
    if ((t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->name)
    {
        ASTNode *def = named_type_def(ctx, t);
        return def && def->type_info && def->type_info->traits.has_drop;
    }
    return 0;
}

// Marks a struct as Drop if one of its fields needs dropping, and an enum if the payload of one of
// its variants does; returns whether it did.
static int propagate_drop_from_fields(ParserContext *ctx, ASTNode *strct)
{
    if (!strct || !strct->type_info || strct->type_info->traits.has_drop)
    {
        return 0;
    }
    if (strct->kind == NODE_ENUM)
    {
        for (ASTNode *v = strct->enm.variants; v; v = v->next)
        {
            if (v->kind == NODE_ENUM_VARIANT && type_needs_drop(ctx, v->variant.payload))
            {
                strct->type_info->traits.has_drop = 1;
                return 1;
            }
        }
        return 0;
    }
    if (strct->kind != NODE_STRUCT)
    {
        return 0;
    }
    for (ASTNode *field = strct->strct.fields; field; field = field->next)
    {
        Type *ft = field->type_info;
        if (ft && (ft->kind == TYPE_VECTOR || type_needs_drop(ctx, ft)))
        {
            strct->type_info->traits.has_drop = 1;
            ft->traits.has_drop = 1;
            return 1;
        }
    }
    return 0;
}

// Whether the struct or enum `node` (not a template) has an `impl Drop`, its own or, for an
// instance (`Wrap__int32_t`), its template's (`Wrap`).
static int implements_drop(ParserContext *ctx, ASTNode *node)
{
    if (!node || (node->kind == NODE_STRUCT ? node->strct.is_template
                                            : node->kind != NODE_ENUM || node->enm.is_template))
    {
        return 0;
    }
    const char *name =
        node->kind == NODE_STRUCT ? declared_type_c_name(node) : declared_type_c_name(node);
    const char *tpl = NULL;
    for (Instantiation *inst = ctx->instantiations; inst && !tpl; inst = inst->next)
    {
        if (inst->template_name && strcmp(inst->name, name) == 0)
        {
            tpl = inst->template_name;
        }
    }
    return check_impl(ctx, "Drop", name) || (tpl && check_impl(ctx, "Drop", tpl));
}

// Marks `strct` as Drop, and as declaring it, if it implements Drop. `impl Drop for X` marks X
// when it is parsed, which misses a struct declared below the impl, and an instance created
// before a generic impl; and a struct its fields made Drop declares it still.
static void seed_drop_from_impl(ParserContext *ctx, ASTNode *strct)
{
    if (strct && strct->type_info && implements_drop(ctx, strct))
    {
        strct->type_info->traits.has_drop = 1;
        strct->type_info->traits.declares_drop = 1;
    }
}

// type_will_drop, `depth` levels into the declarations (a struct may hold itself through a pointer,
// which is not followed, but the guard keeps any other cycle finite).
static int type_will_drop_at(ParserContext *ctx, Type *t, int depth)
{
    if (!t || depth > 32)
    {
        return 0;
    }
    if (type_needs_drop(ctx, t))
    {
        return 1;
    }
    switch (t->kind)
    {
    case TYPE_TUPLE:
        for (int i = 0; i < t->count; i++)
        {
            if (type_will_drop_at(ctx, t->args[i], depth + 1))
            {
                return 1;
            }
        }
        return 0;
    case TYPE_ARRAY:
        return t->array_size > 0 && type_will_drop_at(ctx, t->inner, depth + 1);
    case TYPE_STRUCT:
    case TYPE_ENUM:
    {
        ASTNode *def = t->name ? named_type_def(ctx, t) : NULL;
        if (!def)
        {
            return 0;
        }
        if (implements_drop(ctx, def))
        {
            return 1;
        }
        if (def->kind == NODE_ENUM)
        {
            for (ASTNode *v = def->enm.variants; v; v = v->next)
            {
                if (v->kind == NODE_ENUM_VARIANT &&
                    type_will_drop_at(ctx, v->variant.payload, depth + 1))
                {
                    return 1;
                }
            }
            return 0;
        }
        for (ASTNode *field = def->strct.fields; field; field = field->next)
        {
            if (field->type_info && (field->type_info->kind == TYPE_VECTOR ||
                                     type_will_drop_at(ctx, field->type_info, depth + 1)))
            {
                return 1;
            }
        }
        return 0;
    }
    default:
        return 0;
    }
}

int type_will_drop(ParserContext *ctx, Type *t)
{
    return type_will_drop_at(ctx, t, 0);
}

void propagate_drop_traits(ParserContext *ctx)
{
    for (StructRef *ref = ctx->parsed_structs_list; ref; ref = ref->next)
    {
        seed_drop_from_impl(ctx, ref->node);
    }
    for (StructRef *ref = ctx->parsed_enums_list; ref; ref = ref->next)
    {
        seed_drop_from_impl(ctx, ref->node);
    }
    for (ASTNode *ins = ctx->instantiated_structs; ins; ins = ins->next)
    {
        seed_drop_from_impl(ctx, ins);
    }

    int changed = 1;
    while (changed)
    {
        changed = 0;
        for (StructRef *ref = ctx->parsed_structs_list; ref; ref = ref->next)
        {
            changed |= propagate_drop_from_fields(ctx, ref->node);
        }
        for (StructRef *ref = ctx->parsed_enums_list; ref; ref = ref->next)
        {
            changed |= propagate_drop_from_fields(ctx, ref->node);
        }
        for (ASTNode *ins = ctx->instantiated_structs; ins; ins = ins->next)
        {
            changed |= propagate_drop_from_fields(ctx, ins);
        }
    }
}
