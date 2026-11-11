// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "lexer/keywords.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef KEYWORDS_H
#define KEYWORDS_H

#include "../token.h"
#include <stddef.h>

/** Where a keyword starts a construct: a set of bits, read by completion. */
typedef enum
{
    KEYWORD_AT_TOP_LEVEL = 1 << 0, ///< A declaration of the file: `fn`, `struct`, `import`.
    KEYWORD_IN_BODY = 1 << 1,      ///< A statement or a value in a body: `let`, `if`, `true`.
    KEYWORD_IN_LOOP = 1 << 2,      ///< Only in the body of a loop: `break`, `continue`.
    KEYWORD_AFTER_IF = 1 << 3      ///< Only after the body of an `if`: `else`.
} KeywordPlace;

/** A word of the language that is not a name of the program. */
typedef struct
{
    const char *spelling;
    size_t len;
    /// The token the lexer gives it, or TOK_IDENT for a word the parser reads by its text where a
    /// construct allows it.
    ZenTokenType kind;
    int is_reserved;     ///< It cannot name anything (`let let = 1;`): see is_reserved_keyword.
    unsigned int places; ///< KeywordPlace bits; 0 for a word inside a construct (`in`, `as`).
} ZenKeyword;

/** The keywords of the language; their number goes to *count. */
const ZenKeyword *zen_keywords(int *count);

/** The keyword written as the `len` bytes of `text`, or NULL. */
const ZenKeyword *zen_keyword_find(const char *text, size_t len);

#endif
