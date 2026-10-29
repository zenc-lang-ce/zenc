// SPDX-License-Identifier: MIT
// Standalone Zen C read-eval-print loop. Formerly `zc repl`.
#include "tool_common.h"
#include "../compiler.h"
#include "../repl/repl.h"
#include "../utils/cmd.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define REPL_OPT_COMMAND "-c"
#define REPL_OPT_HELP_SHORT "-h"
#define REPL_OPT_HELP "--help"

static void print_help(void)
{
    printf("usage: zc-repl [options]\n\n");
    printf("Start an interactive Read-Eval-Print Loop session.\n\n");
    printf("options:\n");
    print_help_item(REPL_OPT_COMMAND " <line>", "Run a single line and exit");
    print_help_item(REPL_OPT_HELP_SHORT ", " REPL_OPT_HELP, "Show this help");
}

int main(int argc, char **argv)
{
    ZenCompiler compiler = {0};
    z_compiler_setup(&compiler);

    char *cmd_to_run = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], REPL_OPT_COMMAND) == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "error: '" REPL_OPT_COMMAND "' needs a line to run\n");
                return 1;
            }
            cmd_to_run = argv[++i];
        }
        else if (strcmp(argv[i], REPL_OPT_HELP) == 0 || strcmp(argv[i], REPL_OPT_HELP_SHORT) == 0)
        {
            print_help();
            return 0;
        }
        else
        {
            fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
            fprintf(stderr, "Run 'zc-repl " REPL_OPT_HELP "' to see the options.\n");
            return 1;
        }
    }

    run_repl(&compiler, argv[0], cmd_to_run);
    return 0;
}
