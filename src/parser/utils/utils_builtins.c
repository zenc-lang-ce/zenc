// SPDX-License-Identifier: MIT
#include "plugins/plugin_manager.h"
#include "parser.h"
#include "utils/format_expr.h"
#include "utils/colors.h"
#include "utils/utils.h"
#include "constants.h"
#include "ast/primitives.h"
#include <ctype.h>
#include "analysis/const_fold.h"

void register_builtins(ParserContext *ctx)
{
    // Built-ins are not identifiers of the program: MISRA's rule 5.10 (no identifier named like
    // a standard macro) must not reject them. (`true` and `false` are literals, LITERAL_BOOL.)
    void (*check_standard_macro_name)(Token tok, const char *name) =
        ctx->hook_check_standard_macro_name;
    ctx->hook_check_standard_macro_name = NULL;

    ASTNode *va_def = ast_create(NODE_STRUCT);
    va_def->strct.name = xstrdup("va_list");
    register_struct_def(ctx, "va_list", va_def);
    register_impl(ctx, "Copy", "va_list");

    // A slice, `[T]`, is an instance of this template; std/slice.zc adds its methods.
    Lexer slice_lexer;
#if FF_COMPATIBILITY_0_5
    const char *slice_template = "struct Slice<T> { data: T*; len: usize; cap: usize; }";
#else
    const char *slice_template = "struct Slice<T> { data: T*; len: usize; }";
#endif
    lexer_init(&slice_lexer, slice_template, ctx->config, "<builtin>");
    parse_program_nodes(ctx, &slice_lexer);

    ctx->hook_check_standard_macro_name = check_standard_macro_name;
}
