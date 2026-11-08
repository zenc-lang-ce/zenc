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

// The instance of a generic whose C name is `name`, or NULL.
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

// Whether `t` names a type that is not declared yet: one that a declaration further down, or
// a generic instance still waiting, may provide.
static int names_undeclared_type(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return 0;
    }
    switch (t->kind)
    {
    case TYPE_STRUCT:
    case TYPE_ENUM:
        if (!t->name || t->is_explicit_struct)
        {
            return 0;
        }
        if (t->count > 0)
        {
            return !find_struct_def(ctx, type_def_name(t));
        }
        return !find_struct_def(ctx, t->name) && !find_type_alias_node(ctx, t->name) &&
               !is_primitive_type_name(t->name) && !is_known_generic(ctx, t->name) &&
               !is_extern_symbol(ctx, t->name) && !find_generic_template(ctx, t->name);
    case TYPE_TUPLE:
    case TYPE_FUNCTION:
        for (int i = 0; i < t->count; i++)
        {
            if (names_undeclared_type(ctx, t->args[i]))
            {
                return 1;
            }
        }
        return names_undeclared_type(ctx, t->inner);
    default:
        return names_undeclared_type(ctx, t->inner);
    }
}

void instantiate_type(ParserContext *ctx, Type *instance, Token token)
{
    if (!instance || (instance->kind != TYPE_STRUCT && instance->kind != TYPE_ENUM) ||
        instance->count == 0 || !instance->args ||
        args_name_generic(ctx, instance->args, instance->count))
    {
        return;
    }
    // While declarations are read, the template or an argument may be declared further down.
    if (!ctx->declarations_read &&
        (!find_generic_template(ctx, instance->name) ||
         names_undeclared_type(
             ctx, &(Type){.kind = TYPE_TUPLE, .args = instance->args, .count = instance->count})))
    {
        PendingInstance *p = xcalloc(1, sizeof(PendingInstance));
        p->type = instance;
        p->token = token;
        if (ctx->pending_instances_tail)
        {
            ctx->pending_instances_tail->next = p;
        }
        else
        {
            ctx->pending_instances = p;
        }
        ctx->pending_instances_tail = p;
        return;
    }
    instantiate_generic(ctx, instance->name, instance->args, instance->count, token);
}

// Replaces, in place, every type `t` names that turned out to be an alias by the type it names.
static void replace_alias_names(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return;
    }
    if ((t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->count == 0 && t->name)
    {
        TypeAlias *alias = find_type_alias_node(ctx, t->name);
        if (alias)
        {
            *t = *alias_type(alias);
        }
        return;
    }
    for (int i = 0; t->args && i < t->count; i++)
    {
        replace_alias_names(ctx, t->args[i]);
    }
    replace_alias_names(ctx, t->inner);
}

void resolve_pending_instances(ParserContext *ctx)
{
    PendingInstance *p = ctx->pending_instances;
    ctx->pending_instances = NULL;
    ctx->pending_instances_tail = NULL;
    for (; p; p = p->next)
    {
        Type *instance = p->type;
        for (int i = 0; i < instance->count; i++)
        {
            replace_alias_names(ctx, instance->args[i]);
        }
        instantiate_type(ctx, instance, p->token);
    }
}

static void trigger_type_instantiation(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return;
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

// A function copied for an instance before the body of `template_function` was read: it gets
// its copy of the body in copy_instance_bodies.
static void remember_body_copy(ParserContext *ctx, ASTNode *instance, ASTNode *template_function,
                               Bindings bindings, const char *template_name,
                               const char *instance_name)
{
    if (template_function->func.body || instance->kind != NODE_FUNCTION)
    {
        return;
    }
    BodyCopy *c = xcalloc(1, sizeof(BodyCopy));
    c->instance = instance;
    c->template_function = template_function;
    c->bindings = bindings;
    c->template_name = template_name;
    c->instance_name = instance_name;
    c->next = ctx->body_copies;
    ctx->body_copies = c;
}

void copy_instance_bodies(ParserContext *ctx)
{
    while (ctx->body_copies)
    {
        // Taken out first: the generics the body names may copy more functions.
        BodyCopy *c = ctx->body_copies;
        ctx->body_copies = c->next;
        c->instance->func.body = copy_ast_replacing(
            ctx, c->template_function->func.body, &c->bindings, c->template_name, c->instance_name);
        trigger_instantiations(ctx, c->instance->func.body);
    }
}

char *function_instance_name(FunctionInstance instance)
{
    char **args = xmalloc(sizeof(char *) * (size_t)instance.count);
    for (int i = 0; i < instance.count; i++)
    {
        args[i] = type_mangle_spelling(instance.args[i]);
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

    remember_body_copy(ctx, new_fn, tpl->func_node, b, NULL, NULL);
    add_instantiated_func(ctx, new_fn);

    register_func(ctx, ctx->global_scope, mangled, new_fn->func.count, new_fn->func.default_values,
                  new_fn->func.arg_types, new_fn->func.ret_type_info, new_fn->func.is_varargs,
                  new_fn->func.pure, new_fn->link_name, NULL, new_fn->token,
                  new_fn->func.is_export);

    trigger_instantiations(ctx, new_fn->func.body);

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
    ASTNode *template_meth = NULL;
    const char *trait_name = NULL;
    Type *target = type_new_instance(it->struct_name, args, count);

    if (new_impl->kind == NODE_IMPL)
    {
        new_impl->impl.target = target;
        meth = new_impl->impl.methods;
        template_meth = it->impl_node->impl.methods;
    }
    else if (new_impl->kind == NODE_IMPL_TRAIT)
    {
        new_impl->impl_trait.target = target;
        trait_name = new_impl->impl_trait.trait_name;
        meth = new_impl->impl_trait.methods;
        template_meth = it->impl_node->impl_trait.methods;
    }

    while (meth && template_meth)
    {
        remember_body_copy(ctx, meth, template_meth, b, it->struct_name, mangled_struct_name);

        // The instance's name for the method: `Vec__int32_t__push`, `Wrap__int32_t__Show__show`.
        meth->func.name =
            mangle_method_symbol(mangled_struct_name, trait_name, meth->func.method_name);

        register_func(ctx, ctx->global_scope, meth->func.name, meth->func.count,
                      meth->func.default_values, meth->func.arg_types, meth->func.ret_type_info,
                      meth->func.is_varargs, meth->func.pure, meth->link_name,
                      meth->func.method_name, meth->token, meth->func.is_export);

        // The instances the method's return type names (`-> Option<T>`).
        trigger_type_instantiation(ctx, meth->func.ret_type_info);

        trigger_instantiations(ctx, meth->func.body);

        meth = meth->next;
        template_meth = template_meth->next;
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
        register_func(ctx, ctx->global_scope, mangled_var, 1, NULL, at, ret_t, 0, 0, NULL,
                      xstrdup(var_name), token, is_export);
    }
    else
    {
        register_func(ctx, ctx->global_scope, mangled_var, 0, NULL, NULL, ret_t, 0, 0, NULL,
                      xstrdup(var_name), token, is_export);
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
