// SPDX-License-Identifier: MIT
// Unit tests of src/parser/utils/utils_registry.c: the cache of the names that
// find_struct_def did not find. A cached miss must not hide a struct or enum
// registered after it. And find_func, which finds a function in the current scopes or else
// in the registry of every function declared.
#include "unit_parse.h"
#include <stdio.h>
#include <time.h>
#include "unit_test.h"

static ASTNode *new_struct(const char *name)
{
    ASTNode *node = ast_create(NODE_STRUCT);
    node->strct.name = (char *)name;
    return node;
}

static ASTNode *new_enum(const char *name)
{
    ASTNode *node = ast_create(NODE_ENUM);
    node->enm.name = (char *)name;
    return node;
}

// A parsed program with one struct, and the miss cache turned on as codegen does.
static ParserContext *context_with_miss_cache(void)
{
    parse_source("struct Point { x: int; y: int; }\n");
    ParserContext *ctx = parse_source_context();
    struct_miss_cache_enable(ctx);
    return ctx;
}

UTEST(struct_miss_cache, finds_a_parsed_struct)
{
    ParserContext *ctx = context_with_miss_cache();
    ASSERT_TRUE(find_struct_def(ctx, "Point") != NULL);
    ASSERT_TRUE(find_struct_def(ctx, "Missing") == NULL);
    ASSERT_TRUE(find_struct_def(ctx, "Missing") == NULL);
}

UTEST(struct_miss_cache, sees_a_struct_registered_after_a_miss)
{
    ParserContext *ctx = context_with_miss_cache();
    ASSERT_TRUE(find_struct_def(ctx, "Later") == NULL);

    ASTNode *later = new_struct("Later");
    register_struct_def(ctx, "Later", later);
    ASSERT_TRUE(find_struct_def(ctx, "Later") == later);
}

UTEST(struct_miss_cache, sees_a_struct_added_to_the_list_after_a_miss)
{
    ParserContext *ctx = context_with_miss_cache();
    ASSERT_TRUE(find_struct_def(ctx, "Listed") == NULL);

    ASTNode *listed = new_struct("Listed");
    add_to_struct_list(ctx, listed);
    ASSERT_TRUE(find_struct_def(ctx, "Listed") == listed);
}

UTEST(struct_miss_cache, sees_an_enum_added_after_a_miss)
{
    ParserContext *ctx = context_with_miss_cache();
    ASSERT_TRUE(find_struct_def(ctx, "Color") == NULL);

    ASTNode *color = new_enum("Color");
    add_to_enum_list(ctx, color);
    ASSERT_TRUE(find_struct_def(ctx, "Color") == color);
}

// The contract of the cache: a struct added to a list without calling
// struct_registry_changed stays hidden by a cached miss until someone does.
UTEST(struct_miss_cache, needs_struct_registry_changed_for_a_direct_change)
{
    ParserContext *ctx = context_with_miss_cache();
    ASSERT_TRUE(find_struct_def(ctx, "Direct") == NULL);

    ASTNode *direct = new_struct("Direct");
    direct->next = ctx->instantiated_structs;
    ctx->instantiated_structs = direct;
    ASSERT_TRUE(find_struct_def(ctx, "Direct") == NULL);

    struct_registry_changed(ctx);
    ASSERT_TRUE(find_struct_def(ctx, "Direct") == direct);
}

// Without the cache (the parser's own lookups), a struct added directly is found.
UTEST(struct_miss_cache, is_off_until_enabled)
{
    parse_source("struct Point { x: int; }\n");
    ParserContext *ctx = parse_source_context();
    ASSERT_TRUE(find_struct_def(ctx, "Direct") == NULL);

    ASTNode *direct = new_struct("Direct");
    direct->next = ctx->instantiated_structs;
    ctx->instantiated_structs = direct;
    ASSERT_TRUE(find_struct_def(ctx, "Direct") == direct);
}

static void register_function(ParserContext *ctx, Scope *scope, const char *name, int count)
{
    register_func(ctx, scope, name, count, NULL, NULL, NULL, 0, 0, NULL, TOKEN_UNKNOWN, 0);
}

UTEST(find_func, finds_a_registered_function)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    register_function(ctx, NULL, "alpha", 2);
    FuncSig *sig = find_func(ctx, "alpha");
    ASSERT_TRUE(sig != NULL);
    ASSERT_EQ(sig->total_args, 2);
    ASSERT_TRUE(find_func(ctx, "missing") == NULL);
}

// A function registered in a scope that is not one of the current ones (a method, an import)
// is found in the registry, the one registered last first.
UTEST(find_func, finds_a_function_of_another_scope_in_the_registry)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    Scope *other = symbol_scope_create(NULL, NULL);
    register_function(ctx, other, "beta", 3);
    ASSERT_EQ(find_func(ctx, "beta")->total_args, 3);
    register_function(ctx, other, "beta", 4);
    ASSERT_EQ(find_func(ctx, "beta")->total_args, 4);
}

// Codegen asks find_func for many names that are no function: such a miss must not walk every
// function of the program. 30,000 misses among 30,000 functions take milliseconds when each
// costs the same, and seconds when each walks the registry.
UTEST(find_func, a_miss_takes_constant_time)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    Scope *other = symbol_scope_create(NULL, NULL);
    enum
    {
        FUNCTION_COUNT = 30000
    };
    char name[32];
    for (int i = 0; i < FUNCTION_COUNT; i++)
    {
        snprintf(name, sizeof(name), "fn_%d", i);
        register_function(ctx, other, name, 0);
    }
    int found = 0;
    clock_t start = clock();
    for (int i = 0; i < FUNCTION_COUNT; i++)
    {
        snprintf(name, sizeof(name), "fn_missing_%d", i);
        found += find_func(ctx, name) != NULL;
    }
    double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    ASSERT_EQ(found, 0);
    ASSERT_LT(seconds, 1.0);
}
