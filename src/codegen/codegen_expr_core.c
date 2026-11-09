// SPDX-License-Identifier: MIT
#include "../parser/parser.h"

#include "codegen.h"
#include "zprep.h"
#include "../constants.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../plugins/plugin_manager.h"
#include "ast.h"
#include "zprep_plugin.h"
#include "codegen_internal.h"

const char *get_missing_function_hint(ParserContext *ctx, const char *name)
{
    if (strcmp(name, "malloc") == 0 || strcmp(name, "free") == 0 || strcmp(name, "calloc") == 0 ||
        strcmp(name, "realloc") == 0)
    {
        return "Include <stdlib.h> or use 'use std::mem'";
    }
    if (strcmp(name, "printf") == 0 || strcmp(name, "scanf") == 0 || strcmp(name, "fprintf") == 0 ||
        strcmp(name, "sprintf") == 0 || strcmp(name, "snprintf") == 0)
    {
        return "Include <stdio.h> or use 'use std::io'";
    }
    if (strcmp(name, "memset") == 0 || strcmp(name, "memcpy") == 0 || strcmp(name, "strlen") == 0 ||
        strcmp(name, "strcpy") == 0 || strcmp(name, "strcmp") == 0 || strcmp(name, "strncmp") == 0)
    {
        return "Include <string.h>";
    }

    int best_dist = 4;
    static char best_buf[MAX_SHORT_MSG_LEN];
    const char *best = NULL;

    FuncSig *sig = ctx->func_registry;
    while (sig)
    {
        int dist = levenshtein(name, sig->name);
        if (dist < best_dist)
        {
            best_dist = dist;
            best = sig->name;
        }
        sig = sig->next;
    }

    StructRef *ref = ctx->parsed_funcs_list;
    while (ref)
    {
        if (ref->node && ref->node->kind == NODE_FUNCTION)
        {
            int dist = levenshtein(name, ref->node->func.name);
            if (dist < best_dist)
            {
                best_dist = dist;
                best = ref->node->func.name;
            }
        }
        ref = ref->next;
    }

    if (best)
    {
        snprintf(best_buf, sizeof(best_buf), "Did you mean '%s'?", best);
        return best_buf;
    }

    return NULL;
}

// Emit literal expression (int, float, string, char)
static void codegen_literal_expr(ParserContext *ctx, ASTNode *node)
{
    if (node->literal.kind == LITERAL_STRING || node->literal.kind == LITERAL_RAW_STRING)
    {
        EMIT(ctx, "\"");
        for (int i = 0; node->literal.string_val[i]; i++)
        {
            char c = node->literal.string_val[i];
            if (node->literal.kind == LITERAL_RAW_STRING)
            {
                if (c == '\\')
                {
                    EMIT(ctx, "\\\\");
                }
                else if (c == '"')
                {
                    EMIT(ctx, "\\\"");
                }
                else if (c == '\n')
                {
                    EMIT(ctx, "\\n");
                }
                else if (c == '\r')
                {
                    EMIT(ctx, "\\r");
                }
                else if (c == '\t')
                {
                    EMIT(ctx, "\\t");
                }
                else
                {
                    EMIT(ctx, "%c", c);
                }
            }
            else
            {
                EMIT(ctx, "%c", c);
            }
        }
        EMIT(ctx, "\"");
    }
    else if (node->literal.kind == LITERAL_CHAR)
    {
        if (node->literal.int_val > 127)
        {
            EMIT(ctx, "%u", (unsigned int)node->literal.int_val);
        }
        else
        {
            EMIT(ctx, "%s", node->literal.string_val);
        }
    }
    else if (node->literal.kind == LITERAL_FLOAT)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.17g", node->literal.float_val);
        if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E'))
        {
            strcat(buf, ".0");
        }
        EMIT(ctx, "%s", buf);
    }
    else // LITERAL_INT
    {
        if (node->literal.int_val > 9223372036854775807ULL)
        {
            EMIT(ctx, "%lluULL", (unsigned long long)node->literal.int_val);
        }
        else
        {
            EMIT(ctx, "%lld", (long long)node->literal.int_val);
        }
    }
}

