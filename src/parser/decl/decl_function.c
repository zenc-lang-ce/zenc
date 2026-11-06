// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "ast/ast.h"
#include "analysis/move_check.h"
#include "plugins/plugin_manager.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analysis/const_fold.h"
#include "utils/utils.h"
#include "ast/primitives.h"

// Appends a body to the end of ctx's pending list (parsed in declaration order); a template's
// body to the list of template bodies, which are parsed first.
void push_pending_body(ParserContext *ctx, PendingBody *pending)
{
    PendingBody **head =
        pending->is_template ? &ctx->pending_template_bodies : &ctx->pending_bodies;
    PendingBody **tail =
        pending->is_template ? &ctx->pending_template_bodies_tail : &ctx->pending_bodies_tail;
    if (*tail)
    {
        (*tail)->next = pending;
    }
    else
    {
        *head = pending;
    }
    *tail = pending;
}

// The first body waiting in `head`, taken out of the list.
static PendingBody *take_pending_body(PendingBody **head, PendingBody **tail)
{
    PendingBody *pending = *head;
    *head = pending->next;
    if (!*head)
    {
        *tail = NULL;
    }
    return pending;
}

// Warns about the parameters in `scope` that the function body never used. `func_name` is
// NULL for a lambda.
void check_unused_parameters(ParserContext *ctx, Scope *scope, const char *func_name)
{
    ZenSymbol *sym = scope->symbols;
    while (sym)
    {
        // Check if unused and not prefixed with `_`
        // also ignore `self` as it's often mandated by traits
        if (!sym->is_used && !ctx->config->misra_mode && sym->name[0] != '_' &&
            strcmp(sym->name, "self") != 0 && (!func_name || strcmp(func_name, "main") != 0))
        {
            warn_unused_parameter(sym->decl_token, sym->name, func_name);
        }
        sym = sym->next;
    }
}

// An impl block on a generic struct or enum is a template even when written
// without `<T>` (`impl Drop for Vec`): instantiation copies its methods.
static int in_generic_impl(ParserContext *ctx)
{
    if (!ctx->current_impl_struct)
    {
        return 0;
    }
    ASTNode *def = find_struct_def(ctx, ctx->current_impl_struct);
    return def && ((def->kind == NODE_STRUCT && def->strct.is_template) ||
                   (def->kind == NODE_ENUM && def->enm.is_template));
}

