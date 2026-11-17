// SPDX-License-Identifier: MIT
// The checks of a program's declarations run after it is parsed (analyze_program): names that
// name no type, and functions defined twice.
#include "declaration_checks.h"
#include "plugins/plugin_manager.h"
#include "parser.h"
#include "utils/utils.h"
#include "constants.h"

void warn_unknown_type(ParserContext *ctx, const TypeUsage *use)
{
    char msg[MAX_SHORT_MSG_LEN];
    snprintf(msg, sizeof(msg), "Unknown type '%s' (assuming external C struct)", use->name);
    const char *hint = get_closest_type_hint(ctx, use->name);
    if (hint)
    {
        char help[MAX_MANGLED_NAME_LEN];
        snprintf(help, sizeof(help), "Did you mean '%s'?", hint);
        zwarn_with_suggestion(use->location, msg, help);
    }
    else
    {
        zwarn_at(use->location, "%s", msg);
    }
}

int validate_types(ParserContext *ctx)
{
    if (ctx->config->mode_doc)
    {
        return 1;
    }
    for (TypeUsage *u = ctx->pending_type_validations; u; u = u->next)
    {
        if (!is_known_type_name(ctx, u->location.file, u->name))
        {
            warn_unknown_type(ctx, u);
        }
    }
    return 1;
}

typedef struct
{
    const char **names;
    int count;
    int cap;
} DefinedFunctions;

// Adds a defined function name; returns 0 if it was already defined.
static int defined_functions_add(DefinedFunctions *set, const char *name)
{
    for (const char **n = set->names; n < set->names + set->count; n++)
    {
        if (strcmp(*n, name) == 0)
        {
            return 0;
        }
    }
    if (set->count == set->cap)
    {
        set->cap = set->cap ? set->cap * 2 : 64;
        set->names = xrealloc(set->names, sizeof(const char *) * (size_t)set->cap);
    }
    *(set->names + set->count) = name;
    set->count++;
    return 1;
}

// A function with a body, emitted unconditionally under its C name.
static void check_function_definition(ASTNode *f, const char *owner, DefinedFunctions *set,
                                      int *errors)
{
    if (f->kind != NODE_FUNCTION || !f->func.body || !function_c_name(f) ||
        f->func.generic_param_count > 0 || f->cfg_condition)
    {
        return;
    }
    if (defined_functions_add(set, function_c_name(f)))
    {
        return;
    }

    if (owner && f->func.is_method)
    {
        zerror_at(f->token, "Redefinition of method '%s::%s'", owner, f->func.name);
    }
    else
    {
        zerror_at(f->token, "Redefinition of function '%s'", f->func.name);
    }
    (*errors)++;
}

static void check_redefinitions_in(ASTNode *node, DefinedFunctions *set, int *errors)
{
    // Definitions between `#if` and `#endif` may be alternatives: not checked.
    int preproc_depth = 0;
    for (; node; node = node->next)
    {
        if (node->kind == NODE_PREPROC_DIRECTIVE && node->raw_stmt.content)
        {
            const char *d = node->raw_stmt.content;
            while (*d == '#' || *d == ' ' || *d == '\t')
            {
                d++;
            }
            if (strncmp(d, "if", 2) == 0)
            {
                preproc_depth++;
            }
            else if (strncmp(d, "endif", 5) == 0 && preproc_depth > 0)
            {
                preproc_depth--;
            }
            continue;
        }
        if (node->kind == NODE_IMPORT)
        {
            check_redefinitions_in(node->import_stmt.module_root, set, errors);
            continue;
        }
        if (preproc_depth > 0 || node->cfg_condition)
        {
            continue;
        }
        if (node->kind == NODE_FUNCTION)
        {
            check_function_definition(node, NULL, set, errors);
        }
        else if (node->kind == NODE_IMPL)
        {
            for (ASTNode *m = node->impl.methods; m; m = m->next)
            {
                check_function_definition(m, type_impl_name(node->impl.target), set, errors);
            }
        }
        else if (node->kind == NODE_IMPL_TRAIT)
        {
            for (ASTNode *m = node->impl_trait.methods; m; m = m->next)
            {
                check_function_definition(m, NULL, set, errors);
            }
        }
    }
}

int validate_function_definitions(ParserContext *ctx, ASTNode *root)
{
    // MISRA mode reports duplicate identifiers itself (rules 5.8 and 5.9).
    if (ctx->config->mode_lsp || ctx->config->misra_mode || !root || root->kind != NODE_ROOT)
    {
        return 1;
    }
    DefinedFunctions set = {0};
    int errors = 0;
    check_redefinitions_in(root->root.children, &set, &errors);
    return errors == 0;
}
