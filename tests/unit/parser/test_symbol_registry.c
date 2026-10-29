// SPDX-License-Identifier: MIT
// Unit tests of register_symbol_to_lsp (src/parser/utils/utils_scope.c): the list of every
// symbol seen (ctx->all_symbols) that the language server and the MISRA checks use. A symbol
// registered again from the same declaration is kept once.
#include "unit_parse.h"
#include <string.h>
#include <time.h>
#include "unit_test.h"

static ZenSymbol *new_symbol(const char *name, SymbolKind kind, int line, int col)
{
    ZenSymbol *symbol = xcalloc(1, sizeof(ZenSymbol));
    symbol->name = (char *)name;
    symbol->kind = kind;
    symbol->decl_token.line = line;
    symbol->decl_token.col = col;
    return symbol;
}

static int count_registered(ParserContext *ctx, const char *name)
{
    int count = 0;
    for (ZenSymbol *symbol = ctx->all_symbols; symbol; symbol = symbol->next)
    {
        if (strcmp(symbol->name, name) == 0)
        {
            count++;
        }
    }
    return count;
}

UTEST(lsp_symbol_registry, keeps_a_declaration_registered_twice_once)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    register_symbol_to_lsp(ctx, new_symbol("registered_twice", SYM_VARIABLE, 3, 5));
    register_symbol_to_lsp(ctx, new_symbol("registered_twice", SYM_VARIABLE, 3, 5));
    ASSERT_EQ(count_registered(ctx, "registered_twice"), 1);
}

UTEST(lsp_symbol_registry, keeps_declarations_that_differ_in_position_or_kind)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    register_symbol_to_lsp(ctx, new_symbol("same_name", SYM_VARIABLE, 3, 5));
    register_symbol_to_lsp(ctx, new_symbol("same_name", SYM_VARIABLE, 4, 5));
    register_symbol_to_lsp(ctx, new_symbol("same_name", SYM_VARIABLE, 3, 9));
    register_symbol_to_lsp(ctx, new_symbol("same_name", SYM_FUNCTION, 3, 5));
    ASSERT_EQ(count_registered(ctx, "same_name"), 4);
}

// The language server registers the symbols of every file of a workspace in one context: each
// registration must not walk all the earlier ones. 30,000 of them take milliseconds when each
// one costs the same, and over ten seconds when each one walks the list.
UTEST(lsp_symbol_registry, registering_many_symbols_takes_linear_time)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    enum
    {
        SYMBOL_COUNT = 30000
    };
    clock_t start = clock();
    for (int i = 0; i < SYMBOL_COUNT; i++)
    {
        register_symbol_to_lsp(ctx, new_symbol("many", SYM_VARIABLE, i + 1, 1));
    }
    double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    ASSERT_EQ(count_registered(ctx, "many"), SYMBOL_COUNT);
    ASSERT_LT(seconds, 2.0);
}
