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

    Type *void_t = type_new(TYPE_VOID);
    add_symbol(ctx, "free", void_t, 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;

    add_symbol(ctx, "strdup", type_new(TYPE_STRING), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "malloc", type_new_ptr(void_t), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "realloc", type_new_ptr(void_t), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "calloc", type_new_ptr(void_t), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "puts", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "printf", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "strcmp", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "strlen", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "strcpy", type_new(TYPE_STRING), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "strcat", type_new(TYPE_STRING), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "memset", type_new_ptr(void_t), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "memcpy", type_new_ptr(void_t), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "exit", void_t, 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;

    add_symbol(ctx, "stdin", type_new_ptr(void_t), 0);
    add_symbol(ctx, "stdout", type_new_ptr(void_t), 0);
    add_symbol(ctx, "stderr", type_new_ptr(void_t), 0);

    add_symbol(ctx, "fopen", type_new_ptr(void_t), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "fclose", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "fread", type_new(TYPE_USIZE), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "fwrite", type_new(TYPE_USIZE), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "fseek", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "ftell", type_new(TYPE_I64), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "rewind", void_t, 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "fprintf", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "vprintf", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "vfprintf", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "sprintf", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "vsnprintf", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "snprintf", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "feof", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "ferror", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "mkdir", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "rmdir", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "chdir", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "getcwd", type_new(TYPE_STRING), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "system", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "getenv", type_new(TYPE_STRING), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "fgets", type_new(TYPE_STRING), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;
    add_symbol(ctx, "usleep", type_new(TYPE_INT), 0);
    ctx->current_scope->symbols->kind = SYM_FUNCTION;

    ASTNode *va_def = ast_create(NODE_STRUCT);
    va_def->strct.name = xstrdup("va_list");
    register_struct_def(ctx, "va_list", va_def);
    register_impl(ctx, "Copy", "va_list");

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
