// SPDX-License-Identifier: MIT
// Unit tests of src/parser/struct/struct_impl.c: the methods of an impl body.
#include "unit_parse.h"
#include <string.h>
#include "unit_test.h"

// The first node of `kind` under `node`, in source order.
static ASTNode *first_of_kind(ASTNode *node, NodeType kind)
{
    for (; node; node = node->next)
    {
        if (node->kind == kind)
        {
            return node;
        }
        ASTChildren children = ast_children(node);
        for (int i = 0; i < children.count; i++)
        {
            ASTNode *found = first_of_kind(*children.slots[i], kind);
            if (found)
            {
                return found;
            }
        }
    }
    return NULL;
}

static const char *const IMPL_WITH_DOCS = "struct Point { x: int; }\n"
                                          "impl Point {\n"
                                          "    /// Builds a point.\n"
                                          "    fn new() -> Point { return Point { x: 0 }; }\n"
                                          "    /// The x coordinate.\n"
                                          "    fn get_x(self) -> int { return self.x; }\n"
                                          "}\n"
                                          "/// Adds two numbers.\n"
                                          "fn add(a: int, b: int) -> int { return a + b; }\n";

UTEST(impl_methods, each_method_keeps_its_doc_comment)
{
    ASTNode *impl = first_of_kind(parse_source_keeping_comments(IMPL_WITH_DOCS), NODE_IMPL);
    ASSERT_TRUE(impl != NULL);
    ASTNode *new_fn = impl->impl.methods;
    ASSERT_TRUE(new_fn != NULL && new_fn->next != NULL);
    ASTNode *get_x = new_fn->next;

    ASSERT_TRUE(new_fn->doc_comment && strstr(new_fn->doc_comment, "Builds a point."));
    ASSERT_FALSE(strstr(new_fn->doc_comment, "The x coordinate."));
    ASSERT_TRUE(get_x->doc_comment && strstr(get_x->doc_comment, "The x coordinate."));
    ASSERT_FALSE(strstr(get_x->doc_comment, "Builds a point."));
}

UTEST(impl_methods, their_doc_comments_do_not_reach_what_follows)
{
    ASTNode *root = parse_source_keeping_comments(IMPL_WITH_DOCS);
    ASTNode *impl = first_of_kind(root, NODE_IMPL);
    ASSERT_TRUE(impl != NULL);
    ASSERT_FALSE(impl->doc_comment && strstr(impl->doc_comment, "Builds a point."));

    ASTNode *add = impl->next;
    while (add && !(add->kind == NODE_FUNCTION && strcmp(add->func.name, "add") == 0))
    {
        add = add->next;
    }
    ASSERT_TRUE(add != NULL && add->doc_comment);
    ASSERT_TRUE(strstr(add->doc_comment, "Adds two numbers."));
    ASSERT_FALSE(strstr(add->doc_comment, "coordinate"));
}
