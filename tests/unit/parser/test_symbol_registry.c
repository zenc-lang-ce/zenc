// SPDX-License-Identifier: MIT
// Unit tests of register_in_all_symbols (src/parser/utils/utils_scope.c): the list of every
// symbol seen (ctx->all_symbols) that the language server and the MISRA checks use. A symbol
// registered again from the same declaration is kept once.
#include "unit_parse.h"
#include <stdio.h>
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

UTEST(all_symbols_registry, keeps_a_declaration_registered_twice_once)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    register_in_all_symbols(ctx, new_symbol("registered_twice", SYM_VARIABLE, 3, 5));
    register_in_all_symbols(ctx, new_symbol("registered_twice", SYM_VARIABLE, 3, 5));
    ASSERT_EQ(count_registered(ctx, "registered_twice"), 1);
}

UTEST(all_symbols_registry, keeps_declarations_that_differ_in_position_or_kind)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    register_in_all_symbols(ctx, new_symbol("same_name", SYM_VARIABLE, 3, 5));
    register_in_all_symbols(ctx, new_symbol("same_name", SYM_VARIABLE, 4, 5));
    register_in_all_symbols(ctx, new_symbol("same_name", SYM_VARIABLE, 3, 9));
    register_in_all_symbols(ctx, new_symbol("same_name", SYM_FUNCTION, 3, 5));
    ASSERT_EQ(count_registered(ctx, "same_name"), 4);
}

// The language server registers the symbols of every file of a workspace in one context: each
// registration must not walk all the earlier ones. 30,000 of them take milliseconds when each
// one costs the same, and over ten seconds when each one walks the list.
UTEST(all_symbols_registry, registering_many_symbols_takes_linear_time)
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
        register_in_all_symbols(ctx, new_symbol("many", SYM_VARIABLE, i + 1, 1));
    }
    double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    ASSERT_EQ(count_registered(ctx, "many"), SYMBOL_COUNT);
    ASSERT_LT(seconds, 2.0);
}

// find_symbol_in_all finds the symbol of a name registered last.
UTEST(all_symbols_registry, finds_the_symbol_of_a_name_registered_last)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    register_in_all_symbols(ctx, new_symbol("found", SYM_FUNCTION, 1, 1));
    register_in_all_symbols(ctx, new_symbol("other", SYM_VARIABLE, 2, 1));
    register_in_all_symbols(ctx, new_symbol("found", SYM_VARIABLE, 3, 1));
    ZenSymbol *found = find_symbol_in_all(ctx, "found");
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->decl_token.line, 3);
    ASSERT_TRUE(find_symbol_in_all(ctx, "missing") == NULL);
}

// Codegen looks up the names it emits in every symbol of the program: each lookup must not walk
// them all. 30,000 lookups among 30,000 symbols take milliseconds when each costs the same, and
// seconds when each walks the list.
UTEST(all_symbols_registry, finding_a_symbol_takes_constant_time)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    enum
    {
        SYMBOL_COUNT = 30000
    };
    static char names[SYMBOL_COUNT][24];
    for (int i = 0; i < SYMBOL_COUNT; i++)
    {
        snprintf(names[i], sizeof(names[i]), "registered_%d", i);
        register_in_all_symbols(ctx, new_symbol(names[i], SYM_VARIABLE, i + 1, 1));
    }
    int found = 0;
    clock_t start = clock();
    for (int i = 0; i < SYMBOL_COUNT; i++)
    {
        found += find_symbol_in_all(ctx, names[i]) != NULL;
    }
    double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    ASSERT_EQ(found, SYMBOL_COUNT);
    ASSERT_LT(seconds, 1.0);
}
