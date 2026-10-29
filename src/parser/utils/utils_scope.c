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

void enter_scope(ParserContext *ctx)
{
    Scope *s = symbol_scope_create(ctx->current_scope, NULL);
    ctx->current_scope = s;
}

void exit_scope(ParserContext *ctx)
{
    if (!ctx->current_scope || ctx->current_scope == ctx->global_scope)
    {
        return;
    }

    ZenSymbol *sym = ctx->current_scope->symbols;
    while (sym)
    {
        if (!sym->is_used && strcmp(sym->name, "self") != 0 && sym->name[0] != '_')
        {
        }
        sym = sym->next;
    }

    ctx->current_scope = ctx->current_scope->parent;
}

// all_symbols_set: a declaration is the same when its kind, name and position are.
static int declaration_cmp(const ZenSymbol *a, const ZenSymbol *b)
{
    if (a->kind != b->kind || a->decl_token.line != b->decl_token.line ||
        a->decl_token.col != b->decl_token.col)
    {
        return 1;
    }
    return strcmp(a->name, b->name);
}

static uint32_t declaration_hash(const ZenSymbol *s, uint32_t seed)
{
    uint32_t h = zmap_hash_cstr(s->name, seed);
    h ^= (uint32_t)s->kind;
    h *= 16777619u;
    h ^= (uint32_t)s->decl_token.line;
    h *= 16777619u;
    h ^= (uint32_t)s->decl_token.col;
    h *= 16777619u;
    return h;
}

void register_in_all_symbols(ParserContext *ctx, ZenSymbol *s)
{
    if (!ctx || !s)
    {
        return;
    }

    // The language server registers every file of a workspace in one context, so a declaration
    // registered before is found in a set instead of walking all_symbols.
    if (s->name)
    {
        // Every ParserContext starts zeroed, wherever it is created: the set starts here.
        if (!ctx->all_symbols_set.hash_func)
        {
            ctx->all_symbols_set = zmap_init(DeclSet, declaration_hash, declaration_cmp);
        }
        if (zmap_get(&ctx->all_symbols_set, s))
        {
            return;
        }
    }

    ZenSymbol *lsp_copy = xmalloc(sizeof(ZenSymbol));
    memcpy(lsp_copy, s, sizeof(ZenSymbol));
    lsp_copy->original = s;
    lsp_copy->next = ctx->all_symbols;
    ctx->all_symbols = lsp_copy;
    if (s->name)
    {
        lsp_copy->name = xstrdup(s->name);
    }
    if (s->cfg_condition)
    {
        lsp_copy->cfg_condition = xstrdup(s->cfg_condition);
    }

    lsp_copy->is_local = s->is_local;

    if (s->name)
    {
        zmap_put(&ctx->all_symbols_set, lsp_copy, lsp_copy);
    }
}
