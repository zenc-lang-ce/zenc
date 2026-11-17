// SPDX-License-Identifier: MIT
// Errors for syntax that Zen C does not have but that people write, coming
// from C, from other languages or from older versions of Zen C. Each one says
// what to write instead. The grammar calls them where it finds the token.
#include "parser.h"
#include "constants.h"
#include <stdio.h>
#include <string.h>

// Anything the top-level loop does not recognise is an error: skipping it
// silently made declarations vanish (`const X: T = v;`, `extern let x: T;`).
void report_unexpected_toplevel(Token t)
{
    const char *hints[] = {"Top-level declarations start with fn, struct, enum, union, "
                           "trait, impl, let, def, alias, import, extern, raw, test...",
                           NULL};
    char msg[MAX_SHORT_MSG_LEN];
    snprintf(msg, sizeof(msg), "Unexpected '%.*s' at top level", (int)t.len, t.start);
    zpanic_with_hints(t, msg, hints);
}

void report_toplevel_const(Lexer *l, Token t)
{
    const char *hints[] = {"For a compile-time constant use `def NAME = value;`",
                           "For a read-only global use `let NAME: const Type = value;`", NULL};
    zpanic_with_hints(t, "'const' cannot start a top-level declaration", hints);
    lexer_next(l); // fault-tolerant (LSP): resync on the next declaration
}

void report_local_const(Token t)
{
    const char *hints[] = {"For a compile-time constant use `def NAME = value;`",
                           "For a read-only variable use `let NAME: const Type = value;`", NULL};
    zpanic_with_hints(t, "'const' cannot start a variable declaration", hints);
}

// `extern enum Name;`, `extern const NAME;`...: no such declarations exist.
// Before v0.5.0 the missing ';' was skipped, so the keyword was taken as the
// C name and the real name was ignored.
int report_extern_with_kind(Lexer *l)
{
    Token kind = lexer_peek(l);
    // `const` and `def` name a value, which may also want a type.
    int is_value = is_token(kind, "const") || is_token(kind, "def");
    if (!is_value && !is_token(kind, "enum") && !is_token(kind, "type") && !is_token(kind, "alias"))
    {
        return 0;
    }

    lexer_next(l);
    Token name = lexer_peek(l);
    int name_len = name.kind == TOK_IDENT ? (int)name.len : 4;
    const char *name_text = name.kind == TOK_IDENT ? name.start : "NAME";
    char msg[MAX_SHORT_MSG_LEN];
    char use_hint[MAX_SHORT_MSG_LEN];
    char type_hint[MAX_SHORT_MSG_LEN];
    snprintf(msg, sizeof(msg), "'extern %.*s' is not a declaration", (int)kind.len, kind.start);
    snprintf(use_hint, sizeof(use_hint),
             "To use a name the C header declares, write `extern %.*s;` or remove the line",
             name_len, name_text);
    snprintf(type_hint, sizeof(type_hint),
             "To give a C global a type, write `extern let %.*s: Type;`", name_len, name_text);
    const char *hints[] = {use_hint, is_value ? type_hint : NULL, NULL};
    zpanic_with_hints(kind, msg, hints);

    // Fault-tolerant (LSP): skip the rest of the declaration.
    while (lexer_peek(l).kind != TOK_SEMICOLON && lexer_peek(l).kind != TOK_EOF)
    {
        lexer_next(l);
    }
    if (lexer_peek(l).kind == TOK_SEMICOLON)
    {
        lexer_next(l);
    }
    return 1;
}

// Anything that does not start a field is an error. Skipping it silently read
// the members of an anonymous `union { }` as fields of the struct and took its
// `}` as the end of the struct.
void report_not_a_field(Lexer *l, Token t)
{
    int is_union = t.kind == TOK_UNION || is_token(t, "union");
    if ((is_union || is_token(t, "struct")) && lexer_peek2(l).kind == TOK_LBRACE)
    {
        const char *hints[] = {is_union ? "Declare a named union and use it as a field"
                                        : "Declare a named struct and use it as a field",
                               NULL};
        zpanic_with_hints(t,
                          is_union ? "Anonymous unions are not supported inside a struct"
                                   : "Anonymous structs are not supported inside a struct",
                          hints);
        lexer_next(l);
        skip_braced_block(l); // fault-tolerant (LSP): go on after the block
        return;
    }

    const char *hints[] = {"A field is written `name: Type;`", NULL};
    char msg[MAX_SHORT_MSG_LEN];
    snprintf(msg, sizeof(msg), "Unexpected '%.*s' in a struct body", (int)t.len, t.start);
    zpanic_with_hints(t, msg, hints);
    lexer_next(l); // fault-tolerant (LSP)
}
