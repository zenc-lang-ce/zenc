// SPDX-License-Identifier: MIT
// Standalone Zen C documentation generator. Formerly `zc doc`.
#include "tool_common.h"
#include "../compiler.h"
#include "../parser/parser.h"
#include "../token.h"
#include "../diagnostics/diagnostics.h"
#include "../utils/utils.h"
#include "../utils/cmd.h"
#include "../zen/zen_doc.h"
#include <stdio.h>
#include <string.h>

#define DOC_OPT_VERBOSE_SHORT "-v"
#define DOC_OPT_VERBOSE "--verbose"
#define DOC_OPT_RECURSIVE "--recursive-doc"
#define DOC_OPT_NORECURSIVE "--no-recursive-doc"
#define DOC_OPT_HELP_SHORT "-h"
#define DOC_OPT_HELP "--help"

static void print_help(void)
{
    printf("usage: zc-doc [options] <file>\n\n");
    printf("Generate Markdown documentation for a Zen C source file and its imports.\n\n");
    printf("options:\n");
    print_help_item(DOC_OPT_RECURSIVE, "Traverse and document imported modules (default)");
    print_help_item(DOC_OPT_NORECURSIVE, "Only document the primary input file");
    print_help_item(DOC_OPT_VERBOSE_SHORT ", " DOC_OPT_VERBOSE,
                    "Show all granular compilation phases");
    print_help_item(DOC_OPT_HELP_SHORT ", " DOC_OPT_HELP, "Show this help");
}

int main(int argc, char **argv)
{
    ZenCompiler compiler = {0};
    CompilerConfig *cfg = &compiler.config;

    z_compiler_setup(&compiler);

    cfg->mode_doc = 1;
    cfg->keep_comments = 1;
    cfg->recursive_doc = 1;
    cfg->mode_check = 0; // Documentation does not require typechecking
    cfg->use_typecheck = 0;

    const char *input = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], DOC_OPT_RECURSIVE) == 0)
        {
            cfg->recursive_doc = 1;
        }
        else if (strcmp(argv[i], DOC_OPT_NORECURSIVE) == 0)
        {
            cfg->recursive_doc = 0;
        }
        else if (strcmp(argv[i], DOC_OPT_VERBOSE) == 0 ||
                 strcmp(argv[i], DOC_OPT_VERBOSE_SHORT) == 0)
        {
            cfg->verbose = 1;
        }
        else if (strcmp(argv[i], DOC_OPT_HELP) == 0 || strcmp(argv[i], DOC_OPT_HELP_SHORT) == 0)
        {
            print_help();
            return 0;
        }
        else if (argv[i][0] != '-' && !input)
        {
            input = argv[i];
        }
        else
        {
            fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (!input)
    {
        fprintf(stderr, "error: no input file specified\n");
        fprintf(stderr, "usage: zc-doc [options] <file>\n");
        fprintf(stderr, "Run 'zc-doc " DOC_OPT_HELP "' to see the options.\n");
        return 1;
    }

    cfg->input_file = (char *)input;

    ParserContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.compiler = &compiler;
    ctx.config = cfg;
    ctx.current_filename = cfg->input_file;
    module_state_init(&ctx.imports);
    diag_set_parser_ctx(&ctx);
    token_set_parser_ctx(&ctx);

    char *src = load_file(cfg->input_file, ctx.current_filename, ctx.config);
    if (!src)
    {
        fprintf(stderr, "error: could not read file '%s'\n", cfg->input_file);
        return 1;
    }

    scan_build_directives(&ctx, src);

    Lexer l;
    lexer_init(&l, src, ctx.config, ctx.current_filename);

    ASTNode *root = parse_program(&ctx, &l);
    if (!root)
    {
        return 1;
    }

    generate_docs(&ctx, root);
    return 0;
}
