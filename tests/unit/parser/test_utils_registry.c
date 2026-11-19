// SPDX-License-Identifier: MIT
// Unit tests of src/parser/utils/utils_registry.c: find_struct_def, which reads the registry of
// every struct and enum by name, also for one registered after a lookup missed it. And
// find_func, which finds a function in the registry of every function declared.
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

// A parsed program with one struct.
static ParserContext *context_with_point(void)
{
    parse_source("struct Point { x: int; y: int; }\n");
    return parse_source_context();
}

UTEST(type_registry, finds_a_parsed_struct)
{
    ParserContext *ctx = context_with_point();
    ASSERT_TRUE(find_struct_def(ctx, "Point") != NULL);
    ASSERT_TRUE(find_struct_def(ctx, "Missing") == NULL);
    ASSERT_TRUE(find_struct_def(ctx, "Missing") == NULL);
}

UTEST(type_registry, finds_a_struct_registered_after_a_miss)
{
    ParserContext *ctx = context_with_point();
    ASSERT_TRUE(find_struct_def(ctx, "Later") == NULL);

    ASTNode *later = new_struct("Later");
    register_struct_def(ctx, "Later", later);
    ASSERT_TRUE(find_struct_def(ctx, "Later") == later);
}

UTEST(type_registry, finds_a_struct_added_to_the_list_after_a_miss)
{
    ParserContext *ctx = context_with_point();
    ASSERT_TRUE(find_struct_def(ctx, "Listed") == NULL);

    ASTNode *listed = new_struct("Listed");
    add_to_struct_list(ctx, listed);
    ASSERT_TRUE(find_struct_def(ctx, "Listed") == listed);
}

UTEST(type_registry, finds_an_enum_added_after_a_miss)
{
    ParserContext *ctx = context_with_point();
    ASSERT_TRUE(find_struct_def(ctx, "Color") == NULL);

    ASTNode *color = new_enum("Color");
    add_to_enum_list(ctx, color);
    ASSERT_TRUE(find_struct_def(ctx, "Color") == color);
}

// The one registered last answers: an instance or a redeclaration seen by the language server.
UTEST(type_registry, keeps_the_one_registered_last)
{
    ParserContext *ctx = context_with_point();
    ASTNode *first = new_struct("Twice");
    ASTNode *second = new_struct("Twice");
    register_named_type(ctx, "Twice", first);
    register_named_type(ctx, "Twice", second);
    ASSERT_TRUE(find_struct_def(ctx, "Twice") == second);
}

// A generic template is found by its name, as the parser registers it.
UTEST(type_registry, finds_a_generic_template)
{
    parse_source("struct Pair<T> { a: T; b: T; }\n");
    ParserContext *ctx = parse_source_context();
    ASTNode *pair = find_struct_def(ctx, "Pair");
    ASSERT_TRUE(pair != NULL);
    ASSERT_TRUE(pair->strct.is_template);
}

static void register_signature(ParserContext *ctx, Scope *scope, const char *name, int count)
{
    register_func(ctx, scope, name, count, NULL, NULL, NULL, 0, 0, NULL, NULL, TOKEN_UNKNOWN, 0);
}

UTEST(find_func, finds_a_registered_function)
{
    parse_source("");
    ParserContext *ctx = parse_source_context();
    register_signature(ctx, NULL, "alpha", 2);
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
    register_signature(ctx, other, "beta", 3);
    ASSERT_EQ(find_func(ctx, "beta")->total_args, 3);
    register_signature(ctx, other, "beta", 4);
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
        register_signature(ctx, other, name, 0);
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

// The registry of declarations by module (register_declaration, find_file_declaration): each
// top-level declaration under the file that makes it and the name it writes, a generic function
// too (the parser registers it as a template and does not hand it to the program).
UTEST(file_declarations, every_kind_by_the_name_written)
{
    parse_source("struct Point { x: int; }\n"
                 "enum Shade { Light, Dark }\n"
                 "trait Named { fn name(self) -> int; }\n"
                 "type Count = int;\n"
                 "def LIMIT = 3;\n"
                 "let total: int = 0;\n"
                 "fn twice(x: int) -> int { return x * 2; }\n"
                 "fn first<T>(x: T) -> T { return x; }\n");
    ParserContext *ctx = parse_source_context();
    const char *names[] = {"Point", "Shade", "Named", "Count", "LIMIT", "total", "twice", "first"};
    NodeType kinds[] = {NODE_STRUCT, NODE_ENUM,    NODE_TRAIT,    NODE_TYPE_ALIAS,
                        NODE_CONST,  NODE_VAR_DECL, NODE_FUNCTION, NODE_FUNCTION};
    for (int i = 0; i < 8; i++)
    {
        ASTNode *found = find_file_declaration(ctx, "test.zc", names[i]);
        ASSERT_TRUE(found != NULL);
        ASSERT_EQ(kinds[i], found->kind);
    }
    ASSERT_TRUE(find_file_declaration(ctx, "test.zc", "Missing") == NULL);
    ASSERT_TRUE(find_file_declaration(ctx, "other.zc", "Point") == NULL);
}
