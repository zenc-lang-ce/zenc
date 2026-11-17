// SPDX-License-Identifier: MIT
#include "../parser/parser.h"

#include "codegen.h"
#include "codegen_internal.h"
#include "zprep.h"
#include "../constants.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../plugins/plugin_manager.h"
#include "ast.h"
#include "codegen_internal.h"
#include "zprep_plugin.h"

// Enters a loop: emits its label, if any, and records where its defers start, so `break` and
// `continue` know how many to run.
void codegen_loop_enter(ParserContext *ctx, const char *label)
{
    if (label)
    {
        EMIT(ctx, "%s:;\n", label);
    }
    if (ctx->cg.loop_depth >= ctx->cg.loop_defer_capacity)
    {
        int capacity = ctx->cg.loop_defer_capacity ? ctx->cg.loop_defer_capacity * 2 : 16;
        ctx->cg.loop_defer_boundary =
            xrealloc(ctx->cg.loop_defer_boundary, sizeof(int) * (size_t)capacity);
        ctx->cg.loop_defer_capacity = capacity;
    }
    ctx->cg.loop_defer_boundary[ctx->cg.loop_depth] = ctx->cg.defer_count;
    ctx->cg.loop_depth++;
}

// Emits a loop body. A labelled loop gets a block that ends in its `continue` target. With
// `misra_wrap`, an unlabelled body that is not a block is braced in MISRA mode (rule 15.6).
void codegen_loop_body(ParserContext *ctx, ASTNode *body, const char *label, int misra_wrap)
{
    if (label)
    {
        EMIT(ctx, "{\n");
        emitter_indent(&ctx->cg.emitter);
        codegen_node_single(ctx, body);
        emitter_dedent(&ctx->cg.emitter);
        EMIT(ctx, "__continue_%s:;\n", label);
        EMIT(ctx, "}\n");
        return;
    }

    int wrap = misra_wrap && ctx->config->misra_mode && body->kind != NODE_BLOCK;
    if (wrap)
    {
        EMIT(ctx, "{\n");
        emitter_indent(&ctx->cg.emitter);
    }
    codegen_node_single(ctx, body);
    if (wrap)
    {
        emitter_dedent(&ctx->cg.emitter);
        EMIT(ctx, "\n}");
    }
}

// Leaves a loop entered with codegen_loop_enter: emits its `break` target, if labelled.
void codegen_loop_exit(ParserContext *ctx, const char *label)
{
    ctx->cg.loop_depth--;
    if (label)
    {
        EMIT(ctx, "__break_%s:;\n", label);
    }
}

// Emits the pending defers, innermost first, down to stack index `boundary`.
void emit_defers_down_to(ParserContext *ctx, int boundary)
{
    for (int i = ctx->cg.defer_count - 1; i >= boundary; i--)
    {
        emit_source_mapping_duplicate(ctx, ctx->cg.defer_stack[i]);
        codegen_node_single(ctx, ctx->cg.defer_stack[i]);
    }
}

// Leaves the current function, by `return` or by `?`: runs the pending defers and, under MISRA,
// goes through the single exit. The caller emits the returned value between the two calls and
// passes both the same `value`, NULL for a `return;`.
void emit_function_exit_begin(ParserContext *ctx, ASTNode *value)
{
    int has_defers = ctx->cg.defer_count > ctx->cg.func_defer_boundary;
    Type *ret = ctx->cg.current_func_ret_type_info;
    if (!value)
    {
        if (!has_defers && !ctx->config->misra_mode)
        {
            EMIT(ctx, "return ");
        }
        return;
    }
    if (has_defers)
    {
        EMIT(ctx, "{ ");
        emitter_indent(&ctx->cg.emitter);
        if (ret && ret->kind != TYPE_VOID)
        {
            emit_c_decl(ctx, ret, "_z_ret");
        }
        else
        {
            emit_auto_declaration(ctx, value, "_z_ret");
        }
        EMIT(ctx, " = ");
    }
    else if (!ctx->config->misra_mode)
    {
        EMIT(ctx, "return ");
    }
    else if (ret && ret->kind != TYPE_VOID)
    {
        EMIT(ctx, "_misra_ret = ");
    }
}