ASTNode *parse_function(ParserContext *ctx, Lexer *l, int is_extern, const char *link_name,
                        int is_export)
{
    lexer_next(l);
    Token name_tok = lexer_next(l);
    check_reserved_keyword(name_tok);
    // Methods are emitted with a prefix (`Type__default`) and extern functions
    // keep their C name, so only free functions may not be named like a C
    // keyword.
    if (!ctx->current_impl_struct && !is_extern)
    {
        check_c_identifier(name_tok);
    }
    char *name = token_strdup(name_tok);

    char *gen_param = NULL;
    if (lexer_peek(l).kind == TOK_LANGLE)
    {
        lexer_next(l);

        size_t buf_size = 1024;
        char *buf = xmalloc(buf_size);
        buf[0] = 0;

        while (1)
        {
            Token gt = lexer_next(l);
            if (gt.kind != TOK_IDENT)
            {
                zpanic_at(gt, "Expected generic parameter name");
                return NULL;
            }
            char *s = token_strdup(gt);

            if (strlen(buf) + strlen(s) + 2 >= buf_size)
            {
                buf_size *= 2;
                buf = xrealloc(buf, buf_size);
            }

            if (buf[0])
            {
                strcat(buf, ",");
            }
            strcat(buf, s);

            // Check for shadowing
            if (is_known_generic(ctx, s))
            {
                zpanic_at(gt, "Generic parameter '%s' shadows an existing generic parameter", s);
                return NULL;
            }

            zfree(s);

            if (lexer_peek(l).kind == TOK_COMMA)
            {
                lexer_next(l);
                continue;
            }
            break;
        }

        if (lexer_next(l).kind != TOK_RANGLE)
        {
            zpanic_at(lexer_peek(l), "Expected >");
            return NULL;
        }
        gen_param = xstrdup(buf);
        zfree(buf);
    }

    // Register generic parameters so type parsing recognizes them
    int saved_generic_count = ctx->known_generics_count;
    if (gen_param)
    {
        char *tmp = xstrdup(gen_param);
        char *tok = strtok(tmp, ",");
        while (tok)
        {
            register_generic(ctx, tok);
            tok = strtok(NULL, ",");
        }
        zfree(tmp);
    }

    enter_scope(ctx);
    ASTNode **default_values;
    int count;
    Type **arg_types;
    char **param_names;
    char **ctype_overrides;
    int is_varargs = 0;

    if (!parse_and_convert_args(ctx, l, &default_values, &count, &arg_types, &param_names,
                                &is_varargs, &ctype_overrides))
    {
        return NULL;
    }

    Type *ret_type_obj = type_new(TYPE_VOID);

    if (strcmp(name, "main") == 0)
    {
        ret_type_obj = type_new(TYPE_C_INT);
    }

    if (lexer_peek(l).kind == TOK_ARROW)
    {
        lexer_next(l);
        ret_type_obj = parse_type_formal(ctx, l);
        if (!ret_type_obj)
        {
            return NULL;
        }
    }
    else if (lexer_peek(l).kind == TOK_COLON)
    {
        zpanic_at(lexer_peek(l), "Functions use '->' for the return type, not ':'");
        return NULL;
    }

    curr_func_ret = ret_type_obj;

    // Auto-prefix function name if in module context
    // Don't prefix generic templates or functions inside impl blocks (already
    // mangled)
    if (ctx->imports.current_module_prefix && !gen_param && !ctx->current_impl_struct &&
        !is_extern && !is_extern_symbol(ctx, name))
    {
        char *prefixed_name = mangle_module_symbol(ctx->imports.current_module_prefix, name);
        zfree(name);
        name = prefixed_name;
    }

    // Register if concrete (Global functions only)
    if (!gen_param && !ctx->current_impl_struct)
    {
        register_func(ctx, ctx->current_scope->parent, name, count, default_values, arg_types,
                      ret_type_obj, is_varargs, 0, link_name, name_tok, is_export);
        // Note: required is set after return by caller (parser_core.c)
    }

    // Bodies are parsed once all declarations are known. A generic template's body (which
    // includes the methods of an impl on a generic type written without `<T>`) is parsed
    // before the concrete ones, with its generic parameters known again.
    int is_template = gen_param || ctx->known_generics_count > 0 || in_generic_impl(ctx);
    PendingBody *pending = NULL;

    Token next_tok = lexer_peek(l);
    if (next_tok.kind == TOK_SEMICOLON)
    {
        lexer_next(l); // consume ;
    }
    else if (next_tok.kind == TOK_LBRACE)
    {
        // Set self context flags for .member shorthand in methods with self
        int prev_in_method = ctx->in_method_with_self;
        if (count > 0 && param_names[0] && strcmp(param_names[0], "self") == 0)
        {
            ctx->in_method_with_self = 1;
        }

        pending = xcalloc(1, sizeof(PendingBody));
        pending->name = name;
        pending->body_start = *l;
        pending->scope = ctx->current_scope;
        pending->in_method_with_self = ctx->in_method_with_self;
        pending->is_template = is_template;
        if (is_template)
        {
            pending->generic_count = ctx->known_generics_count;
            pending->generics = xmalloc(sizeof(char *) * (size_t)(ctx->known_generics_count + 1));
            memcpy(pending->generics, ctx->known_generics,
                   sizeof(char *) * (size_t)ctx->known_generics_count);
        }

        // Restore previous state
        ctx->in_method_with_self = prev_in_method;

        if (!skip_braced_block(l))
        {
            return NULL;
        }
    }
    else
    {
        zpanic_at(next_tok, "Expected '{' or ';' after function signature");
        return NULL;
    }

    exit_scope(ctx);

    // Restore generic count to unregister function-scoped generics
    ctx->known_generics_count = saved_generic_count;

    curr_func_ret = NULL;

    ASTNode *node = ast_create(NODE_FUNCTION);
    node->token = name_tok; // Save definition location
    node->func.name = name;

    node->func.arg_types = arg_types;
    node->func.param_names = param_names;
    node->func.count = count;
    node->func.default_values = default_values;
    node->func.ret_type_info = ret_type_obj;
    node->func.is_varargs = is_varargs;
    node->func.is_extern = is_extern;
    node->func.c_type_overrides = ctype_overrides;
    node->link_name = link_name ? xstrdup(link_name) : NULL;
    node->func.module_prefix = ctx->imports.current_module_prefix;
    node->func.impl_struct = ctx->current_impl_struct;
    node->func.filename = ctx->current_filename;

    if (pending)
    {
        pending->node = node;
        push_pending_body(ctx, pending);
    }

    if (gen_param)
    {
        node->func.generic_params = xstrdup(gen_param);
        if (!ctx->current_impl_struct)
        {
            register_func_template(ctx, name, gen_param, node);
            return NULL;
        }
    }
    if (!ctx->current_impl_struct)
    {
        add_to_func_list(ctx, node);
    }
    return node;
}