#if FF_COMPATIBILITY_0_5
// 0.5.1 read `Enum_Variant` as `Enum::Variant`, and `Enum_Variant_Tag` as its tag. Writes
// that C name and returns 1 when `name` is one of them: the part before the first `_` is an
// enum and the rest is one of its variants, or a variant and `_Tag`. Any other name with a
// `_` (`let Color_max`) is its own.
int emit_legacy_variant_name(ParserContext *ctx, const char *name)
{
    const char *underscore = strchr(name, '_');
    if (!underscore || underscore == name || strstr(name, "__"))
    {
        return 0;
    }
    char *enum_name = xstrdup(name);
    enum_name[underscore - name] = 0;
    ASTNode *def = find_struct_def(ctx, enum_name);
    if (!def || def->kind != NODE_ENUM)
    {
        return 0;
    }
    const char *rest = underscore + 1;
    size_t rest_len = strlen(rest);
    for (ASTNode *v = def->enm.variants; v; v = v->next)
    {
        size_t vlen = strlen(v->variant.name);
        if (strncmp(rest, v->variant.name, vlen) == 0 &&
            (rest[vlen] == 0 || (rest_len == vlen + 4 && strcmp(rest + vlen, "_Tag") == 0)))
        {
            emit_mangled_name(ctx, enum_name, rest);
            return 1;
        }
    }
    return 0;
}
#endif

// Emit variable reference expression
static void codegen_var_expr(ParserContext *ctx, ASTNode *node)
{
    if (ctx->cg.current_lambda)
    {
        for (int i = 0; i < ctx->cg.current_lambda->lambda.capture_count; i++)
        {
            if (strcmp(node->var_ref.name, ctx->cg.current_lambda->lambda.captured_vars[i]) == 0)
            {
                if (ctx->cg.current_lambda->lambda.capture_modes &&
                    ctx->cg.current_lambda->lambda.capture_modes[i] == 1)
                {
                    EMIT(ctx, "(*ctx->%s)", c_name(ctx, node->var_ref.name));
                }
                else
                {
                    EMIT(ctx, "ctx->%s", c_name(ctx, node->var_ref.name));
                }
                return;
            }
        }
    }

    // The parser leaves a suggestion only on a name it could not resolve.
    if (node->var_ref.suggestion && !ctx->silent_warnings && !find_func(ctx, node->var_ref.name))
    {
        char msg[MAX_SHORT_MSG_LEN];
        char help[MAX_SHORT_MSG_LEN];
        snprintf(msg, sizeof(msg), "Undefined variable '%s'", node->var_ref.name);
        snprintf(help, sizeof(help), "Did you mean '%s'?", node->var_ref.suggestion);
        zwarn_at(node->token, "%s\n   = help: %s", msg, help);
    }

    const char *name = node->var_ref.name;
    const TypeMember *variant = &node->var_ref.variant;
    const char *enum_c_name =
        variant->member ? link_name_of(ctx, type_def_name(variant->owner)) : NULL;
    if (enum_c_name)
    {
        // A variant of an enum with @link_name: C names it after the enum's C name.
        EMIT(ctx, "%s", mangle_variant_symbol(enum_c_name, variant->member));
    }
    else if (strcmp(name, "self") == 0 && node->type_info && node->type_info->kind == TYPE_STRUCT)
    {
        EMIT(ctx, "(*self)");
        return;
    }
#if FF_COMPATIBILITY_0_5
    else if (emit_legacy_variant_name(ctx, name))
    {
        return;
    }
#endif
    else
    {
        // The name the parser resolved: a local, a function, `Type__method`, `Enum__Variant`.
        ZenSymbol *sym = find_symbol_in_all(ctx, name);
        EMIT(ctx, "%s", sym && sym->link_name ? sym->link_name : c_name(ctx, name));
    }

    // A variant without a payload is a call to its constructor, unless the parent
    // NODE_EXPR_CALL adds the ().
    if (!g_emitting_callee && variant->member &&
        find_enum_variant_of(ctx, type_def_name(variant->owner), variant->member))
    {
        EMIT(ctx, "()");
    }
}

