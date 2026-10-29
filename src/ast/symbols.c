// SPDX-License-Identifier: MIT
#include "../arena.h"
#include "symbols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Scope *symbol_scope_create(Scope *parent, const char *name)
{
    Scope *s = xmalloc(sizeof(Scope));
    memset(s, 0, sizeof(Scope));
    s->parent = parent;
    s->by_name = zmap_init(SymbolByName, zmap_hash_cstr, zmap_cmp_cstr);
    if (name)
    {
        s->name = xstrdup(name);
    }
    return s;
}

ZenSymbol *symbol_add(Scope *s, const char *name, SymbolKind kind)
{
    if (!s || !name)
    {
        return NULL;
    }

    ZenSymbol *sym = xmalloc(sizeof(ZenSymbol));
    memset(sym, 0, sizeof(ZenSymbol));
    sym->name = xstrdup(name);
    sym->kind = kind;

    sym->next = s->symbols;
    s->symbols = sym;

    // The index keeps the newest symbol of each name; the older ones follow it, newest first.
    ZenSymbol **newest = zmap_get(&s->by_name, sym->name);
    sym->next_same_name = newest ? *newest : NULL;
    zmap_put(&s->by_name, sym->name, sym);

    return sym;
}

// The newest symbol of name in s alone. The global scope holds every symbol of a program and its
// imports (of a whole workspace, in the language server), so lookups do not walk the list.
static ZenSymbol *scope_newest(Scope *s, const char *name)
{
    ZenSymbol **newest = zmap_get(&s->by_name, name);
    return newest ? *newest : NULL;
}

ZenSymbol *symbol_lookup_local(Scope *s, const char *name)
{
    if (!s || !name)
    {
        return NULL;
    }
    return scope_newest(s, name);
}

ZenSymbol *symbol_lookup(Scope *s, const char *name)
{
    if (!name)
    {
        return NULL;
    }

    for (Scope *curr_scope = s; curr_scope; curr_scope = curr_scope->parent)
    {
        ZenSymbol *sym = scope_newest(curr_scope, name);
        if (sym)
        {
            return sym;
        }
    }
    return NULL;
}

ZenSymbol *symbol_lookup_kind(Scope *s, const char *name, SymbolKind kind)
{
    if (!name)
    {
        return NULL;
    }

    for (Scope *curr_scope = s; curr_scope; curr_scope = curr_scope->parent)
    {
        for (ZenSymbol *sym = scope_newest(curr_scope, name); sym; sym = sym->next_same_name)
        {
            if (sym->kind == kind)
            {
                return sym;
            }
        }
    }
    return NULL;
}
