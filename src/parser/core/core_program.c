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

// The attributes of a top-level `let` go to every variable it declares
// (`static let a = 1, b = 2;`), not only to the first one.
static void apply_global_attributes(ASTNode *decls, int is_static,
                                    const DeclarationAttributes *attrs)
{
    ASTNode *d = decls;
    while (d)
    {
        if (d->kind == NODE_VAR_DECL)
        {
            d->var_decl.is_static = is_static;
            d->var_decl.is_thread_local = attrs->is_thread_local;
            d->cfg_condition = attrs->cfg_condition;
        }
        d = d->next;
    }
}

void push_pending_declaration(ParserContext *ctx, PendingDeclaration *pending)
{
    if (ctx->pending_declarations_tail)
    {
        ctx->pending_declarations_tail->next = pending;
    }
    else
    {
        ctx->pending_declarations = pending;
    }
    ctx->pending_declarations_tail = pending;
}

// Makes what the struct or enum `decl` derives, after it among the declarations.
static void add_derived_impls(ParserContext *ctx, ASTNode *decl, char **traits, int count)
{
    ASTNode *impls = generate_derive_impls(ctx, decl, traits, count);
    if (!impls)
    {
        return;
    }
    ASTNode *last = impls;
    while (last->next)
    {
        last = last->next;
    }
    last->next = decl->next;
    decl->next = impls;
}

// The `@derive` of the struct or enum `decl`: it reads the fields and the impls of their types,
// so while declarations are read it waits for them (parse_pending_declarations).
static void derive(ParserContext *ctx, ASTNode *decl, DeclarationAttributes *attrs)
{
    if (attrs->derived_count == 0)
    {
        return;
    }
    if (ctx->declarations_read)
    {
        add_derived_impls(ctx, decl, attrs->derived_traits, attrs->derived_count);
        return;
    }
    PendingDeclaration *pending = xcalloc(1, sizeof(PendingDeclaration));
    pending->derived = decl;
    pending->derived_traits = attrs->derived_traits;
    pending->derived_count = attrs->derived_count;
    pending->module_prefix = ctx->imports.current_module_prefix;
    pending->filename = ctx->current_filename;
    pending->scope = ctx->current_scope;
    push_pending_declaration(ctx, pending);
}

int derives_pending(ParserContext *ctx, const char *type_name, const char *trait)
{
    for (PendingDeclaration *p = ctx->pending_declarations; p; p = p->next)
    {
        if (p->is_impl || !declared_type_c_name(p->derived) ||
            strcmp(declared_type_c_name(p->derived), type_name) != 0)
        {
            continue;
        }
        for (int i = 0; i < p->derived_count; i++)
        {
            if (strcmp(p->derived_traits[i], trait) == 0)
            {
                return 1;
            }
        }
    }
    return 0;
}

void parse_pending_declarations(ParserContext *ctx)
{
    char *saved_prefix = ctx->imports.current_module_prefix;
    const char *saved_filename = ctx->current_filename;
    Scope *saved_scope = ctx->current_scope;
    // A parse_program_nodes inside one (an import) does not start another round.
    ctx->parse_depth++;
    // The impls first, then the derives, which read them (a field's `clone`); each in the order
    // written. A derive still waiting is what its type will have (derives_pending).
    for (int impls = 1; impls >= 0; impls--)
    {
        PendingDeclaration **link = &ctx->pending_declarations;
        while (*link)
        {
            PendingDeclaration *pending = *link;
            if (pending->is_impl != impls)
            {
                link = &pending->next;
                continue;
            }
            *link = pending->next;
            ctx->imports.current_module_prefix = pending->module_prefix;
            ctx->current_filename = pending->filename;
            ctx->current_scope = pending->scope;
            ctx->had_error = 0;
            if (pending->is_impl)
            {
                Lexer l = pending->start;
                ctx->impl_slot = pending->impl;
                parse_impl(ctx, &l);
                ctx->impl_slot = NULL;
            }
            else
            {
                add_derived_impls(ctx, pending->derived, pending->derived_traits,
                                  pending->derived_count);
            }
        }
    }
    ctx->pending_declarations_tail = NULL;
    ctx->parse_depth--;
    ctx->imports.current_module_prefix = saved_prefix;
    ctx->current_filename = saved_filename;
    ctx->current_scope = saved_scope;
}