// Emit lambda expression
static void codegen_lambda_expr(ParserContext *ctx, ASTNode *node)
{
    if (node->lambda.is_bare)
    {
        EMIT(ctx, "_lambda_%d", node->lambda.lambda_id);
        return;
    }

    if (node->lambda.capture_count > 0)
    {
        int lid = node->lambda.lambda_id;
        if (ctx->config->use_cpp)
        {
            EMIT(ctx,
                 "({ struct Lambda_%d_Ctx *_z_ctx_%d = (struct Lambda_%d_Ctx*)malloc(sizeof(struct "
                 "Lambda_%d_Ctx));\n",
                 lid, lid, lid, lid);
        }
        else
        {
            EMIT(ctx,
                 "({ struct Lambda_%d_Ctx *_z_ctx_%d = malloc(sizeof(struct Lambda_%d_Ctx));\n",
                 lid, lid, lid);
        }
        for (int i = 0; i < node->lambda.capture_count; i++)
        {
            if (node->lambda.capture_modes && node->lambda.capture_modes[i] == 1)
            {
                int found = 0;
                if (ctx->cg.current_lambda)
                {
                    for (int k = 0; k < ctx->cg.current_lambda->lambda.capture_count; k++)
                    {
                        if (strcmp(node->lambda.captured_vars[i],
                                   ctx->cg.current_lambda->lambda.captured_vars[k]) == 0)
                        {
                            if (ctx->cg.current_lambda->lambda.capture_modes &&
                                ctx->cg.current_lambda->lambda.capture_modes[k] == 1)
                            {
                                const char *cap = c_name(ctx, node->lambda.captured_vars[i]);
                                EMIT(ctx, "_z_ctx_%d->%s = ctx->%s;\n", lid, cap, cap);
                            }
                            else
                            {
                                const char *cap = c_name(ctx, node->lambda.captured_vars[i]);
                                EMIT(ctx, "_z_ctx_%d->%s = &ctx->%s;\n", lid, cap, cap);
                            }
                            found = 1;
                            break;
                        }
                    }
                }
                if (!found)
                {
                    const char *cap = c_name(ctx, node->lambda.captured_vars[i]);
                    EMIT(ctx, "_z_ctx_%d->%s = &%s;\n", lid, cap, cap);
                }
            }
            else
            {
                char *tstr = capture_c_type(ctx, node, i);

                const char *cap_name = c_name(ctx, node->lambda.captured_vars[i]);
                int is_array = !strstr(tstr, "(*") && strchr(tstr, '[');
                if (strstr(tstr, "(*"))
                {
                    // A function pointer type cannot be spelled as a cast target by
                    // appending '*'; assign the field directly.
                    EMIT(ctx, "_z_ctx_%d->%s = ", lid, cap_name);
                }
                else if (is_array)
                {
                    // An array cannot be assigned in C: copy its bytes.
                    EMIT(ctx, "memcpy(&_z_ctx_%d->%s, &", lid, cap_name);
                }
                else
                {
                    EMIT(ctx, "*(%s*)(&_z_ctx_%d->%s) = ", tstr, lid, cap_name);
                }
                zfree(tstr);

                ASTNode *var_node = ast_create(NODE_EXPR_VAR);
                var_node->var_ref.name = xstrdup(node->lambda.captured_vars[i]);
                var_node->token = node->token;

                // A capture with no type is an `int`, as its context field is.
                Type *captured = node->lambda.captured_types_info[i];
                var_node->type_info = captured ? captured : type_new(TYPE_C_INT);

                codegen_expression_with_move(ctx, var_node);

                ast_free(var_node);

                if (is_array)
                {
                    EMIT(ctx, ", sizeof(_z_ctx_%d->%s));\n", lid, cap_name);
                }
                else
                {
                    EMIT(ctx, ";\n");
                }

                if (capture_needs_drop(ctx, node, i))
                {
                    EMIT(ctx, "_z_ctx_%d->__z_drop_flag_%s = 1;\n", lid,
                         node->lambda.captured_vars[i]);
                }
            }
        }
        if (ctx->config->use_cpp)
        {
            EMIT(ctx, "z_closure_T _cl = {(void*)_lambda_%d, _z_ctx_%d, _lambda_%d_drop}; _cl; })",
                 lid, lid, lid);
        }
        else
        {
            EMIT(ctx,
                 "(z_closure_T){.func = _lambda_%d, .ctx = _z_ctx_%d, .drop = _lambda_%d_drop}; })",
                 lid, lid, lid);
        }
    }
    else
    {
        if (ctx->config->use_cpp)
        {
            EMIT(ctx, "(z_closure_T){ (void*)_lambda_%d, NULL, NULL }", node->lambda.lambda_id);
        }
        else
        {
            EMIT(ctx, "((z_closure_T){.func = (void*)_lambda_%d, .ctx = NULL, .drop = NULL})",
                 node->lambda.lambda_id);
        }
    }
}

void handle_expr_match(ParserContext *ctx, ASTNode *node)
{
    codegen_match_internal(ctx, node, 1);
}

void handle_expr_var(ParserContext *ctx, ASTNode *node)
{
    codegen_var_expr(ctx, node);
}

