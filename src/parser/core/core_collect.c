// SPDX-License-Identifier: MIT
// collect: registers each declaration where later code looks it up, from its node.
#include "parser.h"
#include "ast/ast.h"

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
