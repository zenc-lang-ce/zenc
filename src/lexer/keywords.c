// SPDX-License-Identifier: MIT
#include "keywords.h"
#include <string.h>

#define KEYWORD(word, token, reserved, where) {word, sizeof(word) - 1, token, reserved, where}

enum
{
    TOP = KEYWORD_AT_TOP_LEVEL,
    BODY = KEYWORD_IN_BODY,
    LOOP = KEYWORD_IN_LOOP,
    AFTER_IF = KEYWORD_AFTER_IF
};

// The words that the lexer and the parser read as the language's. The lexer gives a token of
// its own to the first ones; the parser reads the rest by their text where a construct allows
// them, and those that are not reserved can name a variable or a field.
static const ZenKeyword KEYWORDS[] = {
    // With a token of their own.
    KEYWORD("test", TOK_TEST, 1, TOP),
    KEYWORD("assert", TOK_ASSERT, 1, BODY),
    KEYWORD("expect", TOK_EXPECT, 0, BODY),
    KEYWORD("sizeof", TOK_SIZEOF, 1, BODY),
    KEYWORD("defer", TOK_DEFER, 1, BODY),
    KEYWORD("def", TOK_DEF, 1, TOP | BODY),
    KEYWORD("trait", TOK_TRAIT, 1, TOP),
    KEYWORD("impl", TOK_IMPL, 1, TOP),
    KEYWORD("alias", TOK_ALIAS, 1, TOP),
    KEYWORD("use", TOK_USE, 1, TOP),
    KEYWORD("comptime", TOK_COMPTIME, 1, TOP | BODY),
    KEYWORD("union", TOK_UNION, 1, TOP),
    KEYWORD("asm", TOK_ASM, 1, BODY),
    KEYWORD("volatile", TOK_VOLATILE, 1, BODY),
    KEYWORD("and", TOK_AND, 1, BODY),
    KEYWORD("or", TOK_OR, 1, BODY),
    KEYWORD("not", TOK_NOT, 0, BODY),
    KEYWORD("opaque", TOK_OPAQUE, 1, TOP),
    KEYWORD("do", TOK_DO, 0, BODY),

    // Read by their text, and reserved.
    KEYWORD("let", TOK_IDENT, 1, TOP | BODY),
    KEYWORD("static", TOK_IDENT, 1, TOP | BODY),
    KEYWORD("const", TOK_IDENT, 1, TOP | BODY),
    KEYWORD("return", TOK_IDENT, 1, BODY),
    KEYWORD("if", TOK_IDENT, 1, BODY),
    KEYWORD("else", TOK_IDENT, 1, AFTER_IF),
    KEYWORD("while", TOK_IDENT, 1, BODY),
    KEYWORD("break", TOK_IDENT, 1, LOOP),
    KEYWORD("continue", TOK_IDENT, 1, LOOP),
    KEYWORD("loop", TOK_IDENT, 1, BODY),
    KEYWORD("repeat", TOK_IDENT, 1, BODY),
    KEYWORD("unless", TOK_IDENT, 1, BODY),
    KEYWORD("guard", TOK_IDENT, 1, BODY),
    KEYWORD("launch", TOK_IDENT, 1, BODY),
    KEYWORD("goto", TOK_IDENT, 1, BODY),
    KEYWORD("plugin", TOK_IDENT, 1, TOP),
    KEYWORD("fn", TOK_IDENT, 1, TOP),
    KEYWORD("struct", TOK_IDENT, 1, TOP),
    KEYWORD("enum", TOK_IDENT, 1, TOP),
    KEYWORD("self", TOK_IDENT, 1, BODY),

    // Read by their text where a construct allows them; they can be names.
    KEYWORD("for", TOK_IDENT, 0, BODY),
    KEYWORD("match", TOK_IDENT, 0, BODY),
    KEYWORD("import", TOK_IDENT, 0, TOP),
    KEYWORD("include", TOK_IDENT, 0, TOP),
    KEYWORD("extern", TOK_IDENT, 0, TOP),
    KEYWORD("export", TOK_IDENT, 0, TOP),
    KEYWORD("inline", TOK_IDENT, 0, TOP),
    KEYWORD("async", TOK_IDENT, 0, TOP),
    KEYWORD("type", TOK_IDENT, 0, TOP),
    KEYWORD("raw", TOK_IDENT, 0, TOP | BODY),
    KEYWORD("embed", TOK_IDENT, 0, BODY),
    KEYWORD("await", TOK_IDENT, 0, BODY),
    KEYWORD("autofree", TOK_IDENT, 0, BODY),
    KEYWORD("typeof", TOK_IDENT, 0, BODY),
    KEYWORD("true", TOK_IDENT, 0, BODY),
    KEYWORD("false", TOK_IDENT, 0, BODY),
    KEYWORD("null", TOK_IDENT, 0, BODY),
    KEYWORD("thread_local", TOK_IDENT, 0, 0),
    KEYWORD("in", TOK_IDENT, 0, 0),
    KEYWORD("step", TOK_IDENT, 0, 0),
    KEYWORD("as", TOK_IDENT, 0, 0),
    KEYWORD("with", TOK_IDENT, 0, 0),
    KEYWORD("ref", TOK_IDENT, 0, 0),
};

#define KEYWORD_COUNT ((int)(sizeof(KEYWORDS) / sizeof(KEYWORDS[0])))

const ZenKeyword *zen_keywords(int *count)
{
    *count = KEYWORD_COUNT;
    return KEYWORDS;
}

const ZenKeyword *zen_keyword_find(const char *text, size_t len)
{
    for (int i = 0; i < KEYWORD_COUNT; i++)
    {
        if (KEYWORDS[i].len == len && memcmp(KEYWORDS[i].spelling, text, len) == 0)
        {
            return &KEYWORDS[i];
        }
    }
    return NULL;
}