void handle_lambda(ParserContext *ctx, ASTNode *node)
{
    codegen_lambda_expr(ctx, node);
}

void handle_expr_literal(ParserContext *ctx, ASTNode *node)
{
    codegen_literal_expr(ctx, node);
}

void emit_raw_with_captures(ParserContext *ctx, const char *content)
{
    // The emitter writes a pending indentation even for an empty string, which would leave
    // spaces on an otherwise empty line: emit nothing for nothing.
    if (!ctx->cg.current_lambda)
    {
        if (*content)
        {
            EMIT(ctx, "%s", content);
        }
        return;
    }

    Lexer l;
    lexer_init(&l, content, ctx->config, ctx->current_filename);
    Token t;
    int last_pos = 0;
    while ((t = lexer_next(&l)).kind != TOK_EOF)
    {
        int current_tok_start = (int)(t.start - content);
        for (int i = last_pos; i < current_tok_start; i++)
        {
            EMIT(ctx, "%c", content[i]);
        }

        if (t.kind == TOK_IDENT)
        {
            char *name = token_strdup(t);
            int captured = -1;
            if (ctx->cg.current_lambda->lambda.captured_vars)
            {
                for (int i = 0; i < ctx->cg.current_lambda->lambda.capture_count; i++)
                {
                    if (strcmp(name, ctx->cg.current_lambda->lambda.captured_vars[i]) == 0)
                    {
                        captured = i;
                        break;
                    }
                }
            }

            if (captured != -1)
            {
                if (ctx->cg.current_lambda->lambda.capture_modes &&
                    ctx->cg.current_lambda->lambda.capture_modes[captured] == 1)
                {
                    EMIT(ctx, "(*ctx->%s)", name);
                }
                else
                {
                    EMIT(ctx, "ctx->%s", name);
                }
            }
            else
            {
                EMIT(ctx, "%.*s", (int)(t.len), t.start);
            }
            zfree(name);
        }
        else
        {
            EMIT(ctx, "%.*s", (int)(t.len), t.start);
        }
        last_pos = current_tok_start + (int)(t.len);
    }
    if (content[last_pos])
    {
        EMIT(ctx, "%s", content + last_pos);
    }
}

void handle_raw_stmt(ParserContext *ctx, ASTNode *node)
{
    emit_raw_with_captures(ctx, node->raw_stmt.content);
}

void handle_ast_comment(ParserContext *ctx, ASTNode *node)
{
    EMIT(ctx, "%s\n", node->comment.content);
}

// Error-recovered node from fault-tolerant (LSP/REPL) parsing: emit a C
// comment so the recovered source is visibly incomplete rather than silently
// producing wrong output.
void handle_erroneous(ParserContext *ctx, ASTNode *node)
{
    (void)node;
    EMIT(ctx, "/* <error-recovered> */\n");
}

void handle_ternary(ParserContext *ctx, ASTNode *node)
{
    EMIT(ctx, "((");
    codegen_expression(ctx, node->ternary.cond);
    EMIT(ctx, ") ? (");
    codegen_expression(ctx, node->ternary.true_expr);
    EMIT(ctx, ") : (");
    codegen_expression(ctx, node->ternary.false_expr);
    EMIT(ctx, "))");
}

void handle_va_start(ParserContext *ctx, ASTNode *node)
{
    EMIT(ctx, "va_start(");
    codegen_expression(ctx, node->va_start_args.ap);
    EMIT(ctx, ", ");
    codegen_expression(ctx, node->va_start_args.last_arg);
    EMIT(ctx, ")");
}

void handle_va_end(ParserContext *ctx, ASTNode *node)
{
    EMIT(ctx, "va_end_args(");
    codegen_expression(ctx, node->va_end_args.ap);
    EMIT(ctx, ")");
}

void handle_va_copy(ParserContext *ctx, ASTNode *node)
{
    EMIT(ctx, "va_copy(");
    codegen_expression(ctx, node->va_copy_args.dest);
    EMIT(ctx, ", ");
    codegen_expression(ctx, node->va_copy_args.src);
    EMIT(ctx, ")");
}

void handle_va_arg(ParserContext *ctx, ASTNode *node)
{
    char *type_str = c_type_spelling(ctx, node->va_arg_val.type_info);
    EMIT(ctx, "va_arg(");
    codegen_expression(ctx, node->va_arg_val.ap);
    EMIT(ctx, ", %s)", type_str);
    zfree(type_str);
}
