// SPDX-License-Identifier: MIT

#include "parser.h"
#include "constants.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ast/ast.h"
#include "plugins/plugin_manager.h"
#include "zen/zen_facts.h"
#include "zprep_plugin.h"
#include "analysis/move_check.h"

char *normalize_raw_content(const char *content)
{
    if (!content)
    {
        return NULL;
    }

    size_t len = strlen(content);
    char *normalized = xmalloc((size_t)(len + 1));
    char *d = normalized;
    const char *s = content;

    while (*s)
    {
        if (*s == '\r')
        {
            if (*(s + 1) == '\n')
            {
                *d++ = '\n';
                s += 2;
                continue;
            }
            *d++ = '\n';
            s++;
        }
        else
        {
            *d++ = *s++;
        }
    }
    *d = '\0';
    return normalized;
}

ASTNode *parse_macro_call(ParserContext *ctx, Lexer *l, char *macro_name)
{
    Token start_tok = lexer_peek(l);
    if (lexer_peek(l).kind != TOK_OP || lexer_peek(l).start[0] != '!')
    {
        return NULL;
    }
    lexer_next(l);

    if (lexer_peek(l).kind != TOK_LBRACE)
    {
        zpanic_at(lexer_peek(l), "Expected { after macro invocation");
        return NULL;
    }
    lexer_next(l);
    int start_line = l->line;
    int start_col = l->col;
    const char *body_start = l->src + l->pos;

    int depth = 1;
    while (depth > 0)
    {
        Token t = lexer_peek(l);
        if (t.kind == TOK_EOF)
        {
            zpanic_at(t, "Unexpected EOF in macro block");
            return NULL;
        }

        if (t.kind == TOK_LBRACE)
        {
            depth++;
        }
        if (t.kind == TOK_RBRACE)
        {
            depth--;
        }

        lexer_next(l);
    }
    int end_line = l->line;
    int end_col = l->col;

    const char *body_end = l->src + l->pos - 1;
    size_t body_len = (size_t)(body_end - body_start);
    char *body = xmalloc((size_t)(body_len + 1));
    memcpy(body, body_start, (size_t)(body_len));
    body[body_len] = '\0';

    const char *plugin_name = NULL;
#if ZC_HAS_PLUGINS
    plugin_name = resolve_plugin(ctx, macro_name);
#endif
    if (!plugin_name)
    {
        char err[MAX_SHORT_MSG_LEN];
        snprintf(err, sizeof(err), "Unknown plugin: %s (did you forget 'import plugin \"%s\"'?)",
                 macro_name, macro_name);
        zpanic_at(start_tok, "%s", err);
        return NULL;

        if (ctx->config->mode_lsp)
        {
            zfree(body);
            ASTNode *n = ast_create(NODE_PLUGIN);
            n->plugin_stmt.plugin_name = xstrdup(macro_name);
            n->plugin_stmt.body = xstrdup("");
            n->plugin_stmt.start_line = start_line;
            n->plugin_stmt.start_col = start_col;
            n->plugin_stmt.end_line = end_line;
            n->plugin_stmt.end_col = end_col;
            return n;
        }
    }

    ZPlugin *found = ctx->hook_find_plugin ? (ZPlugin *)ctx->hook_find_plugin(plugin_name) : NULL;

    if (!found)
    {
        char err[MAX_SHORT_MSG_LEN];
        snprintf(err, sizeof(err), "Plugin implementation not found: %s", plugin_name);
        zpanic_at(start_tok, "%s", err);
        return NULL;

        return NULL;
    }

    FILE *capture = tmpfile();
    if (!capture)
    {
        zpanic_at(start_tok, "Failed to create capture buffer for plugin expansion");
        return NULL;
    }

    ZApi api;
    if (ctx->hook_plugin_init_api)
    {
        ctx->hook_plugin_init_api(&api, ctx->current_filename, start_tok.line, ctx->config);
    }
    api.out = capture;
    api.hoist_out = ctx->cg.hoist_out;

    found->fn(body, &api);

    long len = ftell(capture);
    if (len < 0)
    {
        fclose(capture);
        return NULL;
    }
    if (len < 0)
    {
        fclose(capture);
        return NULL;
    }
    rewind(capture);
    char *expanded_code = xmalloc((size_t)(len + 1));
    size_t nread = fread(expanded_code, 1, (size_t)(len), capture);
    if (nread != (size_t)(len))
    {
        zfree(expanded_code);
        fclose(capture);
        return NULL;
    }
    expanded_code[len] = 0;
    fclose(capture);
    if (ctx->config->mode_lsp)
    {
        ASTNode *n = ast_create(NODE_PLUGIN);
        n->plugin_stmt.plugin_name = xstrdup(plugin_name);
        n->plugin_stmt.body = body;
        n->plugin_stmt.start_line = start_line;
        n->plugin_stmt.start_col = start_col;
        n->plugin_stmt.end_line = end_line;
        n->plugin_stmt.end_col = end_col;
        n->line = start_tok.line;
        n->token = start_tok;
        return n;
    }

    zfree(body);

    ASTNode *n = ast_create(NODE_RAW_STMT);
    n->token = start_tok;
    n->line = start_tok.line;
    n->raw_stmt.content = expanded_code;

    return n;
}

