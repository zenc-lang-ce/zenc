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

char *unmangle_ptr_suffix(const char *s);
Type *replace_type_formal(ParserContext *ctx, Type *t, const char *p, const char *c, const char *os,
                          const char *ns);
ASTNode *copy_ast_replacing(ParserContext *ctx, ASTNode *n, const char *p, const char *c,
                            const char *os, const char *ns);

// Helper function to recursively scan AST for sizeof types AND generic calls to trigger
// instantiation
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

void instantiate_type(ParserContext *ctx, Type *instance, Token token)
{
    if (!instance || (instance->kind != TYPE_STRUCT && instance->kind != TYPE_ENUM) ||
        instance->count == 0 || !instance->args)
    {
        return;
    }
    // Instances are still built from the text of their arguments.
    char **args = xmalloc(sizeof(char *) * (size_t)instance->count);
    for (int i = 0; i < instance->count; i++)
    {
        args[i] = type_to_string(instance->args[i]);
        if (is_generic_dependent_str(ctx, args[i]))
        {
            return;
        }
    }
    if (instance->count == 1)
    {
        instantiate_generic(ctx, instance->name, args[0], args[0], token);
    }
    else
    {
        instantiate_generic_multi(ctx, instance->name, args, instance->count, token);
    }
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

    // Instances are still built from the text of their arguments.
    char *subst_arg = NULL;
    for (int i = 0; i < instance.count; i++)
    {
        char *arg = type_to_string(instance.args[i]);
        if (is_generic_dependent_str(ctx, arg))
        {
            return mangled;
        }
        if (!subst_arg)
        {
            subst_arg = arg;
        }
        else
        {
            size_t size = strlen(subst_arg) + 1 + strlen(arg) + 1;
            char *joined = xmalloc(size);
            snprintf(joined, size, "%s,%s", subst_arg, arg);
            subst_arg = joined;
        }
    }

    if (find_func(ctx, mangled))
    {
        return mangled;
    }

    ASTNode *new_fn =
        copy_ast_replacing(ctx, tpl->func_node, tpl->generic_param, subst_arg, NULL, NULL);
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

static ASTNode *copy_fields_replacing(ParserContext *ctx, ASTNode *fields, const char *param,
                                      const char *concrete)
{
    if (!fields)
    {
        return NULL;
    }
    ASTNode *n = ast_create(NODE_FIELD);
    n->field.name = xstrdup(fields->field.name);

    // Replace formal types (Deep Copy)
    n->type_info = replace_type_formal(ctx, fields->type_info, param, concrete, NULL, NULL);

    // The instances the field's type names (`Node<T>*`).
    trigger_type_instantiation(ctx, n->type_info);

    n->next = copy_fields_replacing(ctx, fields->next, param, concrete);
    return n;
}

void instantiate_methods(ParserContext *ctx, GenericImplTemplate *it,
                         const char *mangled_struct_name, const char *arg,
                         const char *unmangled_arg)
{
    ASTNode *backup_next = it->impl_node->next;
    it->impl_node->next = NULL; // Break link to isolate node

    // Use unmangled_arg if provided, otherwise arg
    char *raw = (char *)(unmangled_arg ? unmangled_arg : arg);
    char *subst_arg = unmangle_ptr_suffix(raw);

    ASTNode *new_impl = copy_ast_replacing(ctx, it->impl_node, it->generic_param, subst_arg,
                                           it->struct_name, mangled_struct_name);

    zfree(subst_arg);
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

void instantiate_generic(ParserContext *ctx, const char *tpl, const char *arg,
                         const char *unmangled_arg, Token token)
{
    // Ignore generic placeholders
    if (strlen(arg) == 1 && isupper(arg[0]))
    {
        return;
    }
    if (strcmp(arg, "T") == 0)
    {
        return;
    }

    char *one_arg = (char *)arg;
    char *m = mangle_generic_name(tpl, &one_arg, 1);

    Instantiation *c = ctx->instantiations;
    while (c)
    {
        if (strcmp(c->name, m) == 0)
        {
            zfree(m);
            return; // Already instantiated, DO NOTHING.
        }
        c = c->next;
    }

    GenericTemplate *t = ctx->templates;
    while (t)
    {
        if (strcmp(t->name, tpl) == 0)
        {
            break;
        }
        t = t->next;
    }
    if (!t)
    {
        zpanic_at(token, "Unknown generic: %s", tpl);
        return; // fault tolerance: zpanic_at returned, bail out
    }

    Instantiation *ni = xcalloc(1, sizeof(Instantiation));
    ni->name = xstrdup(m);
    ni->template_name = xstrdup(tpl);
    ni->concrete_arg = xstrdup(arg);
    ni->unmangled_arg = unmangled_arg ? xstrdup(unmangled_arg)
                                      : xstrdup(arg); // Fallback to arg if unmangled is generic
    ni->struct_node = NULL;                           // Placeholder to break cycles
    ni->next = ctx->instantiations;
    ctx->instantiations = ni;

    ASTNode *struct_node_copy = NULL;

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
        const char *gp = (t->struct_node->strct.generic_param_count > 0)
                             ? t->struct_node->strct.generic_params[0]
                             : "T";
        const char *subst_arg = unmangled_arg ? unmangled_arg : arg;
        i->strct.fields = copy_fields_replacing(ctx, t->struct_node->strct.fields, gp, subst_arg);
        struct_node_copy = i;
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
            const char *subst_arg = unmangled_arg ? unmangled_arg : arg;
            nv->variant.payload = replace_type_formal(
                ctx, v->variant.payload, t->struct_node->enm.generic_param, subst_arg, NULL, NULL);

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
        struct_node_copy = i;
    }

    ni->struct_node = struct_node_copy;
    struct_registry_changed(ctx);

    if (struct_node_copy)
    {
        // Cache in hash table for fast lookup.
        if (struct_node_copy->kind == NODE_STRUCT && struct_node_copy->strct.name)
        {
            struct_hash_insert(ctx, struct_node_copy->strct.name, struct_node_copy);
        }
        else if (struct_node_copy->kind == NODE_ENUM && struct_node_copy->enm.name)
        {
            struct_hash_insert(ctx, struct_node_copy->enm.name, struct_node_copy);
        }

        struct_node_copy->next = ctx->instantiated_structs;
        ctx->instantiated_structs = struct_node_copy;
        struct_registry_changed(ctx);
    }

    GenericImplTemplate *it = ctx->impl_templates;
    while (it)
    {
        if (strcmp(it->struct_name, tpl) == 0)
        {
            instantiate_methods(ctx, it, m, arg, unmangled_arg);
        }
        it = it->next;
    }
    zfree(m);
}

static void free_field_list(ASTNode *fields)
{
    while (fields)
    {
        ASTNode *next = fields->next;
        if (fields->field.name)
        {
            zfree(fields->field.name);
        }
        zfree(fields);
        fields = next;
    }
}

void instantiate_generic_multi(ParserContext *ctx, const char *tpl, char **args, int count,
                               Token token)
{
    char *m = mangle_generic_name(tpl, args, count);

    // Check if already instantiated
    Instantiation *c = ctx->instantiations;
    while (c)
    {
        if (strcmp(c->name, m) == 0)
        {
            zfree(m);
            return; // Already done
        }
        c = c->next;
    }

    // Find the template
    GenericTemplate *t = ctx->templates;
    while (t)
    {
        if (strcmp(t->name, tpl) == 0)
        {
            break;
        }
        t = t->next;
    }
    if (!t)
    {
        zpanic_at(token, "Unknown generic: %s", tpl);
        return;
    }

    // Register instantiation first (to break cycles)
    Instantiation *ni = xcalloc(1, sizeof(Instantiation));
    ni->name = xstrdup(m);
    ni->template_name = xstrdup(tpl);
    ni->concrete_arg = (count > 0) ? xstrdup(args[0]) : xstrdup("T");

    // For multi-param, build a comma-separated string for unmangled_arg
    size_t u_len = 0;
    for (int i = 0; i < count; i++)
    {
        u_len += strlen(args[i]) + 1;
    }
    char *u_buf = xmalloc((size_t)(u_len + 1));
    u_buf[0] = 0;
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            strcat(u_buf, ",");
        }
        strcat(u_buf, args[i]);
    }
    ni->unmangled_arg = u_buf;

    ni->struct_node = NULL;
    ni->next = ctx->instantiations;
    ctx->instantiations = ni;

    if (t->struct_node->kind == NODE_STRUCT)
    {
        ASTNode *i = ast_create(NODE_STRUCT);
        i->strct.name = xstrdup(m);
        i->strct.is_template = 0;
        i->strct.is_export = t->struct_node->strct.is_export;

        // Copy struct attributes
        i->strct.is_packed = t->struct_node->strct.is_packed;
        i->strct.is_union = t->struct_node->strct.is_union;
        i->strct.align = t->struct_node->strct.align;
        if (t->struct_node->strct.parent)
        {
            i->strct.parent = xstrdup(t->struct_node->strct.parent);
        }

        // Copy fields with sequential substitutions for each param
        ASTNode *fields = t->struct_node->strct.fields;
        int param_count = t->struct_node->strct.generic_param_count;

        if (param_count > 0 && count > 0)
        {
            // First substitution
            i->strct.fields = copy_fields_replacing(
                ctx, fields, t->struct_node->strct.generic_params[0], args[0]);

            // Subsequent substitutions (for params B, C, etc.)
            for (int j = 1; j < param_count && j < count; j++)
            {
                ASTNode *prev_fields = i->strct.fields;
                ASTNode *tmp = copy_fields_replacing(
                    ctx, prev_fields, t->struct_node->strct.generic_params[j], args[j]);
                free_field_list(prev_fields);
                i->strct.fields = tmp;
            }
        }
        else
        {
            i->strct.fields = copy_fields_replacing(ctx, fields, "T", "int");
        }

        ni->struct_node = i;
        register_struct_def(ctx, m, i);

        i->next = ctx->instantiated_structs;
        ctx->instantiated_structs = i;
        struct_registry_changed(ctx);
    }
    else if (t->struct_node->kind == NODE_ENUM)
    {
        ASTNode *i = ast_create(NODE_ENUM);
        i->enm.name = xstrdup(m);
        i->enm.is_template = 0;
        i->enm.is_export = t->struct_node->enm.is_export;

        // Copy type attributes
        i->type_info = type_new(TYPE_ENUM);
        i->type_info->name = xstrdup(m);
        if (t->struct_node->type_info)
        {
            i->type_info->traits = t->struct_node->type_info->traits;
        }

        ASTNode *h = 0, *tl = 0;
        ASTNode *v = t->struct_node->enm.variants;

        // Construct comma-separated concrete args string
        size_t c_args_len = 1;
        for (int j = 0; j < count; j++)
        {
            c_args_len += strlen(args[j]) + 1;
        }
        char *c_args = xmalloc((size_t)(c_args_len));
        c_args[0] = 0;
        for (int j = 0; j < count; j++)
        {
            if (j > 0)
            {
                strcat(c_args, ",");
            }
            strcat(c_args, args[j]);
        }

        while (v)
        {
            ASTNode *nv = ast_create(NODE_ENUM_VARIANT);
            nv->variant.name = xstrdup(v->variant.name);
            nv->variant.tag_id = v->variant.tag_id;
            nv->variant.payload_count = v->variant.payload_count;

            // Use multi-parameter substitution for payload
            Type *payload = v->variant.payload;
            nv->variant.payload = NULL;
            if (payload)
            {
                nv->variant.payload = replace_type_formal(
                    ctx, payload, t->struct_node->enm.generic_param, c_args, NULL, NULL);
            }

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
        zfree(c_args);
        i->enm.variants = h;
        ni->struct_node = i;
        register_struct_def(ctx, m, i);

        i->next = ctx->instantiated_structs;
        ctx->instantiated_structs = i;
        struct_registry_changed(ctx);
    }
    zfree(m);
}
