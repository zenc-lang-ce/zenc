// SPDX-License-Identifier: MIT
// Unit tests of audit_section_5 (src/parser/utils/utils_audit.c) outside MISRA mode: a new
// name warns once for each symbol of that name in the scopes around its own.
#include "diagnostics/diagnostics.h"
#include "parser/parser.h"
#include <stdio.h>
#include <time.h>
#include "unit_test.h"

static void count_warning(void *data, const ZenDiagnostic *d)
{
    if (!d->is_error)
    {
        (*(int *)data)++;
    }
}

// A parser context with the shadowing warning on, whose warnings are counted in *warnings, with
// a global scope.
static ParserContext *context_counting_warnings(ZenCompiler *compiler, ParserContext *ctx,
                                                int *warnings)
{
    compiler->config.diag_mask |= (uint64_t)1 << DIAG_STYLE_SHADOWING;
    compiler->diag_consumer = count_warning;
    compiler->diag_consumer_data = warnings;
    ctx->compiler = compiler;
    ctx->config = &compiler->config;
    ctx->global_scope = symbol_scope_create(NULL, NULL);
    diag_set_parser_ctx(ctx);
    return ctx;
}

UTEST(audit_section_5, warns_for_each_symbol_of_the_name_in_the_scopes_around)
{
    ZenCompiler compiler = {0};
    ParserContext ctx = {0};
    int warnings = 0;
    context_counting_warnings(&compiler, &ctx, &warnings);
    symbol_add(ctx.global_scope, "x", SYM_VARIABLE);
    symbol_add(ctx.global_scope, "x", SYM_FUNCTION);
    symbol_add(ctx.global_scope, "y", SYM_VARIABLE);
    Scope *block = symbol_scope_create(ctx.global_scope, NULL);
    symbol_add(block, "z", SYM_VARIABLE);

    audit_section_5(&ctx, block, "x", NULL, TOKEN_UNKNOWN);
    EXPECT_EQ(2, warnings);

    warnings = 0;
    audit_section_5(&ctx, block, "z", NULL, TOKEN_UNKNOWN);
    audit_section_5(&ctx, block, "new_name", NULL, TOKEN_UNKNOWN);
    EXPECT_EQ(0, warnings);
    diag_set_parser_ctx(NULL);
}

// Every declaration is audited, and the global scope holds every symbol of a program and its
// imports: auditing must not walk them all. 30,000 audits against 30,000 global symbols take
// milliseconds when each costs the same, and seconds when each walks the scope.
UTEST(audit_section_5, auditing_takes_constant_time)
{
    ZenCompiler compiler = {0};
    ParserContext ctx = {0};
    int warnings = 0;
    context_counting_warnings(&compiler, &ctx, &warnings);
    enum
    {
        SYMBOL_COUNT = 30000
    };
    char name[32];
    for (int i = 0; i < SYMBOL_COUNT; i++)
    {
        snprintf(name, sizeof(name), "global_%d", i);
        symbol_add(ctx.global_scope, name, SYM_VARIABLE);
    }
    Scope *block = symbol_scope_create(ctx.global_scope, NULL);
    clock_t start = clock();
    for (int i = 0; i < SYMBOL_COUNT; i++)
    {
        snprintf(name, sizeof(name), "local_%d", i);
        audit_section_5(&ctx, block, name, NULL, TOKEN_UNKNOWN);
    }
    double seconds = (double)(clock() - start) / CLOCKS_PER_SEC;
    EXPECT_EQ(0, warnings);
    EXPECT_LT(seconds, 1.0);
    diag_set_parser_ctx(NULL);
}
