// SPDX-License-Identifier: MIT
// Unit tests of src/ast/ast.c.
#include "ast/ast.h"
#include "arena.h"
#include "unit_test.h"

UTEST(ast_children, if_gives_condition_then_else_in_order)
{
    ASTNode *node = ast_create(NODE_IF);
    ASTChildren c = ast_children(node);
    ASSERT_EQ(3, c.count);
    ASSERT_TRUE(c.slots[0] == &node->if_stmt.condition);
    ASSERT_TRUE(c.slots[1] == &node->if_stmt.then_body);
    ASSERT_TRUE(c.slots[2] == &node->if_stmt.else_body);
}

UTEST(ast_children, a_slot_can_replace_the_child)
{
    ASTNode *node = ast_create(NODE_IF);
    ASTNode *cond = ast_create(NODE_EXPR_VAR);
    ASTChildren c = ast_children(node);
    *c.slots[0] = cond;
    ASSERT_TRUE(node->if_stmt.condition == cond);
}

UTEST(ast_children, import_does_not_enter_the_module)
{
    ASTNode *node = ast_create(NODE_IMPORT);
    node->import_stmt.module_root = ast_create(NODE_FUNCTION);
    ASSERT_EQ(0, ast_children(node).count);
}

UTEST(ast_children, comptime_gives_body_and_generated)
{
    ASTNode *node = ast_create(NODE_COMPTIME);
    ASTChildren c = ast_children(node);
    ASSERT_EQ(2, c.count);
    ASSERT_TRUE(c.slots[0] == &node->comptime.body);
    ASSERT_TRUE(c.slots[1] == &node->comptime.generated);
}

UTEST(ast_children, a_leaf_has_none)
{
    ASSERT_EQ(0, ast_children(ast_create(NODE_EXPR_LITERAL)).count);
}

UTEST(ast_children, null_has_none)
{
    ASSERT_EQ(0, ast_children(NULL).count);
}

UTEST(ast_children, no_kind_has_more_than_the_maximum)
{
    for (int kind = NODE_ROOT; kind <= NODE_ERRONEOUS; kind++)
    {
        ASTChildren c = ast_children(ast_create((NodeType)kind));
        ASSERT_LE(c.count, AST_MAX_CHILDREN);
    }
}

// A tuple of two elements: an int and a pointer to a struct.
static Type *int_and_point_pointer(TypeKind first)
{
    Type *point = type_new(TYPE_STRUCT);
    point->name = "Point";
    Type **elements = xmalloc(sizeof(Type *) * 2);
    elements[0] = type_new(first);
    elements[1] = type_new_ptr(point);
    return type_new_tuple(elements, 2);
}

UTEST(type_tuple, is_named_after_its_elements_in_c)
{
    Type *tuple = int_and_point_pointer(TYPE_I32);
    ASSERT_STREQ("Tuple__int32_t__PointPtr", type_to_c_string(tuple, 0));
    ASSERT_STREQ("Tuple__int32_t__PointPtr", type_to_string(tuple));
}

UTEST(type_tuple, equals_a_tuple_of_the_same_elements)
{
    ASSERT_TRUE(type_eq(int_and_point_pointer(TYPE_I32), int_and_point_pointer(TYPE_I32)));
}

UTEST(type_tuple, differs_from_one_whose_element_is_another_integer)
{
    // type_eq lets an int stand for a u8, but the two tuples are different C structs.
    ASSERT_FALSE(type_eq(int_and_point_pointer(TYPE_I32), int_and_point_pointer(TYPE_U8)));
}
