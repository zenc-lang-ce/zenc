// SPDX-License-Identifier: MIT
// collect: registers each declaration where later code looks it up, from its node.
#include "parser.h"
#include "ast/ast.h"
#include "constants.h"
#include <stdio.h>
#include <string.h>

// The struct `node`, named `name` in C, against what is registered before it: another concrete
// struct (or enum) by that name is a redefinition. Generic templates are registered apart, so a
// concrete struct and a generic one with the same name (a user `struct Box` and the std's
// `Box<T>`) would be merged without a word: the generic's impls, Drop included, would apply to
// the concrete struct.
static void check_struct_name(ParserContext *ctx, ASTNode *node, const char *name)
{
    if (ctx->config->mode_lsp)
    {
        return;
    }
    int gp_count = node->strct.generic_param_count;
    const char *own_name = node->strct.name;
    if (gp_count == 0 && find_concrete_struct_def(ctx, name))
    {
        zerror_at(node->token, "Redefinition of %s '%s'", node->strct.is_union ? "union" : "struct",
                  own_name);
    }
    GenericTemplate *tpl = ctx->templates;
    while (tpl && strcmp(tpl->name, name) != 0)
    {
        tpl = tpl->next;
    }
    ASTNode *concrete = gp_count > 0 ? find_concrete_struct_def(ctx, name) : NULL;
    concrete = concrete && concrete->kind == NODE_STRUCT ? concrete : NULL;
    if ((gp_count == 0 && tpl) || concrete)
    {
        ASTNode *other = tpl ? tpl->struct_node : concrete;
        const char *where =
            (other && other->strct.defined_in_file) ? other->strct.defined_in_file : "?";
        char msg[MAX_SHORT_MSG_LEN];
        snprintf(msg, sizeof(msg), "Struct '%s' clashes with the %s struct '%s' from %s", own_name,
                 gp_count == 0 ? "generic" : "concrete", own_name, where);
        const char *hints[] = {"A generic and a concrete struct cannot share a name",
                               "Rename one of them (std types such as Box, Vec, Option, "
                               "Map or String keep their names)",
                               NULL};
        zpanic_with_hints(node->token, msg, hints);
    }
}

void collect_variant(ParserContext *ctx, ASTNode *enum_node, ASTNode *variant)
{
    const char *enum_name = enum_node->type_info->name;
    char *name = variant->variant.name;
    register_enum_variant(ctx, name, enum_name, variant->variant.tag_id);
    if (variant->variant.value_pending)
    {
        EnumVariantReg *reg = find_enum_variant_of(ctx, enum_name, name);
        if (reg)
        {
            reg->tag_pending = 1;
        }
    }

    // A variant that carries values has a constructor, named after the enum; with @link_name,
    // C calls it by that name. A generic enum's are made with each instance.
    Type *payload = variant->variant.payload;
    if (!payload || enum_node->enm.is_template)
    {
        return;
    }
    char *mangled = mangle_variant_symbol(enum_name, name);
    char *link_name = enum_node->enm.written_link_name;
    char *c_mangled = link_name ? mangle_variant_symbol(link_name, name) : mangled;
    // Several values make a tuple, and each is a parameter. One value that is itself a tuple
    // (`Pair((int, int))`) is registered without parameters, as it was.
    int several = variant->variant.payload_count > 1;
    int count = several ? payload->count : 0;
    Type **arg_types = several ? payload->args : NULL;
    if (payload->kind != TYPE_TUPLE)
    {
        count = 1;
        arg_types = xmalloc(sizeof(Type *));
        arg_types[0] = payload;
    }
    register_func(ctx, ctx->current_scope, mangled, count, NULL, arg_types,
                  type_new_enum(enum_name), 0, c_mangled, name, variant->token,
                  enum_node->enm.is_export);
}

// A struct, or an enum, by its C name: the type's name, and the definition's or the template's.
static void collect_type(ParserContext *ctx, ASTNode *node)
{
    char *name = declared_type_c_name(node);
    int is_template = node->kind == NODE_STRUCT ? node->strct.is_template : node->enm.is_template;
    if (node->kind == NODE_STRUCT)
    {
        check_struct_name(ctx, node, name);
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
    for (ASTNode *variant = node->enm.variants; variant && !node->enm.variants_collected;
         variant = variant->next)
    {
        collect_variant(ctx, node, variant);
    }
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
    char *param = impl_trait_template_param(ctx, node, type_name);
    if (param)
    {
        char **params = xmalloc(sizeof(char *));
        params[0] = param;
        register_impl_template(ctx, type_name, params, 1, node);
    }
}

// Registers the declaration `node` at once.
static void register_collected(ParserContext *ctx, ASTNode *node)
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

void collect_declaration(ParserContext *ctx, ASTNode *node)
{
    if (ctx->declarations_read)
    {
        register_collected(ctx, node);
        return;
    }
    PendingCollection *p = xcalloc(1, sizeof(PendingCollection));
    p->node = node;
    p->scope = ctx->current_scope;
    p->module_prefix = ctx->imports.current_module_prefix;
    p->filename = ctx->current_filename;
    if (ctx->pending_collections_tail)
    {
        ctx->pending_collections_tail->next = p;
    }
    else
    {
        ctx->pending_collections = p;
    }
    ctx->pending_collections_tail = p;
}

void collect_pending_declarations(ParserContext *ctx)
{
    PendingCollection *pending = ctx->pending_collections;
    ctx->pending_collections = NULL;
    ctx->pending_collections_tail = NULL;
    Scope *saved_scope = ctx->current_scope;
    char *saved_prefix = ctx->imports.current_module_prefix;
    const char *saved_filename = ctx->current_filename;
    for (PendingCollection *p = pending; p; p = p->next)
    {
        ctx->current_scope = p->scope;
        ctx->imports.current_module_prefix = p->module_prefix;
        ctx->current_filename = p->filename;
        register_collected(ctx, p->node);
    }
    ctx->current_scope = saved_scope;
    ctx->imports.current_module_prefix = saved_prefix;
    ctx->current_filename = saved_filename;
}
