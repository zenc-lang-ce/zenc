// SPDX-License-Identifier: MIT
#include "plugins/plugin_manager.h"
#include "parser.h"
#include "utils/format_expr.h"
#include "utils/colors.h"
#include "utils/utils.h"
#include "constants.h"
#include "ast/primitives.h"
#include <ctype.h>
#include "analysis/const_fold.h"
#include <stdlib.h>
#include <string.h>

// Helper function to recursively scan AST for sizeof types AND generic calls to trigger
// instantiation
Instantiation *find_instantiation(ParserContext *ctx, const char *name)
{
    for (Instantiation *inst = ctx->instantiations; inst; inst = inst->next)
    {
        if (strcmp(inst->name, name) == 0)
        {
            return inst;
        }
    }
    return NULL;
}

GenericTemplate *find_generic_template(ParserContext *ctx, const char *name)
{
    for (GenericTemplate *gt = ctx->templates; gt; gt = gt->next)
    {
        if (strcmp(gt->name, name) == 0)
        {
            return gt;
        }
    }
    return NULL;
}

// Whether an instance's arguments still name a generic parameter (`Vec<T>` inside its
// template): it is instantiated once the parameter is bound.
static int args_name_generic(ParserContext *ctx, Type **args, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (type_is_generic_dependent(ctx, args[i]))
        {
            return 1;
        }
    }
    return 0;
}

static void instantiate_generic(ParserContext *ctx, const char *tpl, Type **args, int count,
                                Token token);

void instantiate_type(ParserContext *ctx, Type *instance, Token token)
{
    if (!instance || (instance->kind != TYPE_STRUCT && instance->kind != TYPE_ENUM) ||
        instance->count == 0 || !instance->args ||
        args_name_generic(ctx, instance->args, instance->count))
    {
        return;
    }
    instantiate_generic(ctx, instance->name, instance->args, instance->count, token);
}

static void trigger_type_instantiation(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return;
    }

    // Handle slices
    if (t->kind == TYPE_ARRAY && t->array_size == 0 && t->inner)
    {
        char *inner_str = type_to_string(t->inner);
        register_slice(ctx, inner_str);
        zfree(inner_str);
    }

    if ((t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->count > 0 && t->args && t->name)
    {
        if (find_generic_template(ctx, t->name))
        {
            Token no_token = {0};
            instantiate_type(ctx, t, no_token);
        }
    }

    // Recursive scan
    trigger_type_instantiation(ctx, t->inner);
    if (t->args)
    {
        for (int i = 0; i < t->count; i++)
        {
            trigger_type_instantiation(ctx, t->args[i]);
        }
    }
}

static void trigger_instantiations(ParserContext *ctx, ASTNode *node)
{
    if (!node)
    {
        return;
    }

    // Process type information
    if (node->type_info)
    {
        trigger_type_instantiation(ctx, node->type_info);
    }

    // Process current node
    if (node->kind == NODE_EXPR_SIZEOF)
    {
        trigger_type_instantiation(ctx, node->size_of.target_type_info);
    }
    else if (node->kind == NODE_EXPR_VAR && node->var_ref.instance.name)
    {
        instantiate_function(ctx, node->var_ref.instance);
    }

    ASTChildren children = ast_children(node);
    for (int i = 0; i < children.count; i++)
    {
        trigger_instantiations(ctx, *children.slots[i]);
    }

    // Visit next sibling
    trigger_instantiations(ctx, node->next);
}

char *function_instance_name(FunctionInstance instance)
{
    char **args = xmalloc(sizeof(char *) * (size_t)instance.count);
    for (int i = 0; i < instance.count; i++)
    {
        args[i] = type_to_string(instance.args[i]);
    }
    return mangle_generic_name(instance.name, args, instance.count);
}

char *instantiate_function(ParserContext *ctx, FunctionInstance instance)
{
    GenericFuncTemplate *tpl = find_func_template(ctx, instance.name);
    if (!tpl)
    {
        return NULL;
    }

    char *mangled = function_instance_name(instance);
    if (args_name_generic(ctx, instance.args, instance.count) || find_func(ctx, mangled))
    {
        return mangled;
    }

    Bindings b = bindings_new(tpl->generic_param, instance.args, instance.count);
    ASTNode *new_fn = copy_ast_replacing(ctx, tpl->func_node, &b, NULL, NULL);
    if (!new_fn || new_fn->kind != NODE_FUNCTION)
    {
        return NULL;
    }

    zfree(new_fn->func.name);
    new_fn->func.name = xstrdup(mangled);
    new_fn->func.generic_params = NULL;

    // The instances the copy's signature names (`-> Triple<X, Y, Z>`).
    trigger_type_instantiation(ctx, new_fn->func.ret_type_info);
    for (int i = 0; i < new_fn->func.count; i++)
    {
        trigger_type_instantiation(ctx, new_fn->func.arg_types[i]);
    }

    add_instantiated_func(ctx, new_fn);

    register_func(ctx, ctx->global_scope, mangled, new_fn->func.count, new_fn->func.default_values,
                  new_fn->func.arg_types, new_fn->func.ret_type_info, new_fn->func.is_varargs,
                  new_fn->func.pure, new_fn->link_name, new_fn->token, new_fn->func.is_export);

    trigger_instantiations(ctx, new_fn->func.body);

    if (new_fn->func.arg_types)
    {
        for (int i = 0; i < new_fn->func.count; i++)
        {
            Type *at = new_fn->func.arg_types[i];
            if (at && at->kind == TYPE_ARRAY && at->array_size == 0 && at->inner)
            {
                char *inner_str = type_to_string(at->inner);
                register_slice(ctx, inner_str);
                zfree(inner_str);
            }
        }
    }

    return mangled;
}

