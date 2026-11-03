// SPDX-License-Identifier: MIT

#include "../ast/ast.h"
#include "../constants.h"
#include "../parser/parser.h"
#include "../zprep.h"
#include "codegen.h"
#include "compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../platform/misra.h"
#include "codegen_internal.h"

// Emit includes and type aliases (and top-level comments)

int is_module_visited(VisitedModules *visited, const char *path)
{
    while (visited)
    {
        if (strcmp(visited->path, path) == 0)
        {
            return 1;
        }
        visited = visited->next;
    }
    return 0;
}

void mark_module_visited(VisitedModules **visited, const char *path)
{
    VisitedModules *node = xmalloc(sizeof(VisitedModules));
    node->path = path;
    node->next = *visited;
    *visited = node;
}

void free_visited_modules(VisitedModules *visited)
{
    // NO-OP: We use arena allocation (xmalloc) for VisitedModules.
    // Arena is reset globally, single nodes must not be zfree()d.
    (void)visited;
}

static void emit_includes_and_aliases_internal(ParserContext *ctx, ASTNode *node,
                                               VisitedModules **visited, int depth)
{
    if (depth > 1024)
    {
        zfatal("Infinite recursion detected in emit_includes_and_aliases (ctx, circular imports?)");
    }
    while (node)
    {
        if (node->kind == NODE_IMPORT)
        {
            if (!is_module_visited(*visited, node->import_stmt.path))
            {
                mark_module_visited(visited, node->import_stmt.path);
                emit_includes_and_aliases_internal(ctx, node->import_stmt.module_root, visited,
                                                   depth + 1);
            }
            node = node->next;
            continue;
        }
        if (node->kind == NODE_INCLUDE)
        {
            if (node->include.path)
            {
                if (node->include.is_system)
                {
                    EMIT(ctx, "#include <%s>\n", node->include.path);
                }
                else
                {
                    EMIT(ctx, "#include \"%s\"\n", node->include.path);
                }
            }
        }
        else if (node->kind == NODE_AST_COMMENT)
        {
            EMIT(ctx, "%s\n", node->comment.content);
        }
        node = node->next;
    }
}

void emit_includes_and_aliases(ParserContext *ctx, ASTNode *node, VisitedModules **visited)
{
    if (visited)
    {
        emit_includes_and_aliases_internal(ctx, node, visited, 0);
    }
    else
    {
        VisitedModules *local_visited = NULL;
        emit_includes_and_aliases_internal(ctx, node, &local_visited, 0);
    }
}

// Emit type aliases (after struct defs so the aliased types exist)
// `typedef <type> <alias>;`. A function pointer type (`int (*)(int)`) takes the alias name
// inside it.
static void emit_alias_typedef(ParserContext *ctx, Type *type, const char *alias)
{
    const char *c_type = c_type_spelling(ctx, type);
    const char *ptr = strstr(c_type, "(*)");
    if (ptr)
    {
        EMIT(ctx, "typedef %.*s (*%s)%s;\n", (int)(ptr - c_type), c_type, alias, ptr + 3);
    }
    else
    {
        EMIT(ctx, "typedef %s %s;\n", c_type, alias);
    }
}

static void emit_type_aliases_internal(ParserContext *ctx, ASTNode *node, VisitedModules **visited,
                                       int depth)
{
    if (depth > 1024)
    {
        zfatal("Infinite recursion detected in emit_type_aliases (ctx, circular imports?)");
    }
    while (node)
    {
        if (node->kind == NODE_IMPORT)
        {
            if (!is_module_visited(*visited, node->import_stmt.path))
            {
                mark_module_visited(visited, node->import_stmt.path);
                emit_type_aliases_internal(ctx, node->import_stmt.module_root, visited, depth + 1);
            }
        }
        else if (node->kind == NODE_TYPE_ALIAS)
        {
            if (node->cfg_condition)
            {
                EMIT(ctx, "#if %s\n", node->cfg_condition);
            }
            emit_alias_typedef(ctx, node->type_info, node->type_alias.alias);
            if (node->cfg_condition)
            {
                EMIT(ctx, "#endif\n");
            }
        }
        node = node->next;
    }
}

void emit_type_aliases(ParserContext *ctx, ASTNode *node, VisitedModules **visited)
{
    if (visited)
    {
        emit_type_aliases_internal(ctx, node, visited, 0);
    }
    else
    {
        VisitedModules *local_visited = NULL;
        emit_type_aliases_internal(ctx, node, &local_visited, 0);
    }
}