void emit_function_exit_end(ParserContext *ctx, ASTNode *value)
{
    if (ctx->cg.defer_count <= ctx->cg.func_defer_boundary)
    {
        EMIT(ctx, ";\n");
        if (ctx->config->misra_mode)
        {
            EMIT(ctx, "goto _misra_end_of_func;\n");
        }
        return;
    }
    if (value)
    {
        EMIT(ctx, "; ");
    }
    emit_defers_down_to(ctx, ctx->cg.func_defer_boundary);
    if (value)
    {
        emitter_dedent(&ctx->cg.emitter);
        EMIT(ctx, ctx->config->misra_mode ? "_misra_ret = _z_ret; goto _misra_end_of_func; }\n"
                                          : "return _z_ret; }\n");
    }
    else
    {
        EMIT(ctx, ctx->config->misra_mode ? "goto _misra_end_of_func;\n" : "return;\n");
    }
}

// Name of the struct with Drop that a call statement produces and discards, or NULL.
static char *discarded_drop_type(ParserContext *ctx, ASTNode *node)
{
    if (node->kind != NODE_EXPR_CALL)
    {
        return NULL;
    }

    return value_drop_struct(ctx, node);
}

void codegen_node_single(ParserContext *ctx, ASTNode *node)
{
    if (!node)
    {
        return;
    }

    static const CodegenHandler handlers[256] = {
        [NODE_AST_COMMENT] = handle_node_ast_comment,
        [NODE_MATCH] = handle_node_match,
        [NODE_ASSERT] = handle_node_assert,
        [NODE_EXPECT] = handle_node_expect,
        [NODE_DEFER] = handle_node_defer,
        [NODE_BLOCK] = handle_node_block,
        [NODE_IMPL] = handle_node_impl,
        [NODE_FUNCTION] = handle_node_function,
        [NODE_IMPL_TRAIT] = handle_node_impl_trait,
        [NODE_DESTRUCT_VAR] = handle_node_destruct_var,
        [NODE_VAR_DECL] = handle_node_var_decl,
        [NODE_CONST] = handle_node_const,
        [NODE_FIELD] = handle_node_field,
        [NODE_IF] = handle_node_if,
        [NODE_WHILE] = handle_node_while,
        [NODE_COMPTIME] = handle_node_comptime,
    };

    if (node->kind < 256 && handlers[node->kind])
    {
        handlers[node->kind](ctx, node);
        return;
    }

    switch (node->kind)
    {
    case NODE_FOR:
    {
        // The initializer is declared in the `for` header, not by handle_node_var_decl. A loop
        // variable that hides a name its initializer reads (`for (let i = i; ...)`) has a C name
        // of its own (tc_declare_local), so the initializer reads the hidden one.
        codegen_loop_enter(ctx, node->for_stmt.loop_label);
        EMIT(ctx, "for (");
        if (node->for_stmt.init)
        {
            if (node->for_stmt.init->kind == NODE_VAR_DECL)
            {
                ASTNode *v = node->for_stmt.init;
                char *declared = node_c_type(ctx, v);
                if (declared)
                {
                    emit_c_decl(ctx, v->type_info,
                                local_decl_c_name(ctx, v->token, v->var_decl.name));
                    EMIT(ctx, " = (%s)(", declared);
                    codegen_expression(ctx, v->var_decl.init_expr);
                    EMIT(ctx, ")");
                }
                else
                {
                    emit_auto_declaration(ctx, v->var_decl.init_expr,
                                          local_decl_c_name(ctx, v->token, v->var_decl.name));
                    EMIT(ctx, " = ");
                    codegen_expression(ctx, v->var_decl.init_expr);
                }
            }
            else
            {
                codegen_expression(ctx, node->for_stmt.init);
            }
        }
        EMIT(ctx, "; ");
        if (node->for_stmt.condition)
        {
            codegen_expression_bare(ctx, node->for_stmt.condition);
        }
        EMIT(ctx, "; ");
        if (node->for_stmt.step)
        {
            codegen_expression_bare(ctx, node->for_stmt.step);
        }
        EMIT(ctx, ") ");
        codegen_loop_body(ctx, node->for_stmt.body, node->for_stmt.loop_label, 1);
        codegen_loop_exit(ctx, node->for_stmt.loop_label);
        break;
    }
    case NODE_BREAK:
        // Run defers from current scope down to loop boundary before breaking
        if (ctx->cg.loop_depth > 0)
        {
            int boundary = ctx->cg.loop_defer_boundary[ctx->cg.loop_depth - 1];
            emit_defers_down_to(ctx, boundary);
        }
        if (node->break_stmt.target_label)
        {
            EMIT(ctx, "goto __break_%s;\n", node->break_stmt.target_label);
        }
        else
        {
            EMIT(ctx, "break;\n");
        }
        break;
    case NODE_CONTINUE:
        // Run defers from current scope down to loop boundary before continuing
        if (ctx->cg.loop_depth > 0)
        {
            int boundary = ctx->cg.loop_defer_boundary[ctx->cg.loop_depth - 1];
            emit_defers_down_to(ctx, boundary);
        }
        if (node->continue_stmt.target_label)
        {
            EMIT(ctx, "goto __continue_%s;\n", node->continue_stmt.target_label);
        }
        else
        {
            EMIT(ctx, "continue;\n");
        }
        break;
    case NODE_GOTO:
        if (node->goto_stmt.goto_expr)
        {
            // Computed goto: goto *expr;
            EMIT(ctx, "goto *(");
            codegen_expression(ctx, node->goto_stmt.goto_expr);
            EMIT(ctx, ");\n");
        }
        else
        {
            EMIT(ctx, "goto %s;\n", node->goto_stmt.label_name);
        }
        break;
    case NODE_LABEL:
        EMIT(ctx, "%s:;\n", node->label_stmt.label_name);
        break;
    // Loop constructs: loop, for-in
    case NODE_LOOP:
    {
        // loop { ... } => while (1) { ... }
        codegen_loop_enter(ctx, node->loop_stmt.loop_label);
        EMIT(ctx, "while (1) ");
        codegen_loop_body(ctx, node->loop_stmt.body, node->loop_stmt.loop_label, 0);
        codegen_loop_exit(ctx, node->loop_stmt.loop_label);
        break;
    }
    case NODE_FOR_RANGE:
    {
        // A loop variable that hides a name its bounds or step read (`for i in 0..i`) has a C
        // name of its own (tc_declare_local), so the header reads the hidden one. A negated step
        // (`step -1`) counts down.
        const char *var =
            local_decl_c_name(ctx, node->for_range.var_token, node->for_range.var_name);
        ASTNode *step = node->for_range.step;
        int counts_down = step && step->kind == NODE_EXPR_UNARY && step->unary.op == OP_NEG;

        codegen_loop_enter(ctx, node->for_range.loop_label);
        EMIT(ctx, "for (");
        if (z_path_match_compiler(ctx->config->cc, "tcc"))
        {
            EMIT(ctx, "__typeof__((");
            codegen_expression(ctx, node->for_range.start);
            EMIT(ctx, ")) %s = ", var);
        }
        else
        {
            EMIT(ctx, "ZC_AUTO %s = ", var);
        }
        codegen_expression(ctx, node->for_range.start);
        if (counts_down)
        {
            if (node->for_range.is_inclusive)
            {
                EMIT(ctx, "; %s >= ", var);
            }
            else
            {
                EMIT(ctx, "; %s > ", var);
            }
        }
        else
        {
            if (node->for_range.is_inclusive)
            {
                EMIT(ctx, "; %s <= ", var);
            }
            else
            {
                EMIT(ctx, "; %s < ", var);
            }
        }
        codegen_expression(ctx, node->for_range.end);
        EMIT(ctx, "; %s", var);
        if (step)
        {
            EMIT(ctx, " += ");
            codegen_expression(ctx, step);
            EMIT(ctx, ") ");
        }
        else
        {
            EMIT(ctx, "++) ");
        }
        codegen_loop_body(ctx, node->for_range.body, node->for_range.loop_label, 0);
        codegen_loop_exit(ctx, node->for_range.loop_label);
        break;
    }

    case NODE_ASM:
    {
        int is_extended = (node->asm_stmt.output_count > 0 || node->asm_stmt.input_count > 0 ||
                           node->asm_stmt.clobber_count > 0);

        if (node->asm_stmt.is_volatile)
        {
            EMIT(ctx, "__asm__ __volatile__(");
        }
        else
        {
            EMIT(ctx, "__asm__(");
        }

        char *code = node->asm_stmt.code;
        size_t transformed_sz = strlen(code) * 3 + 128; // Extra buffer for expansions
        char *transformed = xmalloc(transformed_sz);
        char *dst = transformed;

        for (char *p = code; *p; p++)
        {
            if (*p == '{')
            {
                // Find matching }
                char *end = (char *)strchr(p + 1, '}');
                if (end)
                {
                    // Extract variable name
                    ptrdiff_t var_len = end - p - 1;
                    char var_name[64];
                    strncpy(var_name, p + 1, (size_t)(var_len));
                    var_name[var_len] = 0;

                    // Find variable index
                    int idx = -1;

                    // Check outputs first
                    for (int i = 0; i < node->asm_stmt.output_count; i++)
                    {
                        if (strcmp(node->asm_stmt.outputs[i], var_name) == 0)
                        {
                            idx = i;
                            break;
                        }
                    }

                    // Then check inputs
                    if (idx == -1)
                    {
                        for (int i = 0; i < node->asm_stmt.input_count; i++)
                        {
                            if (strcmp(node->asm_stmt.inputs[i], var_name) == 0)
                            {
                                idx = node->asm_stmt.output_count + i;
                                break;
                            }
                        }
                    }

                    size_t rem = transformed_sz - (size_t)(dst - transformed);
                    if (idx >= 0)
                    {
                        // Replace with %N
#if defined(ZC_ARCH_ARM64)
                        // Use most optimal register size on arm architectures
                        if (node->asm_stmt.register_size <= 32)
                        {
                            int _n = snprintf(dst, rem, "%%w%d", idx);
                            dst += (_n > 0 && (size_t)_n < rem) ? (size_t)_n
                                   : rem > 1                    ? rem - 1
                                                                : 0;
                        }
                        else
#endif
                        {
                            int _n = snprintf(dst, rem, "%%%d", idx);
                            dst += (_n > 0 && (size_t)_n < rem) ? (size_t)_n
                                   : rem > 1                    ? rem - 1
                                                                : 0;
                        }
                    }
                    else
                    {
                        // Variable not found - error or keep as-is?
                        int _n = snprintf(dst, rem, "{%s}", var_name);
                        dst += (_n > 0 && (size_t)_n < rem) ? (size_t)_n : rem > 1 ? rem - 1 : 0;
                    }

                    p = end; // Skip past }
                }
                else
                {
                    *dst++ = *p;
                }
            }
            else if (*p == '%')
            {
                if (is_extended)
                {
                    *dst++ = '%';
                    *dst++ = '%';
                }
                else
                {
                    *dst++ = '%';
                }
            }
            else
            {
                *dst++ = *p;
            }
        }
        *dst = 0;

        EMIT(ctx, "\"");
        for (char *p = transformed; *p; p++)
        {
            if (*p == '\n')
            {
                EMIT(ctx, "\\n\"\n        \"");
            }
            else if (*p == '"')
            {
                EMIT(ctx, "\\\"");
            }
            else if (*p == '\\')
            {
                EMIT(ctx, "\\\\");
            }
            else
            {
                EMIT(ctx, "%c", *p);
            }
        }
        EMIT(ctx, "\\n\"");

        if (node->asm_stmt.output_count > 0)
        {
            EMIT(ctx, "\n        : ");
            for (int i = 0; i < node->asm_stmt.output_count; i++)
            {
                if (i > 0)
                {
                    EMIT(ctx, ", ");
                }

                // `inout(x)` is read and written, `out(x)` only written.
                EMIT(ctx, "\"%s\"(%s)", node->asm_stmt.output_is_inout[i] ? "+r" : "=r",
                     node->asm_stmt.outputs[i]);
            }
        }

        if (node->asm_stmt.input_count > 0)
        {
            EMIT(ctx, "\n        : ");
            for (int i = 0; i < node->asm_stmt.input_count; i++)
            {
                if (i > 0)
                {
                    EMIT(ctx, ", ");
                }
                EMIT(ctx, "\"r\"(%s)", node->asm_stmt.inputs[i]);
            }
        }
        else if (node->asm_stmt.output_count > 0)
        {
            EMIT(ctx, "\n        : ");
        }

        if (node->asm_stmt.clobber_count > 0)
        {
            EMIT(ctx, "\n        : ");
            for (int i = 0; i < node->asm_stmt.clobber_count; i++)
            {
                if (i > 0)
                {
                    EMIT(ctx, ", ");
                }
                EMIT(ctx, "\"%s\"", node->asm_stmt.clobbers[i]);
            }
        }

        EMIT(ctx, ");\n");
        break;
    }
    case NODE_RETURN:
    {
        int handled = 0;

        if (node->ret.value && node->ret.value->kind == NODE_EXPR_VAR &&
            moves_drop_value(ctx, node->ret.value))
        {
            if (ctx->config->misra_mode)
            {
                EMIT(ctx, "_misra_ret = ({ ");
            }
            else
            {
                EMIT(ctx, "return ({ ");
            }

            if (z_path_match_compiler(ctx->config->cc, "tcc"))
            {
                EMIT(ctx, "__typeof__(");
                codegen_expression(ctx, node->ret.value);
                EMIT(ctx, ")");
            }
            else
            {
                EMIT(ctx, "ZC_AUTO");
            }
            EMIT(ctx, " _z_ret_mv = ");
            codegen_expression(ctx, node->ret.value);
            // A `const` variable is still owned: it is cleared through a mutable pointer.
            Type *ret_type = node->ret.value->type_info;
            EMIT(ctx, "; memset(%s&", (ret_type && ret_type->is_const) ? "(void *)" : "");
            codegen_expression(ctx, node->ret.value);
            EMIT(ctx, ", 0, sizeof(_z_ret_mv)); ");
            EMIT(ctx, "__z_drop_flag_%s = 0; ", var_ref_flag_name(node->ret.value));
            emit_defers_down_to(ctx, ctx->cg.func_defer_boundary);
            EMIT(ctx, "_z_ret_mv; });\n");

            if (ctx->config->misra_mode)
            {
                EMIT(ctx, "goto _misra_end_of_func;\n");
            }
            handled = 1;
        }

        if (!handled)
        {
            ASTNode *value = node->ret.value;
            emit_function_exit_begin(ctx, value);
            if (value)
            {
                codegen_expression(ctx, value);
            }
            emit_function_exit_end(ctx, value);
        }
        break;
    }
    case NODE_EXPR_MEMBER:
    {
        codegen_expression(ctx, node->member.target);
        Type *target_type = node->member.target->type_info;
        if (target_type && get_inner_type(target_type)->kind == TYPE_POINTER)
        {
            EMIT(ctx, "->%s", c_name(ctx, node->member.field));
        }
        else
        {
            EMIT(ctx, ".%s", c_name(ctx, node->member.field));
        }
        break;
    }
    case NODE_REPL_PRINT:
    {
        EMIT(ctx, "{ ");
        emit_auto_declaration(ctx, node->repl_print.expr, "_zval");
        EMIT(ctx, " = (");
        codegen_expression(ctx, node->repl_print.expr);
        EMIT(ctx,
             "); fprintf(stdout, _z_str(_zval), _z_arg(_zval)); fprintf(stdout, \"\\n\"); }\n");
        break;
    }
    case NODE_EXPR_LITERAL:
        // String literal statement should auto-print
        if (node->literal.kind == LITERAL_STRING)
        {
            EMIT(ctx, "printf(\"%%s\\n\", ");
            codegen_expression(ctx, node);
            EMIT(ctx, ");\n");
        }
        else
        {
            // Non-string literals as statements - just evaluate
            codegen_expression(ctx, node);
            EMIT(ctx, ";\n");
        }
        break;
    case NODE_CUDA_LAUNCH:
    {
        // Emit CUDA kernel launch: kernel<<<grid, block, shared, stream>>>(args);
        ASTNode *call = node->cuda_launch.call;

        // Get kernel name from callee
        if (call->call.callee->kind == NODE_EXPR_VAR)
        {
            EMIT(ctx, "%s<<<", call->call.callee->var_ref.name);
        }
        else
        {
            codegen_expression(ctx, call->call.callee);
            EMIT(ctx, "<<<");
        }

        // Grid dimension
        codegen_expression(ctx, node->cuda_launch.grid);
        EMIT(ctx, ", ");

        // Block dimension
        codegen_expression(ctx, node->cuda_launch.block);

        // Optional shared memory size
        if (node->cuda_launch.shared_mem || node->cuda_launch.stream)
        {
            EMIT(ctx, ", ");
            if (node->cuda_launch.shared_mem)
            {
                codegen_expression(ctx, node->cuda_launch.shared_mem);
            }
            else
            {
                EMIT(ctx, "0");
            }
        }

        // Optional CUDA stream
        if (node->cuda_launch.stream)
        {
            EMIT(ctx, ", ");
            codegen_expression(ctx, node->cuda_launch.stream);
        }

        EMIT(ctx, ">>>(");

        // Arguments
        ASTNode *arg = call->call.args;
        int first = 1;
        while (arg)
        {
            if (!first)
            {
                EMIT(ctx, ", ");
            }
            codegen_expression(ctx, arg);
            first = 0;
            arg = arg->next;
        }

        EMIT(ctx, ");\n");
        break;
    }
    case NODE_PREPROC_DIRECTIVE:
    {
        EMIT(ctx, "%s\n", node->raw_stmt.content);
        break;
    }
    case NODE_RAW_STMT:
        emit_raw_with_captures(ctx, node->raw_stmt.content);
        EMIT(ctx, "\n");
        break;

    case NODE_LINK:
        // @link handled during compilation setup — nothing to emit
        break;

    default:
    {
        char *discard_type = discarded_drop_type(ctx, node);
        if (emit_direct_print(ctx, node))
        {
            EMIT(ctx, ";\n");
        }
        else if (discard_type)
        {
            // The result is never bound to a name: drop it right away.
            int id = ctx->cg.tmp_counter++;
            EMIT(ctx, "{ %s _z_discard_%d = ", discard_type, id);
            codegen_expression(ctx, node);
            EMIT(ctx, "; %s__Drop__glue(&_z_discard_%d); }\n", discard_type, id);
        }
        else
        {
            codegen_expression(ctx, node);
            EMIT(ctx, ";\n");
        }
    }
    }
}

// Walks AST nodes and generates code.
void codegen_walker(ParserContext *ctx, ASTNode *node)
{
    while (node)
    {
        emit_source_mapping(ctx, node); // Step to this expression
        codegen_node_single(ctx, node);
        node = node->next;
    }
}