// The methods of the generic impl that declares `method`. They are templates, not registered
// functions, so a method's body finds its sibling methods in this list.
static ASTNode *template_impl_methods(ParserContext *ctx, ASTNode *method)
{
    for (GenericImplTemplate *it = ctx->impl_templates; it; it = it->next)
    {
        ASTNode *impl = it->impl_node;
        ASTNode *methods = impl->kind == NODE_IMPL ? impl->impl.methods : impl->impl_trait.methods;
        for (ASTNode *m = methods; m; m = m->next)
        {
            if (m == method)
            {
                return methods;
            }
        }
    }
    return NULL;
}

/**
 * @brief Parses the bodies that parse_function deferred, in declaration order: the generic
 * templates' first, so the instances copied while declarations were read get a copy of them.
 */
void parse_pending_bodies(ParserContext *ctx)
{
    Scope *saved_scope = ctx->current_scope;
    char *saved_prefix = ctx->imports.current_module_prefix;
    char *saved_impl = ctx->current_impl_struct;
    ASTNode *saved_impl_methods = ctx->current_impl_methods;
    const char *saved_filename = ctx->current_filename;
    int saved_in_method = ctx->in_method_with_self;
    Type *saved_ret = curr_func_ret;
    char *saved_doc = ctx->last_doc_comment;
    int saved_generic_count = ctx->known_generics_count;
    char *saved_generics[MAX_KNOWN_GENERICS];
    if (saved_generic_count > 0)
    {
        memcpy(saved_generics, ctx->known_generics, sizeof(char *) * (size_t)saved_generic_count);
    }

    // We increment parse depth so a parse_program_nodes inside a body does not start another round.
    ctx->parse_depth++;

    while (1)
    {
        // Template bodies first, then the instances copied before them get theirs, then the
        // concrete bodies. Each is taken out before it is processed: parsing may add more.
        PendingBody *pending = NULL;
        if (ctx->pending_template_bodies)
        {
            pending = take_pending_body(&ctx->pending_template_bodies,
                                        &ctx->pending_template_bodies_tail);
        }
        else if (ctx->body_copies)
        {
            copy_instance_bodies(ctx);
            continue;
        }
        else if (ctx->pending_bodies)
        {
            pending = take_pending_body(&ctx->pending_bodies, &ctx->pending_bodies_tail);
        }
        else
        {
            break;
        }

        if (pending->generic_count > 0)
        {
            memcpy(ctx->known_generics, pending->generics,
                   sizeof(char *) * (size_t)pending->generic_count);
        }
        ctx->known_generics_count = pending->generic_count;

        ASTNode *node = pending->node;
        ctx->current_scope = pending->scope;
        // A concrete method finds the others registered by now; a template method in its impl.
        ctx->current_impl_methods = pending->is_template && node->kind == NODE_FUNCTION
                                        ? template_impl_methods(ctx, node)
                                        : NULL;
        ctx->in_method_with_self = pending->in_method_with_self;
        ctx->had_error = 0;

        if (node->kind == NODE_TEST)
        {
            ctx->imports.current_module_prefix = node->test_stmt.module_prefix;
            ctx->current_impl_struct = NULL;
            ctx->current_filename = node->test_stmt.filename;
            curr_func_ret = NULL;
        }
        else
        {
            ctx->imports.current_module_prefix = node->func.module_prefix;
            ctx->current_impl_struct = node->func.impl_struct;
            ctx->current_filename = node->func.filename;
            curr_func_ret = node->func.ret_type_info;
        }

        Lexer body_lexer = pending->body_start;
        ASTNode *body = parse_block(ctx, &body_lexer);
        if (node->kind == NODE_TEST)
        {
            node->test_stmt.body = body;
        }
        else
        {
            node->func.body = body;
            if (body)
            {
                check_unused_parameters(ctx, pending->scope, pending->name);
            }
        }

        ctx->last_doc_comment = NULL;
    }

    // Restore context state
    if (saved_generic_count > 0)
    {
        memcpy(ctx->known_generics, saved_generics, sizeof(char *) * (size_t)saved_generic_count);
    }
    ctx->known_generics_count = saved_generic_count;
    ctx->parse_depth--;
    ctx->current_scope = saved_scope;
    ctx->imports.current_module_prefix = saved_prefix;
    ctx->current_impl_struct = saved_impl;
    ctx->current_impl_methods = saved_impl_methods;
    ctx->current_filename = saved_filename;
    ctx->in_method_with_self = saved_in_method;
    curr_func_ret = saved_ret;
    ctx->last_doc_comment = saved_doc;
}