void emit_global_aliases(ParserContext *ctx)
{
    TypeAlias *ta = ctx->type_aliases;
    while (ta)
    {
        emit_alias_typedef(ctx, ta->type_info, ta->alias);
        ta = ta->next;
    }
}

// Emit enum constructor prototypes
void emit_enum_protos(ParserContext *ctx, ASTNode *node)
{
    while (node)
    {
        if (node->kind == NODE_ENUM && !node->enm.is_template)
        {
            // Only emit prototypes for ADT-style enums (with payloads)
            int has_payload = 0;
            ASTNode *v_ptr = node->enm.variants;
            while (v_ptr)
            {
                if (v_ptr->variant.payload)
                {
                    has_payload = 1;
                    break;
                }
                v_ptr = v_ptr->next;
            }

            if (has_payload)
            {
                const char *final_name = node->link_name ? node->link_name : node->enm.name;
                if (node->cfg_condition)
                {
                    EMIT(ctx, "#if %s\n", node->cfg_condition);
                }
                ASTNode *v = node->enm.variants;
                while (v)
                {
                    if (v->variant.payload)
                    {
                        Type *pt = v->variant.payload;
                        if (pt->kind == TYPE_TUPLE)
                        {
                            EMIT(ctx, "%s %s(", final_name,
                                 mangle_variant_symbol(final_name, v->variant.name));
                            for (int i = 0; i < pt->count; i++)
                            {
                                EMIT(ctx, "%s _%d%s", c_type_spelling(ctx, pt->args[i]), i,
                                     i < pt->count - 1 ? ", " : "");
                            }
                            EMIT(ctx, ");\n");
                        }
                        else
                        {
                            char *tstr = c_type_spelling(ctx, v->variant.payload);
                            EMIT(ctx, "%s %s(%s v);\n", final_name,
                                 mangle_variant_symbol(final_name, v->variant.name), tstr);
                            zfree(tstr);
                        }
                    }
                    else
                    {
                        EMIT(ctx, "%s %s();\n", final_name,
                             mangle_variant_symbol(final_name, v->variant.name));
                    }
                    v = v->next;
                }
                if (node->cfg_condition)
                {
                    EMIT(ctx, "#endif\n");
                }
            }
        }
        node = node->next;
    }
}

// Emit lambda definitions.
// Collect the set of C type strings produced by `return <expr>;` in a lambda body.
// Emits one field of a closure context struct. A function pointer type needs the
// field name inside its declarator (`int32_t (*f)(int32_t)`), which
// emit_var_decl_type does.
static void emit_lambda_ctx_field(ParserContext *ctx, const char *tstr, const char *name,
                                  int by_ref)
{
    // Function pointers and arrays put the name inside the type: `int (*name)(int)`,
    // `int name[3]`. By reference, a function pointer gets one more `*` and an array becomes
    // a pointer to it, `int (*name)[3]` (`int *name[3]` would be an array of pointers).
    int is_fn_ptr = strstr(tstr, "(*") != NULL;
    if (is_fn_ptr || strchr(tstr, '['))
    {
        size_t decl_sz = strlen(name) + 4;
        char *decl_name = xmalloc(decl_sz);
        if (!by_ref)
        {
            snprintf(decl_name, decl_sz, "%s", name);
        }
        else
        {
            snprintf(decl_name, decl_sz, is_fn_ptr ? "*%s" : "(*%s)", name);
        }
        emit_var_decl_type(ctx, tstr, decl_name);
        EMIT(ctx, ";\n");
        zfree(decl_name);
        return;
    }
    EMIT(ctx, by_ref ? "%s* %s;\n" : "%s %s;\n", tstr, name);
}

