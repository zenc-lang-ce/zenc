// SPDX-License-Identifier: MIT
// Unit tests of the scopes of src/ast/symbols.c: a lookup finds the most recent symbol of a
// name, of a kind when asked, in the scope or else in its parents. Each case runs on a small
// scope and on one holding many symbols, which the lookups may index by name.
#include "unit_parse.h"
#include <stdio.h>
#include <time.h>
#include "unit_test.h"

// Adds `count` symbols with distinct names to s, as a large scope (the global one) has.
static void fill(Scope *s, int count)
{
    char name[32];
    for (int i = 0; i < count; i++)
    {
        snprintf(name, sizeof(name), "filler_%d", i);
        symbol_add(s, name, SYM_VARIABLE);
    }
}

static void finds_the_most_recent_of_a_name(int *utest_result, int filler)
{
    Scope *s = symbol_scope_create(NULL, NULL);
    fill(s, filler);
    symbol_add(s, "x", SYM_VARIABLE);
    fill(s, filler);
    ZenSymbol *newest = symbol_add(s, "x", SYM_VARIABLE);
    ASSERT_TRUE(symbol_lookup_local(s, "x") == newest);
    ASSERT_TRUE(symbol_lookup(s, "x") == newest);
    ASSERT_TRUE(symbol_lookup_local(s, "missing") == NULL);
}

UTEST(scope_lookup, finds_the_most_recent_of_a_name)
{
    finds_the_most_recent_of_a_name(utest_result, 0);
}

UTEST(scope_lookup, finds_the_most_recent_of_a_name_in_a_large_scope)
{
    finds_the_most_recent_of_a_name(utest_result, 100);
}

static void finds_the_most_recent_of_a_kind(int *utest_result, int filler)
{
    Scope *s = symbol_scope_create(NULL, NULL);
    ZenSymbol *function = symbol_add(s, "f", SYM_FUNCTION);
    fill(s, filler);
    ZenSymbol *variable = symbol_add(s, "f", SYM_VARIABLE);
    fill(s, filler);
    ASSERT_TRUE(symbol_lookup_kind(s, "f", SYM_FUNCTION) == function);
    ASSERT_TRUE(symbol_lookup_kind(s, "f", SYM_VARIABLE) == variable);
    ASSERT_TRUE(symbol_lookup_local(s, "f") == variable);
}

UTEST(scope_lookup, finds_the_most_recent_of_a_kind)
{
    finds_the_most_recent_of_a_kind(utest_result, 0);
}

UTEST(scope_lookup, finds_the_most_recent_of_a_kind_in_a_large_scope)
{
    finds_the_most_recent_of_a_kind(utest_result, 100);
}

static void looks_in_the_parent_scopes(int *utest_result, int filler)
{
    Scope *global = symbol_scope_create(NULL, NULL);
    fill(global, filler);
    ZenSymbol *outer = symbol_add(global, "y", SYM_VARIABLE);
    ZenSymbol *function = symbol_add(global, "g", SYM_FUNCTION);
    Scope *block = symbol_scope_create(global, NULL);
    ZenSymbol *inner = symbol_add(block, "y", SYM_VARIABLE);
    ASSERT_TRUE(symbol_lookup(block, "y") == inner);
    ASSERT_TRUE(symbol_lookup(global, "y") == outer);
    ASSERT_TRUE(symbol_lookup_local(block, "g") == NULL);
    ASSERT_TRUE(symbol_lookup(block, "g") == function);
    ASSERT_TRUE(symbol_lookup_kind(block, "g", SYM_FUNCTION) == function);
}

UTEST(scope_lookup, looks_in_the_parent_scopes)
{
    looks_in_the_parent_scopes(utest_result, 0);
}

UTEST(scope_lookup, looks_in_the_parent_scopes_of_a_large_scope)
{
    looks_in_the_parent_scopes(utest_result, 100);
}

// The global scope holds every symbol of a program and its imports (of a whole workspace, in
// the language server), and each name lookup must not walk them all. 50,000 lookups in a scope
// of 50,000 symbols take milliseconds when each costs the same, and seconds when each walks
// the list.
UTEST(scope_lookup, lookups_in_a_large_scope_take_constant_time)
{
    enum
    {
        SYMBOL_COUNT = 50000
    };
    Scope *s = symbol_scope_create(NULL, NULL);
    fill(s, SYMBOL_COUNT);
    char name[32];
    int found = 0;
    clock_t start = clock();
    for (int i = 0; i < SYMBOL_COUNT; i++)
    {
        snprintf(name, sizeof(name), "filler_%d", i);
        found += symbol_lookup_kind(s, name, SYM_VARIABLE) != NULL;
    }
    double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    ASSERT_EQ(found, SYMBOL_COUNT);
    ASSERT_LT(seconds, 1.0);
}
