// SPDX-License-Identifier: MIT
#include "unit_parse.h"
#include "diagnostics/diagnostics.h"
#include <stdio.h>
#include <string.h>

static ZenCompiler compiler;
static ParserContext ctx;

static ASTNode *parse_with(const char *src, int keep_comments)
{
    memset(&compiler, 0, sizeof(compiler));
    compiler.config.quiet = 1;
    compiler.config.keep_comments = keep_comments;
    parser_context_init(&ctx, &compiler, "test.zc");
    ctx.is_fault_tolerant = 1;
    ctx.cg.hoist_out = tmpfile();
    register_builtins(&ctx);

    Lexer l;
    lexer_init(&l, src, ctx.config, ctx.current_filename);
    ASTNode *root = parse_program(&ctx, &l);

    token_set_parser_ctx(NULL);
    diag_set_parser_ctx(NULL);
    if (ctx.cg.hoist_out)
    {
        fclose(ctx.cg.hoist_out);
    }
    return root;
}

ASTNode *parse_source(const char *src)
{
    return parse_with(src, 0);
}

ASTNode *parse_source_keeping_comments(const char *src)
{
    return parse_with(src, 1);
}

ParserContext *parse_source_context(void)
{
    return &ctx;
}
