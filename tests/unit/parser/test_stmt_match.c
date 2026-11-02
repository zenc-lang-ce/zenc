// SPDX-License-Identifier: MIT
// Unit tests of src/parser/stmt/stmt_match.c: the alternatives of a match case, kept as
// written (MatchPattern) rather than as the C name of a variant.
#include "unit_parse.h"
#include <string.h>
#include "unit_test.h"

// The n-th node of `kind` under `node` (0 is the first), in source order; *n counts down.
static ASTNode *nth_of_kind(ASTNode *node, NodeType kind, int *n)
{
    for (; node; node = node->next)
    {
        if (node->kind == kind && (*n)-- == 0)
        {
            return node;
        }
        ASTChildren children = ast_children(node);
        for (int i = 0; i < children.count; i++)
        {
            ASTNode *found = nth_of_kind(*children.slots[i], kind, n);
            if (found)
            {
                return found;
            }
        }
    }
    return NULL;
}

// The cases of the n-th match of the source.
static ASTNode *match_cases(const char *src, int n)
{
    ASTNode *match = nth_of_kind(parse_source(src), NODE_MATCH, &n);
    return match ? match->match_stmt.cases : NULL;
}

static const char *const MATCHES = "enum Color { Red, Green, Blue }\n"
                                   "fn pick(c: Color, n: int) -> int {\n"
                                   "    match c {\n"
                                   "        Color::Red => { return 1; },\n"
                                   "        Green || Blue => { return 2; },\n"
                                   "    }\n"
                                   "    match n {\n"
                                   "        1..=5 => { return 3; },\n"
                                   "        6..9 => { return 4; },\n"
                                   "        42 => { return 5; },\n"
                                   "        _ => { return 6; },\n"
                                   "    }\n"
                                   "    return 0;\n"
                                   "}\n";

UTEST(match_patterns, a_qualified_variant_keeps_its_enum_and_name)
{
    ASTNode *c = match_cases(MATCHES, 0);
    ASSERT_TRUE(c != NULL && c->match_case.patterns != NULL);
    MatchPattern *p = c->match_case.patterns;
    ASSERT_EQ(PATTERN_NAME, p->kind);
    ASSERT_STREQ("Color", p->name.scope);
    ASSERT_STREQ("Red", p->name.name);
    ASSERT_TRUE(p->next == NULL);
}

UTEST(match_patterns, alternatives_are_a_list_of_bare_names)
{
    ASTNode *c = match_cases(MATCHES, 0);
    ASSERT_TRUE(c != NULL && c->next != NULL);
    MatchPattern *p = c->next->match_case.patterns;
    ASSERT_TRUE(p != NULL && p->next != NULL);
    ASSERT_EQ(PATTERN_NAME, p->kind);
    ASSERT_TRUE(p->name.scope == NULL);
    ASSERT_STREQ("Green", p->name.name);
    ASSERT_EQ(PATTERN_NAME, p->next->kind);
    ASSERT_STREQ("Blue", p->next->name.name);
    ASSERT_TRUE(p->next->next == NULL);
}

UTEST(match_patterns, ranges_literals_and_the_wildcard)
{
    ASTNode *c = match_cases(MATCHES, 1);
    ASSERT_TRUE(c != NULL);

    MatchPattern *inclusive = c->match_case.patterns;
    ASSERT_EQ(PATTERN_RANGE, inclusive->kind);
    ASSERT_STREQ("1", inclusive->range_start);
    ASSERT_STREQ("5", inclusive->range_end);
    ASSERT_TRUE(inclusive->is_inclusive);

    c = c->next;
    ASSERT_TRUE(c != NULL);
    MatchPattern *exclusive = c->match_case.patterns;
    ASSERT_EQ(PATTERN_RANGE, exclusive->kind);
    ASSERT_STREQ("6", exclusive->range_start);
    ASSERT_STREQ("9", exclusive->range_end);
    ASSERT_FALSE(exclusive->is_inclusive);

    c = c->next;
    ASSERT_TRUE(c != NULL);
    ASSERT_EQ(PATTERN_LITERAL, c->match_case.patterns->kind);
    ASSERT_STREQ("42", c->match_case.patterns->literal);

    c = c->next;
    ASSERT_TRUE(c != NULL);
    ASSERT_EQ(PATTERN_WILDCARD, c->match_case.patterns->kind);
}
