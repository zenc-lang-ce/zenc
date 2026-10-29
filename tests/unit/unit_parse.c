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
    memset(&ctx, 0, sizeof(ctx));
    compiler.config.quiet = 1;
    compiler.config.keep_comments = keep_comments;
    ctx.compiler = &compiler;
    ctx.config = &compiler.config;
    ctx.current_filename = "test.zc";
    ctx.is_fault_tolerant = 1;
    module_state_init(&ctx.imports);
    ctx.cg.hoist_out = tmpfile();
    token_set_parser_ctx(&ctx);
    diag_set_parser_ctx(&ctx);
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