ASTNode *parse_comptime_body(ParserContext *ctx, Lexer *l)
{
    z_parse_expect(l, TOK_COMPTIME, "comptime");
    z_parse_expect(l, TOK_LBRACE, "expected { after comptime");

    const char *start = l->src + l->pos;
    int depth = 1;
    while (depth > 0)
    {
        Token t = lexer_next(l);
        if (t.kind == TOK_EOF)
        {
            zpanic_at(t, "Unexpected EOF in comptime block");
            return NULL;
        }
        if (t.kind == TOK_STRING || t.kind == TOK_FSTRING || t.kind == TOK_RAW_STRING)
        {
            continue;
        }
        if (t.kind == TOK_LBRACE)
        {
            depth++;
        }
        if (t.kind == TOK_RBRACE)
        {
            depth--;
        }
    }
    ptrdiff_t len = (l->src + l->pos - 1) - start;
    char *code = xmalloc((size_t)(len + 1));
    strncpy(code, start, (size_t)(len));
    code[len] = 0;

    size_t wrapped_len = (size_t)len + 4;
    char *wrapped = xmalloc((size_t)(wrapped_len + 1));
    sprintf(wrapped, "{ %s }", code); /* safe */
    zfree(code);

    Lexer cl;
    lexer_init(&cl, wrapped, ctx->config, ctx->current_filename);
    ASTNode *block = parse_block(ctx, &cl);
    zfree(wrapped);
    if (!block)
    {
        return NULL;
    }

    ASTNode *stmts = block->block.statements;
    block->block.statements = NULL;
    ast_free(block);
    return stmts;
}

ASTNode *parse_plugin(ParserContext *ctx, Lexer *l, Token tok)
{
    (void)ctx;

    Token tk = lexer_next(l);
    if (tk.kind != TOK_IDENT)
    {
        zpanic_at(tk, "Expected plugin name after 'plugin' keyword");
        return NULL;
    }

    char *plugin_name = xmalloc(tk.len + 1);
    strncpy(plugin_name, tk.start, tk.len);
    plugin_name[tk.len] = '\0';

    char *body = xmalloc(8192);
    body[0] = '\0';
    int body_len = 0;

    while (1)
    {
        Token t = lexer_peek(l);
        if (t.kind == TOK_EOF)
        {
            zpanic_at(t, "Unexpected EOF in plugin block, expected 'end'");
            break;
        }

        if (t.kind == TOK_IDENT && t.len == 3 && strncmp(t.start, "end", 3) == 0)
        {
            lexer_next(l);
            break;
        }

        if ((size_t)((size_t)(body_len) + t.len) + 2 < 8192)
        {
            strncat(body, t.start, t.len);
            body[(size_t)((size_t)(body_len) + t.len)] = ' ';
            body[(size_t)((size_t)(body_len) + t.len) + 1] = '\0';
            body_len += (int)(t.len) + 1;
        }

        lexer_next(l);
    }

    ASTNode *n = ast_create(NODE_PLUGIN);
    n->token = tok;
    n->plugin_stmt.plugin_name = plugin_name;
    n->plugin_stmt.body = body;

    if (lexer_peek(l).kind == TOK_SEMICOLON)
    {
        lexer_next(l);
    }
    return n;
}
