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
    // a standard macro) must not reject `true` and `false`.
    void (*check_standard_macro_name)(Token tok, const char *name) =
        ctx->hook_check_standard_macro_name;
    ctx->hook_check_standard_macro_name = NULL;

    Type *t = type_new(TYPE_BOOL);
    t->is_const = 1;
    add_symbol(ctx, "true", t, 0);

    t = type_new(TYPE_BOOL);
    t->is_const = 1;
    add_symbol(ctx, "false", t, 0);

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

void register_comptime_builtins(ParserContext *ctx)
{
    Type *void_t = type_new(TYPE_VOID);
    add_symbol(ctx, "yield", void_t, 0);
    add_symbol(ctx, "code", void_t, 0);
    add_symbol(ctx, "compile_warn", void_t, 0);
    add_symbol(ctx, "compile_error", void_t, 0);

    Type *string_t = type_new(TYPE_STRING);
    add_symbol(ctx, "__COMPTIME_TARGET__", string_t, 0);
    add_symbol(ctx, "__COMPTIME_FILE__", string_t, 0);

    register_extern_symbol(ctx, "yield");
    register_extern_symbol(ctx, "code");
    register_extern_symbol(ctx, "compile_warn");
    register_extern_symbol(ctx, "compile_error");
}