static ASTNode *parse_top_level_nodes(ParserContext *ctx, Lexer *l)
{
    ASTNode *h = 0, *tl = 0;
    while (1)
    {
        skip_comments(l);

        // Fault-tolerant recovery: if a sub-parser encountered an error and
        // returned (instead of exit()ing), skip tokens until we find a likely
        // top-level keyword so we can resume parsing cleanly.
        if (ctx->is_fault_tolerant && ctx->had_error)
        {
            ctx->had_error = 0;
            while (1)
            {
                Token recovery = lexer_peek(l);
                if (recovery.kind == TOK_EOF)
                {
                    return h;
                }
                // Resync on tokens that typically start top-level declarations
                if (recovery.kind == TOK_IDENT)
                {
                    if ((recovery.len == 2 && strncmp(recovery.start, "fn", 2) == 0) ||
                        (recovery.len == 6 && strncmp(recovery.start, "struct", 6) == 0) ||
                        (recovery.len == 4 && strncmp(recovery.start, "enum", 4) == 0) ||
                        (recovery.len == 4 && strncmp(recovery.start, "impl", 4) == 0) ||
                        (recovery.len == 5 && strncmp(recovery.start, "trait", 5) == 0) ||
                        (recovery.len == 6 && strncmp(recovery.start, "import", 6) == 0) ||
                        (recovery.len == 6 && strncmp(recovery.start, "extern", 6) == 0) ||
                        (recovery.len == 4 && strncmp(recovery.start, "test", 4) == 0) ||
                        (recovery.len == 5 && strncmp(recovery.start, "alias", 5) == 0))
                    {
                        break; // Found a top-level keyword, resume parsing
                    }
                }
                lexer_next(l); // Skip this token
            }
            continue; // Re-enter loop with peeked top-level keyword
        }

        skip_comments(l);
        Token t = lexer_peek(l);
        if (t.kind == TOK_EOF)
        {
            break;
        }

        if (t.kind == TOK_COMPTIME)
        {
            ASTNode *body = parse_comptime_body(ctx, l);
            ASTNode *ct = ast_create(NODE_COMPTIME);
            ct->comptime.body = body;
            ct->comptime.generated = NULL;
            if (!h)
            {
                h = ct;
            }
            else
            {
                tl->next = ct;
            }
            tl = ct;
            continue;
        }

        ASTNode *s = 0;
        DeclarationAttributes attrs = parse_attributes(ctx, l);
        t = lexer_peek(l);

        // Standalone @link("path") directive
        if (attrs.link_path)
        {
            // Push to config.c_files immediately (module AST may be freed later)
            const char *path = attrs.link_path;
            char *resolved = realpath(path, NULL);
            if (!resolved && ctx->config->root_path)
            {
                // The path may be relative to the workspace/stdlib root
                // (ZC_ROOT) rather than the current working directory.
                char alt[MAX_PATH_LEN];
                snprintf(alt, sizeof(alt), "%s/%s", ctx->config->root_path, path);
                resolved = realpath(alt, NULL);
            }
            if (!resolved && ctx->config->std_root[0])
            {
                // Installed compilers resolve the std root from an actual std
                // import; @link paths inside std (e.g. the vendored TRE unity
                // build) are relative to that root.
                char alt[MAX_PATH_LEN];
                snprintf(alt, sizeof(alt), "%s/%s", ctx->config->std_root, path);
                resolved = realpath(alt, NULL);
            }
            zvec_push_Str(&ctx->config->c_files, xstrdup(resolved ? resolved : path));
            if (resolved)
            {
                libc_free(resolved);
            }

            s = ast_create(NODE_LINK);
            s->link.path = attrs.link_path;
            attrs.link_path = NULL;
            goto add_node;
        }

        if (t.kind == TOK_PREPROC)
        {
            lexer_next(l);
            char *content = xmalloc(t.len + 2);
            strncpy(content, t.start, t.len);
            content[t.len] = '\n';
            content[t.len + 1] = 0;
            s = ast_create(NODE_PREPROC_DIRECTIVE);
            s->token = t;
            s->raw_stmt.content = content;

            // Audit and potentially deprecate preprocessor directives
            parser_audit_preprocessor(ctx, t);
        }
        else if (t.kind == TOK_DEF)
        {
            s = parse_def(ctx, l, attrs.is_export);
        }
        else if (t.kind == TOK_IDENT)
        {
            // Inline function: inline fn name(...) { }
            if (0 == strncmp(t.start, "inline", 6) && 6 == t.len)
            {
                lexer_next(l);
                Token next = lexer_peek(l);
                if (next.kind == TOK_IDENT && 2 == next.len && 0 == strncmp(next.start, "fn", 2))
                {
                    s = parse_function(ctx, l, 0, attrs.link_name, attrs.is_export);
                    attrs.is_inline = 1;
                }
                else
                {
                    zpanic_at(next, "Expected 'fn' after 'inline'");
                    return NULL;
                }
            }
            else if (0 == strncmp(t.start, "async", 5) && 5 == t.len)
            {
                lexer_next(l);
                Token next = lexer_peek(l);
                zerror_at(t, "'async' was removed; call the function directly or use "
                             "std/thread.zc");
                if (next.kind == TOK_IDENT && 2 == next.len && 0 == strncmp(next.start, "fn", 2))
                {
                    s = parse_function(ctx, l, 0, attrs.link_name, attrs.is_export);
                }
                else
                {
                    zpanic_at(next, "Expected 'fn' after 'async'");
                    return NULL;
                }
            }
            else if (0 == strncmp(t.start, "fn", 2) && 2 == t.len)
            {
                s = parse_function(ctx, l, 0, attrs.link_name, attrs.is_export);
            }
            else if (0 == strncmp(t.start, "struct", 6) && 6 == t.len)
            {
                s = parse_struct(ctx, l, 0, 0, 0, attrs.link_name, attrs.is_export);
                if (s && s->kind == NODE_STRUCT)
                {
                    s->strct.is_packed = attrs.is_packed;
                    s->strct.align = attrs.align;
                    s->strct.attributes = attrs.custom_attributes;
                    if (attrs.crepr_c_type)
                    {
                        s->strct.crepr_c_type = xstrdup(attrs.crepr_c_type);
                    }

                    if (attrs.vector_size > 0)
                    {
                        s->type_info->kind = TYPE_VECTOR;
                        s->type_info->array_size = attrs.vector_size;
                        s->type_info->inner = s->strct.fields ? s->strct.fields->type_info : NULL;
                    }
                }
            }
            else if (0 == strncmp(t.start, "enum", 4) && 4 == t.len)
            {
                s = parse_enum(ctx, l, attrs.link_name, attrs.is_export);
            }
            else if (t.len == 4 && strncmp(t.start, "impl", 4) == 0)
            {
                s = parse_impl(ctx, l);
            }
            else if (t.len == 5 && strncmp(t.start, "trait", 5) == 0)
            {
                s = parse_trait(ctx, l);
            }
            else if (t.len == 7 && strncmp(t.start, "include", 7) == 0)
            {
                s = parse_include(ctx, l);
            }
            else if (t.len == 6 && strncmp(t.start, "export", 6) == 0)
            {
                lexer_next(l); // eat 'export'
                Token next = lexer_peek(l);
                if (next.len == 6 && strncmp(next.start, "import", 6) == 0)
                {
                    s = parse_import(ctx, l, 1);
                }
            }
            else if (t.len == 6 && strncmp(t.start, "import", 6) == 0)
            {
                s = parse_import(ctx, l, 0);
            }
            else if (t.len == 6 && strncmp(t.start, "static", 6) == 0)
            {
                lexer_next(l); // eat static
                Token next = lexer_peek(l);
                if (next.kind == TOK_IDENT && next.len == 3 && strncmp(next.start, "let", 3) == 0)
                {
                    s = parse_var_decl(ctx, l, attrs.is_export);
                    apply_global_attributes(s, 1, &attrs);
                }
                else
                {
                    zpanic_at(next, "Expected 'let' after 'static' in global scope");
                    return NULL;
                }
            }
            else if (t.len == 3 && strncmp(t.start, "let", 3) == 0)
            {
                s = parse_var_decl(ctx, l, attrs.is_export);
                apply_global_attributes(s, 0, &attrs);
            }
            else if (t.len == 6 && strncmp(t.start, "extern", 6) == 0)
            {
                lexer_next(l);

                Token peek = lexer_peek(l);
                if (peek.kind == TOK_IDENT && peek.len == 2 && strncmp(peek.start, "fn", 2) == 0)
                {
                    s = parse_function(ctx, l, 1, attrs.link_name, attrs.is_export);
                }
                else if (peek.kind == TOK_IDENT && peek.len == 6 &&
                         strncmp(peek.start, "struct", 6) == 0)
                {
                    // extern struct Name; -> opaque struct declaration
                    s = parse_struct(ctx, l, 0, 1, 1, attrs.link_name, attrs.is_export);
                    if (s && s->kind == NODE_STRUCT)
                    {
                        register_extern_symbol(ctx, declared_type_c_name(s));
                    }
                }
                else if ((peek.kind == TOK_IDENT && peek.len == 5 &&
                          strncmp(peek.start, "union", 5) == 0) ||
                         peek.kind == TOK_UNION)
                {
                    // extern union Name; -> opaque union declaration
                    s = parse_struct(ctx, l, 1, 1, 1, attrs.link_name, attrs.is_export);
                    if (s && s->kind == NODE_STRUCT)
                    {
                        register_extern_symbol(ctx, declared_type_c_name(s));
                    }
                }
                else if (report_extern_with_kind(l))
                {
                    continue;
                }
                else
                {
                    if (lexer_peek(l).kind == TOK_IDENT && lexer_peek(l).len == 3 &&
                        strncmp(lexer_peek(l).start, "let", 3) == 0)
                    {
                        lexer_next(l); // eat let
                    }

                    while (1)
                    {
                        Token sym = lexer_next(l);
                        if (sym.kind != TOK_IDENT)
                        {
                            break;
                        }

                        char *name = token_strdup(sym);
                        register_extern_symbol(ctx, name);

                        // extern let NAME: T; -> a C global: typed for the
                        // front-end, nothing emitted (C already declares it).
                        if (lexer_peek(l).kind == TOK_COLON)
                        {
                            lexer_next(l);
                            Type *type_obj = parse_type_formal(ctx, l);
                            if (type_obj != NULL)
                            {
                                add_symbol_with_token(ctx, name, type_obj, sym, attrs.is_export);
                            }
                        }

                        Token next = lexer_peek(l);
                        if (next.kind == TOK_COMMA)
                        {
                            lexer_next(l);
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (lexer_peek(l).kind == TOK_SEMICOLON)
                    {
                        lexer_next(l);
                    }
                    else
                    {
                        // Fault-tolerant (LSP): the loop resyncs on the next declaration.
                        zpanic_at(lexer_peek(l), "Expected ';' after extern declaration");
                    }
                    continue;
                }
            }
            else if (0 == strncmp(t.start, "type", 4) && 4 == t.len)
            {
                s = parse_type_alias(ctx, l, 0, attrs.is_export);
            }
            else if (0 == strncmp(t.start, "raw", 3) && 3 == t.len)
            {
                lexer_next(l);
                if (lexer_peek(l).kind != TOK_LBRACE)
                {
                    zpanic_at(lexer_peek(l), "Expected { after raw");
                    return NULL;
                }
                lexer_next(l);

                char *content = parse_raw_block_body(ctx, l);
                if (!content)
                {
                    return NULL;
                }
                s = ast_create(NODE_RAW_STMT);
                s->token = t;
                s->raw_stmt.content = content;
            }
            else if (is_token(t, "const"))
            {
                report_toplevel_const(l, t);
                continue;
            }
            else
            {
                report_unexpected_toplevel(t);
                lexer_next(l); // fault-tolerant (LSP): resync on the next declaration
                continue;
            }
        }
        else if (t.kind == TOK_OPAQUE)
        {
            lexer_next(l); // eat opaque
            Token next = lexer_peek(l);
            if (0 == strncmp(next.start, "struct", 6) && 6 == next.len)
            {
                s = parse_struct(ctx, l, 0, 1, 0, attrs.link_name, attrs.is_export);
                if (s && s->kind == NODE_STRUCT)
                {
                    s->strct.is_packed = attrs.is_packed;
                    s->strct.align = attrs.align;
                }
            }
            else if (next.kind == TOK_ALIAS)
            {
                s = parse_type_alias(ctx, l, 1, attrs.is_export);
            }
            else
            {
                zpanic_at(next, "Expected 'struct' or 'alias' after 'opaque'");
                return NULL;
            }
        }
        else if (t.kind == TOK_ALIAS)
        {
            s = parse_type_alias(ctx, l, 0, attrs.is_export);
        }
        else if (t.kind == TOK_UNION)
        {
            s = parse_struct(ctx, l, 1, 0, 0, attrs.link_name, attrs.is_export);
        }
        else if (t.kind == TOK_TRAIT)
        {
            s = parse_trait(ctx, l);
        }
        else if (t.kind == TOK_IMPL)
        {
            s = parse_impl(ctx, l);
        }
        else if (t.kind == TOK_TEST)
        {
            s = parse_test(ctx, l);
        }
        else if (t.kind == TOK_SEMICOLON)
        {
            lexer_next(l); // a stray `;` between declarations is harmless
        }
        else
        {
            report_unexpected_toplevel(t);
            lexer_next(l); // fault-tolerant (LSP): resync on the next declaration
            continue;
        }

        if (s && s->kind == NODE_FUNCTION)
        {
            s->func.required = attrs.is_required;
            s->func.is_inline = attrs.is_inline || s->func.is_inline;
            s->func.noinline = attrs.is_noinline;
            s->func.constructor = attrs.is_constructor;
            s->func.destructor = attrs.is_destructor;
            s->func.unused = attrs.is_unused;
            s->func.weak = attrs.is_weak;
            s->func.is_export = attrs.is_export;
            s->func.cold = attrs.is_cold;
            s->func.hot = attrs.is_hot;
            s->func.noreturn = attrs.is_noreturn;
            s->func.pure = attrs.is_pure;
            s->func.section = attrs.section;
            s->func.is_comptime = attrs.is_comptime;
            s->func.cuda_global = attrs.cuda_global;
            s->func.cuda_device = attrs.cuda_device;
            s->func.cuda_host = attrs.cuda_host;
            s->func.attributes = attrs.custom_attributes;

            if (attrs.is_deprecated && function_c_name(s))
            {
                register_deprecated_func(ctx, function_c_name(s), attrs.deprecated_msg);
            }
        }
        else if (s && s->kind == NODE_STRUCT)
        {
            s->strct.is_export = attrs.is_export;
            s->strct.attributes = attrs.custom_attributes;
            s->strct.is_packed = attrs.is_packed || s->strct.is_packed;
            if (attrs.align)
            {
                s->strct.align = attrs.align;
            }
            if (attrs.is_deprecated && declared_type_c_name(s))
            {
                register_deprecated_func(ctx, declared_type_c_name(s), attrs.deprecated_msg);
            }
            derive(ctx, s, &attrs);
        }
        else if (s && s->kind == NODE_ENUM)
        {
            derive(ctx, s, &attrs);
        }

        if (s)
        {
        add_node:
            ATTACH_DOC_COMMENT(ctx, s);
            s->cfg_condition = attrs.cfg_condition;
            // A definition may have chosen its C name already (a reserved name, `class__zc`).
            if (attrs.link_name)
            {
                s->link_name = attrs.link_name;
            }

            for (ASTNode *declared = s; declared; declared = declared->next)
            {
                register_declaration(ctx, declared);
            }
            if (!h)
            {
                h = s;
            }
            else
            {
                tl->next = s;
            }
            tl = s;
            while (tl->next)
            {
                tl = tl->next;
            }
        }
        else
        {
            // Cleanup attributes if no node was created to receive them
            if (attrs.cfg_condition)
            {
                zfree(attrs.cfg_condition);
            }
            if (attrs.link_name)
            {
                zfree(attrs.link_name);
            }
            if (attrs.deprecated_msg)
            {
                zfree(attrs.deprecated_msg);
            }
            if (attrs.section)
            {
                zfree(attrs.section);
            }
            // ... potentially more cleanup needed for derived_traits and custom_attributes
        }
    }
    return h;
}

void parser_context_init(ParserContext *ctx, ZenCompiler *compiler, const char *filename)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->compiler = compiler;
    ctx->config = &compiler->config;
    ctx->current_filename = filename;
    module_state_init(&ctx->imports);
    diag_set_parser_ctx(ctx);
    token_set_parser_ctx(ctx);
}

ASTNode *parse_program_nodes(ParserContext *ctx, Lexer *l)
{
    ctx->parse_depth++;
    ASTNode *nodes = parse_top_level_nodes(ctx, l);
    ctx->parse_depth--;

    // Back at the outermost level: every declaration has been read, imports
    // included, so the deferred bodies can see all of them.
    if (ctx->parse_depth == 0)
    {
        ctx->declarations_read = 1;
        collect_pending_declarations(ctx);
        parse_pending_global_values(ctx);
        resolve_pending_constants(ctx);
        resolve_pending_type_names(ctx);
        resolve_waiting_structs(ctx);
        parse_pending_declarations(ctx);
        resolve_pending_instances(ctx);
        parse_pending_default_values(ctx);
        parse_pending_bodies(ctx);
        ctx->declarations_read = 0;
    }
    return nodes;
}

ASTNode *parse_program(ParserContext *ctx, Lexer *l)
{

    if (!ctx->global_scope)
    {
        ctx->global_scope = symbol_scope_create(NULL, "Global");
        ctx->current_scope = ctx->global_scope;
        register_builtins(ctx);
    }
    else
    {
        ctx->current_scope = ctx->global_scope;
    }

    // A parse the language server abandoned on an error may have left instances or bodies waiting.
    ctx->pending_instances = NULL;
    ctx->pending_instances_tail = NULL;
    ctx->declarations_read = 0;
    ctx->waiting_structs = NULL;
    ctx->pending_type_names = NULL;
    ctx->pending_constants = NULL;
    ctx->pending_global_values = NULL;
    ctx->pending_collections = NULL;
    ctx->pending_collections_tail = NULL;
    ctx->pending_declarations = NULL;
    ctx->pending_declarations_tail = NULL;
    ctx->impl_slot = NULL;
    ctx->pending_template_bodies = NULL;
    ctx->pending_template_bodies_tail = NULL;
    ctx->pending_default_values = NULL;
    ctx->pending_default_values_tail = NULL;
    ctx->body_copies = NULL;

    ASTNode *r = ast_create(NODE_ROOT);
    skip_comments(l);
    ATTACH_DOC_COMMENT(ctx, r);
    r->root.children = parse_program_nodes(ctx, l);

    return r;
}
