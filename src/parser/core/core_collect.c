// SPDX-License-Identifier: MIT
// collect: registers each declaration where later code looks it up, from its node.
#include "parser.h"
#include "ast/ast.h"
#include <string.h>

// A struct, or an enum, by its C name: the type's name, and the definition's or the template's.
static void collect_type(ParserContext *ctx, ASTNode *node)
{
    char *name = declared_type_c_name(node);
    int is_template = node->kind == NODE_STRUCT ? node->strct.is_template : node->enm.is_template;
    if (node->kind == NODE_STRUCT)
    {
        add_to_struct_list(ctx, node);
        if (is_template)
        {
            register_template(ctx, name, node);
        }
        else
        {
            register_struct_def(ctx, name, node);
        }
        return;
    }
    // An enum template is a definition too: `Option` names it without its argument.
    if (is_template)
    {
        register_template(ctx, name, node);
    }
    register_struct_def(ctx, name, node);
    add_to_enum_list(ctx, node);
}

// A function outside an impl: a template by its written name, a concrete one by its C name.
static void collect_function(ParserContext *ctx, ASTNode *node)
{
    if (node->func.generic_param_count > 0)
    {
        register_func_template(ctx, node->func.name, node->func.generic_params,
                               node->func.generic_param_count, node);
        register_declaration(ctx, node); // A template is not handed to the program.
        return;
    }
    register_function(ctx, ctx->current_scope, node, function_c_name(node), node->func.is_export);
    add_to_func_list(ctx, node);
}

// The methods of an impl: templates of the impl's parameter `template_param` or of their own,
// or functions, by their C names (Type__method, Type__Trait__method).
static void collect_methods(ParserContext *ctx, ASTNode *methods, const char *template_param)
{
    for (ASTNode *f = methods; f; f = f->next)
    {
        if (template_param)
        {
            char **params = xmalloc(sizeof(char *));
            params[0] = xstrdup(template_param);
            register_func_template(ctx, function_c_name(f), params, 1, f);
        }
        else if (f->func.generic_param_count > 0)
        {
            register_func_template(ctx, function_c_name(f), f->func.generic_params,
                                   f->func.generic_param_count, f);
        }
        else
        {
            register_function(ctx, ctx->current_scope, f, function_c_name(f), f->func.is_export);
        }
    }
}

// `impl Box` adds functions; `impl Box<T>` is a template, instantiated with each `Box<X>`.
static void collect_impl(ParserContext *ctx, ASTNode *node)
{
    char *param = node->impl.generic_param;
    collect_methods(ctx, node->impl.methods, param);
    if (!param)
    {
        add_to_impl_list(ctx, node);
        return;
    }
    char **params = xmalloc(sizeof(char *));
    params[0] = param;
    register_impl_template(ctx, type_impl_name(node->impl.target), params, 1, node);
}

// The parameter of the generic type a trait impl is for, or NULL: the one written
// (`for Box<T>`), or the first of the type's declaration (`for Box`).
static char *impl_trait_template_param(ParserContext *ctx, ASTNode *node, const char *type_name)
{
    if (node->impl_trait.generic_param)
    {
        return node->impl_trait.generic_param;
    }
    ASTNode *def = find_struct_def(ctx, type_name);
    if (def && def->kind == NODE_STRUCT && def->strct.is_template)
    {
        return def->strct.generic_param_count > 0 ? def->strct.generic_params[0] : "T";
    }
    if (def && def->kind == NODE_ENUM && def->enm.is_template)
    {
        return def->enm.generic_param_count > 0 ? def->enm.generic_params[0] : "T";
    }
    return NULL;
}

// A trait impl for a generic type is a template too, instantiated with each instance of the type.
static void register_impl_trait_template(ParserContext *ctx, ASTNode *node)
{
    char *type_name = type_impl_name(node->impl_trait.target);
    char *param = impl_trait_template_param(ctx, node, type_name);
    if (param)
    {
        char **params = xmalloc(sizeof(char *));
        params[0] = param;
        register_impl_template(ctx, type_name, params, 1, node);
    }
}

void resolve_waiting_impls(ParserContext *ctx)
{
    for (StructRef *r = ctx->waiting_impls; r; r = r->next)
    {
        ASTNode *node = r->node;
        if (impl_trait_template_param(ctx, node, type_impl_name(node->impl_trait.target)))
        {
            // Its methods were read as concrete ones: an instance made before their bodies
            // are read gets a copy of them, as from any template.
            move_bodies_to_templates(ctx, node->impl_trait.methods);
        }
        register_impl_trait_template(ctx, node);
    }
    ctx->waiting_impls = NULL;
}

// `impl Trait for Type`: the type implements the trait, and its methods are functions; for a
// generic type the impl is a template too.
static void collect_impl_trait(ParserContext *ctx, ASTNode *node)
{
    char *trait = node->impl_trait.trait_name;
    char *type_name = type_impl_name(node->impl_trait.target);
    register_impl(ctx, trait, type_name);
    if (strcmp(trait, "Iterable") == 0)
    {
        ZenSymbol *s = find_symbol_entry(ctx, type_name);
        ASTNode *def = s && s->type_info ? NULL : find_struct_def(ctx, type_name);
        Type *t = s && s->type_info ? s->type_info : (def ? def->type_info : NULL);
        if (t)
        {
            t->traits.has_iterable = 1;
        }
    }
    collect_methods(ctx, node->impl_trait.methods, NULL);
    add_to_impl_list(ctx, node);
    // While declarations are read, a type written without its parameter (`for Box`) may be
    // declared further down: whether it is generic is known once every declaration is.
    if (!node->impl_trait.generic_param && !ctx->declarations_read &&
        !find_struct_def(ctx, type_name))
    {
        StructRef *waiting = xmalloc(sizeof(StructRef));
        waiting->node = node;
        waiting->next = NULL;
        StructRef **last = &ctx->waiting_impls;
        while (*last)
        {
            last = &(*last)->next;
        }
        *last = waiting;
        return;
    }
    register_impl_trait_template(ctx, node);
}

void collect_declaration(ParserContext *ctx, ASTNode *node)
{
    switch (node->kind)
    {
    case NODE_STRUCT:
    case NODE_ENUM:
        collect_type(ctx, node);
        break;
    case NODE_FUNCTION:
        collect_function(ctx, node);
        break;
    case NODE_IMPL:
        collect_impl(ctx, node);
        break;
    case NODE_IMPL_TRAIT:
        collect_impl_trait(ctx, node);
        break;
    case NODE_TYPE_ALIAS:
        register_type_alias(ctx, node->type_alias.alias, node->type_info,
                            node->type_alias.is_opaque, node->type_alias.defined_in_file,
                            node->token, node->type_alias.is_export);
        break;
    case NODE_TRAIT:    // For its vtable and is_trait.
    case NODE_CONST:    // A global one.
    case NODE_VAR_DECL: // A global one.
        add_to_global_list(ctx, node);
        break;
    default:
        break;
    }
}