void register_template(ParserContext *ctx, const char *name, ASTNode *node)
{
    GenericTemplate *t = xcalloc(1, sizeof(GenericTemplate));
    t->name = xstrdup(name);
    t->struct_node = node;
    t->next = ctx->templates;
    ctx->templates = t;
}

static ASTNode *copy_fields_replacing(ParserContext *ctx, ASTNode *fields, const Bindings *b)
{
    if (!fields)
    {
        return NULL;
    }
    ASTNode *n = ast_create(NODE_FIELD);
    n->field.name = xstrdup(fields->field.name);

    // Replace formal types (Deep Copy)
    n->type_info = replace_type_formal(ctx, fields->type_info, b, NULL, NULL);

    // The instances the field's type names (`Node<T>*`).
    trigger_type_instantiation(ctx, n->type_info);

    n->next = copy_fields_replacing(ctx, fields->next, b);
    return n;
}

void instantiate_methods(ParserContext *ctx, GenericImplTemplate *it,
                         const char *mangled_struct_name, Type **args, int count)
{
    ASTNode *backup_next = it->impl_node->next;
    it->impl_node->next = NULL; // Break link to isolate node

    Bindings b = bindings_new(it->generic_param, args, count);
    ASTNode *new_impl =
        copy_ast_replacing(ctx, it->impl_node, &b, it->struct_name, mangled_struct_name);

    it->impl_node->next = backup_next; // Restore

    ASTNode *meth = NULL;

    if (new_impl->kind == NODE_IMPL)
    {
        new_impl->impl.struct_name = xstrdup(mangled_struct_name);
        meth = new_impl->impl.methods;
    }
    else if (new_impl->kind == NODE_IMPL_TRAIT)
    {
        new_impl->impl_trait.target_type = xstrdup(mangled_struct_name);
        meth = new_impl->impl_trait.methods;
    }

    while (meth)
    {
        // Standardize: ensure __ between type and method
        // If it's already correctly mangled (e.g. Vec__int32_t__with_capacity), skip
        size_t mlen = strlen(mangled_struct_name);
        int correctly_mangled =
            (strncmp(meth->func.name, mangled_struct_name, (size_t)(mlen)) == 0 &&
             meth->func.name[mlen] == '_' && meth->func.name[mlen + 1] == '_');

        if (!correctly_mangled)
        {
            // Find the method part in the original name (e.g. "with_capacity" in
            // "Vec_with_capacity")
            char *original_method = meth->func.name;
            if (strncmp(original_method, it->struct_name, strlen(it->struct_name)) == 0)
            {
                original_method += strlen(it->struct_name);
                // Strip the separator (legacy single "_" or the "__" separator),
                // but keep the method's own leading underscores so that e.g.
                // "_private_method" stays distinct from "private_method".
                if (original_method[0] == '_' && original_method[1] == '_')
                {
                    original_method += 2;
                }
                else if (original_method[0] == '_')
                {
                    original_method += 1;
                }
            }

            char *new_name = mangle_method_symbol(mangled_struct_name, NULL, original_method);
            zfree(meth->func.name);
            meth->func.name = new_name;
        }

        register_func(ctx, ctx->global_scope, meth->func.name, meth->func.count,
                      meth->func.default_values, meth->func.arg_types, meth->func.ret_type_info,
                      meth->func.is_varargs, meth->func.pure, meth->link_name, meth->token,
                      meth->func.is_export);

        // The instances the method's return type names (`-> Option<T>`).
        trigger_type_instantiation(ctx, meth->func.ret_type_info);

        trigger_instantiations(ctx, meth->func.body);

        meth = meth->next;
    }
    add_instantiated_func(ctx, new_impl);
}

static void register_enum_constructor(ParserContext *ctx, const char *m, const char *var_name,
                                      int tag_id, Type *payload, Token token, int is_export)
{
    char *mangled_var = mangle_variant_symbol(m, var_name);
    register_enum_variant(ctx, var_name, m, tag_id);

    Type *ret_t = type_new(TYPE_ENUM);
    ret_t->name = xstrdup(m);

    if (payload)
    {
        Type **at = xmalloc(sizeof(Type *));
        at[0] = payload;
        register_func(ctx, ctx->global_scope, mangled_var, 1, NULL, at, ret_t, 0, 0, NULL, token,
                      is_export);
    }
    else
    {
        register_func(ctx, ctx->global_scope, mangled_var, 0, NULL, NULL, ret_t, 0, 0, NULL, token,
                      is_export);
    }
    zfree(mangled_var);
}

