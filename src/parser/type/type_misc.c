// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "ast/ast.h"
#include "analysis/move_check.h"
#include "plugins/plugin_manager.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analysis/const_fold.h"
#include "utils/utils.h"
#include "ast/primitives.h"

char *parse_type(ParserContext *ctx, Lexer *l)
{
    Type *t = parse_type_formal(ctx, l);
    if (!t)
    {
        return NULL;
    }

    return type_mangle_spelling(t);
}

ASTNode *parse_embed(ParserContext *ctx, Lexer *l)
{
    lexer_next(l);
    Token t = lexer_next(l);
    if (t.kind != TOK_STRING && t.kind != TOK_RAW_STRING)
    {
        zpanic_at(t, "String required");
        return NULL;
    }
    char *content = token_get_string_content(t);
    char fn[MAX_PATH_LEN];
    strncpy(fn, content, MAX_PATH_LEN - 1);
    fn[MAX_PATH_LEN - 1] = 0;
    zfree(content);

    // Check for optional "as Type"
    Type *target_type = NULL;
    if (lexer_peek(l).kind == TOK_IDENT && lexer_peek(l).len == 2 &&
        strncmp(lexer_peek(l).start, "as", 2) == 0)
    {
        lexer_next(l); // consume 'as'
        target_type = parse_type_formal(ctx, l);
    }

    // Without a type, the bytes are a `[char]`.
    if (!target_type)
    {
        target_type = instantiate_slice(ctx, type_new(TYPE_CHAR), t);
    }

    ASTNode *n = ast_create(NODE_EMBED);
    n->token = t;
    n->embed.path = xstrdup(fn);
    n->embed.as_type = target_type;
    n->embed.in_function = (ctx->current_scope != ctx->global_scope);
    // An array is written as a compound literal of its elements: the value is a pointer to them.
    n->type_info = (!slice_element(target_type) && target_type->kind == TYPE_ARRAY)
                       ? type_new_ptr(target_type->inner)
                       : target_type;

    // A file that cannot be read is an error; in fault-tolerant mode (the language server),
    // where zpanic_at returns, the embed stays, without bytes.
    FILE *f = fopen(fn, "rb");
    if (!f)
    {
        zpanic_at(t, "404: %s", fn);
        return n;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    unsigned char *b = len > 0 ? xmalloc((size_t)(len)) : NULL;
    if (len < 0 || (len > 0 && fread(b, 1, (size_t)(len), f) != (size_t)(len)))
    {
        fclose(f);
        zpanic_at(t, "cannot read %s", fn);
        return n;
    }
    fclose(f);

    n->embed.bytes = b;
    n->embed.len = (size_t)len;
    return n;
}