void emit_lambda_defs(ParserContext *ctx)
{
    LambdaRef *cur = ctx->global_lambdas;
    while (cur)
    {
        ASTNode *node = cur->node;
        int saved_defer = ctx->cg.defer_count;
        ctx->cg.defer_count = 0;

        if (node->lambda.capture_count > 0)
        {
            EMIT(ctx, "struct Lambda_%d_Ctx {\n", node->lambda.lambda_id);
            emitter_indent(&ctx->cg.emitter);
            for (int i = 0; i < node->lambda.capture_count; i++)
            {
                int by_ref = node->lambda.capture_modes && node->lambda.capture_modes[i] == 1;
                emit_lambda_ctx_field(ctx, capture_c_type(ctx, node, i),
                                      node->lambda.captured_vars[i], by_ref);
                if (!by_ref)
                {
                    ASTNode *fdef = capture_struct_def(ctx, node, i);
                    if (fdef && fdef->type_info && fdef->type_info->traits.has_drop)
                    {
                        EMIT(ctx, "int __z_drop_flag_%s;\n", node->lambda.captured_vars[i]);
                    }
                }
            }
            emitter_dedent(&ctx->cg.emitter);
            EMIT(ctx, "};\n\n");

            // Generate Drop function for the closure context
            EMIT(ctx, "static void _lambda_%d_drop(void* _ctx) {\n", node->lambda.lambda_id);
            emitter_indent(&ctx->cg.emitter);
            EMIT(ctx, "struct Lambda_%d_Ctx* ctx = (struct Lambda_%d_Ctx*)_ctx;\n",
                 node->lambda.lambda_id, node->lambda.lambda_id);

            for (int i = 0; i < node->lambda.capture_count; i++)
            {
                if (node->lambda.capture_modes && node->lambda.capture_modes[i] == 0)
                {
                    ASTNode *fdef = capture_struct_def(ctx, node, i);
                    if (fdef && fdef->type_info && fdef->type_info->traits.has_drop)
                    {
                        EMIT(ctx, "if (ctx->__z_drop_flag_%s) %s__Drop__glue(&ctx->%s);\n",
                             node->lambda.captured_vars[i],
                             type_def_name(node->lambda.captured_types_info[i]),
                             node->lambda.captured_vars[i]);
                    }
                }
            }

            EMIT(ctx, "free(_ctx);\n");
            emitter_dedent(&ctx->cg.emitter);
            EMIT(ctx, "}\n\n");
        }

        // A return type nothing told is `void*`.
        Type *ret = node->type_info->inner;
        const char *ret_c_type =
            (ret && ret->kind != TYPE_UNKNOWN) ? c_type_spelling(ctx, ret) : "void*";
        EMIT(ctx, "%s _lambda_%d(", ret_c_type, node->lambda.lambda_id);

        if (!node->lambda.is_bare)
        {
            EMIT(ctx, "void* _ctx");
        }

        for (int i = 0; i < node->lambda.count; i++)
        {
            if (!node->lambda.is_bare || i > 0)
            {
                EMIT(ctx, ", ");
            }

            // A parameter whose type nothing told is passed as `void*`.
            Type *param = node->type_info->args[i];
            const char *param_c_type =
                (param && param->kind != TYPE_UNKNOWN) ? c_type_spelling(ctx, param) : "void*";
            EMIT(ctx, "%s %s", param_c_type, node->lambda.param_names[i]);
        }
        EMIT(ctx, ") {\n");
        emitter_indent(&ctx->cg.emitter);

        if (node->lambda.capture_count > 0)
        {
            EMIT(ctx, "struct Lambda_%d_Ctx* ctx = (struct Lambda_%d_Ctx*)_ctx;\n",
                 node->lambda.lambda_id, node->lambda.lambda_id);
        }

        ctx->cg.current_lambda = node;
        if (node->lambda.body && node->lambda.body->kind == NODE_BLOCK)
        {
            if (node->lambda.is_expression && node->type_info->inner &&
                node->type_info->inner->kind != TYPE_VOID)
            {
                ASTNode *stmt = node->lambda.body->block.statements;
                while (stmt)
                {
                    if (stmt->next == NULL)
                    {
                        if (stmt->kind != NODE_RETURN)
                        {
                            EMIT(ctx, "return ");
                        }
                        codegen_node_single(ctx, stmt);
                    }
                    else
                    {
                        codegen_node_single(ctx, stmt);
                    }
                    stmt = stmt->next;
                }
            }
            else
            {
                codegen_walker(ctx, node->lambda.body->block.statements);
            }
        }
        else if (node->lambda.body)
        {
            if (node->type_info->inner && node->type_info->inner->kind != TYPE_VOID &&
                node->lambda.body->kind != NODE_RETURN)
            {
                EMIT(ctx, "return ");
            }
            codegen_node_single(ctx, node->lambda.body);
            EMIT(ctx, ";\n");
        }
        ctx->cg.current_lambda = NULL;

        emit_defers_down_to(ctx, 0);

        emitter_dedent(&ctx->cg.emitter);
        EMIT(ctx, "}\n\n");

        ctx->cg.defer_count = saved_defer;
        cur = cur->next;
    }
}