// The parameters a struct or enum template declares, as `K,V`.
static char *template_params(ASTNode *def)
{
    if (def->kind == NODE_ENUM)
    {
        return def->enm.generic_param;
    }
    return join_with_commas(def->strct.generic_params, def->strct.generic_param_count);
}

// Instantiates the template `tpl` with `args`: its struct or enum, the constructors of its
// variants, and the methods of the generic impls registered so far (the later ones are
// instantiated when they are registered).
static void instantiate_generic(ParserContext *ctx, const char *tpl, Type **args, int count,
                                Token token)
{
    Type instance = {0};
    instance.kind = TYPE_STRUCT;
    instance.name = (char *)tpl;
    instance.args = args;
    instance.count = count;
    char *m = type_def_name(&instance);
    if (find_instantiation(ctx, m))
    {
        return;
    }

    GenericTemplate *t = find_generic_template(ctx, tpl);
    if (!t)
    {
        zpanic_at(token, "Unknown generic: %s", tpl);
        return; // fault tolerance: zpanic_at returned, bail out
    }

    // Registered before the copy, which may name the instance again (`Node<T>*`).
    Instantiation *ni = xcalloc(1, sizeof(Instantiation));
    ni->name = xstrdup(m);
    ni->template_name = xstrdup(tpl);
    ni->args = args;
    ni->count = count;
    ni->next = ctx->instantiations;
    ctx->instantiations = ni;

    Bindings b = bindings_new(template_params(t->struct_node), args, count);
    ASTNode *copy = NULL;

    if (t->struct_node->kind == NODE_STRUCT)
    {
        ASTNode *i = ast_create(NODE_STRUCT);
        i->strct.name = xstrdup(m);
        i->strct.is_template = 0;
        i->strct.is_export = t->struct_node->strct.is_export;

        // Copy type attributes (e.g. has_drop)
        i->type_info = type_new(TYPE_STRUCT);
        i->type_info->name = xstrdup(m);
        if (t->struct_node->type_info)
        {
            i->type_info->traits = t->struct_node->type_info->traits;
            i->type_info->is_restrict = t->struct_node->type_info->is_restrict;
        }
        i->strct.is_packed = t->struct_node->strct.is_packed;
        i->strct.is_union = t->struct_node->strct.is_union;
        i->strct.align = t->struct_node->strct.align;
        if (t->struct_node->strct.parent)
        {
            i->strct.parent = xstrdup(t->struct_node->strct.parent);
        }
        i->strct.fields = copy_fields_replacing(ctx, t->struct_node->strct.fields, &b);
        copy = i;
        register_struct_def(ctx, m, i);

        // Register slice types used in the instantiated struct's fields
        ASTNode *fld = i->strct.fields;
        while (fld)
        {
            char *field_type = fld->type_info ? type_to_string(fld->type_info) : NULL;
            if (field_type && strncmp(field_type, "Slice__", 7) == 0)
            {
                register_slice(ctx, field_type + 7);
            }
            fld = fld->next;
        }
    }
    else if (t->struct_node->kind == NODE_ENUM)
    {
        ASTNode *i = ast_create(NODE_ENUM);
        i->enm.name = xstrdup(m);
        i->enm.is_template = 0;
        i->enm.is_export = t->struct_node->enm.is_export;

        // Copy type attributes (e.g. has_drop)
        i->type_info = type_new(TYPE_ENUM);
        i->type_info->name = xstrdup(m);
        if (t->struct_node->type_info)
        {
            i->type_info->traits = t->struct_node->type_info->traits;
        }

        ASTNode *h = 0, *tl = 0;
        ASTNode *v = t->struct_node->enm.variants;
        while (v)
        {
            ASTNode *nv = ast_create(NODE_ENUM_VARIANT);
            nv->variant.name = xstrdup(v->variant.name);
            nv->variant.tag_id = v->variant.tag_id;
            nv->variant.payload_count = v->variant.payload_count;
            nv->variant.payload = replace_type_formal(ctx, v->variant.payload, &b, NULL, NULL);

            register_enum_constructor(ctx, m, nv->variant.name, nv->variant.tag_id,
                                      nv->variant.payload, token, i->enm.is_export);

            if (!h)
            {
                h = nv;
            }
            else
            {
                tl->next = nv;
            }
            tl = nv;
            v = v->next;
        }
        i->enm.variants = h;
        copy = i;
        register_struct_def(ctx, m, i);
    }

    ni->struct_node = copy;
    if (copy)
    {
        // Cache in hash table for fast lookup.
        struct_hash_insert(ctx, m, copy);
        copy->next = ctx->instantiated_structs;
        ctx->instantiated_structs = copy;
    }
    struct_registry_changed(ctx);

    for (GenericImplTemplate *it = ctx->impl_templates; it; it = it->next)
    {
        if (strcmp(it->struct_name, tpl) == 0)
        {
            instantiate_methods(ctx, it, m, args, count);
        }
    }
}
