// SPDX-License-Identifier: MIT
// AST dump backend - Unicode tree format
// Licensed under MIT.
// Repository: https://github.com/z-libs
#include "codegen_backend.h"
#include "../parser/parser.h"
#include "../ast/ast.h"

// Forward declaration
static void dump_node(ParserContext *ctx, ASTNode *node, int is_last, const char *prefix);

// Emit a node label: name, literals, type info, location
static void emit_label(ParserContext *ctx, ASTNode *node)
{
    // Title: short type name
    emitter_printf(&ctx->cg.emitter, " %s", ast_kind_name(node->kind));

    // Name or operator for relevant node types
    switch (node->kind)
    {
    case NODE_FUNCTION:
    case NODE_VAR_DECL:
    case NODE_FIELD_INIT:
    case NODE_CONST:
    case NODE_TYPE_ALIAS:
    case NODE_STRUCT:
    case NODE_ENUM:
    case NODE_TRAIT:
    case NODE_FIELD:
    case NODE_ENUM_VARIANT:
        if (node->var_decl.name)
        {
            emitter_printf(&ctx->cg.emitter, " '%s'", node->var_decl.name);
        }
        break;
    case NODE_IMPL:
        if (type_impl_name(node->impl.target))
        {
            emitter_printf(&ctx->cg.emitter, " '%s'", type_impl_name(node->impl.target));
        }
        break;
    case NODE_IMPORT:
        if (node->import_stmt.path)
        {
            const char *p = node->import_stmt.path;
            const char *last = (char *)strrchr(p, '/');
            emitter_printf(&ctx->cg.emitter, " '%s'", last ? last + 1 : p);
        }
        break;
    case NODE_INCLUDE:
        if (node->include.path)
        {
            emitter_printf(&ctx->cg.emitter, " '%s'%s", node->include.path,
                           node->include.is_system ? " (system)" : "");
        }
        break;
    case NODE_EXPR_BINARY:
        if (node->binary.op != OP_NONE)
        {
            emitter_printf(&ctx->cg.emitter, " '%s'", operator_spelling(node->binary.op));
        }
        break;
    case NODE_EXPR_UNARY:
        if (node->unary.op != OP_NONE)
        {
            emitter_printf(&ctx->cg.emitter, " '%s'", operator_spelling(node->unary.op));
        }
        break;
    case NODE_EXPR_CALL:
        if (node->call.callee && node->call.callee->kind == NODE_EXPR_VAR &&
            node->call.callee->var_ref.name)
        {
            // A method or a variant by its own name (`push`, not `Vec__int32_t__push`).
            const char *own = callee_own_name(ctx, node->call.callee);
            emitter_printf(&ctx->cg.emitter, " '%s'", own ? own : node->call.callee->var_ref.name);
        }
        break;
    case NODE_RAW_STMT:
    {
        if (node->raw_stmt.content)
        {
            if (backend_opt(&ctx->config->backend_opts, "full-content"))
            {
                emitter_printf(&ctx->cg.emitter, " '%s'", node->raw_stmt.content);
            }
            else
            {
                char preview[44];
                size_t len = strlen(node->raw_stmt.content);
                int slen = (int)(len < 40 ? len : 40);
                snprintf(preview, sizeof(preview), "%.*s%s", slen, node->raw_stmt.content,
                         (size_t)slen < len ? "..." : "");
                emitter_printf(&ctx->cg.emitter, " '%s'", preview);
            }
        }
        break;
    }
    default:
        break;
    }

    // Literal values
    if (node->kind == NODE_EXPR_LITERAL)
    {
        switch (node->literal.kind)
        {
        case LITERAL_INT:
            emitter_printf(&ctx->cg.emitter, " %llu", node->literal.int_val);
            break;
        case LITERAL_FLOAT:
            emitter_printf(&ctx->cg.emitter, " %g", node->literal.float_val);
            break;
        case LITERAL_STRING:
        case LITERAL_RAW_STRING:
            if (node->literal.string_val)
            {
                emitter_printf(&ctx->cg.emitter, " \"%s\"", node->literal.string_val);
            }
            break;
        case LITERAL_CHAR:
            emitter_printf(&ctx->cg.emitter, " '%c'", (char)node->literal.int_val);
            break;
        default:
            break;
        }
    }

    // Type annotation
    if (node->type_info)
    {
        char *tn = type_to_string(node->type_info);
        if (tn)
        {
            emitter_printf(&ctx->cg.emitter, " :%s", tn);
            zfree(tn);
        }
    }

    if (node->token.start)
    {
        emitter_printf(&ctx->cg.emitter, " [%d:%d]", node->token.line, node->token.col);
    }
}

// Dump a node and, below it, all its children
static void dump_node(ParserContext *ctx, ASTNode *node, int is_last, const char *prefix)
{
    if (!node)
    {
        return;
    }

    // Emit prefix + tree branch
    if (prefix && prefix[0])
    {
        emitter_printf(&ctx->cg.emitter, "%s%s", prefix,
                       is_last ? "\342\224\224\342\224\200\342\224\200 "   // └──
                               : "\342\224\234\342\224\200\342\224\200 "); // ├──
    }

    emit_label(ctx, node);
    emitter_printf(&ctx->cg.emitter, "\n");

    // Children: every node of every child field, in order. Count them first so
    // that only the last one closes the branch (└──).
    ASTChildren children = ast_children(node);
    int total = 0;
    for (int i = 0; i < children.count; i++)
    {
        ASTNode *c = *children.slots[i];
        while (c)
        {
            total++;
            c = c->next;
        }
    }

    char child_prefix[512];
    snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix,
             is_last ? "    " : "\342\224\202   "); // │

    int idx = 0;
    for (int i = 0; i < children.count; i++)
    {
        ASTNode *c = *children.slots[i];
        while (c)
        {
            idx++;
            dump_node(ctx, c, idx == total, child_prefix);
            c = c->next;
        }
    }
}

static void astdump_emit_program(ParserContext *ctx, ASTNode *root)
{
    dump_node(ctx, root, 1, "");
}

static void astdump_emit_preamble(ParserContext *ctx)
{
    (void)ctx;
}

static const BackendOptAlias astdump_aliases[] = {
    {"--backend-full-content", "full-content", NULL},
    {NULL, NULL, NULL},
};

static const CodegenBackend astdump_backend = {
    .name = "ast-dump",
    .extension = ".ast",
    .emit_program = astdump_emit_program,
    .emit_preamble = astdump_emit_preamble,
    .needs_cc = 0,
    .aliases = astdump_aliases,
};

void codegen_register_astdump_backend(void)
{
    codegen_register_backend(&astdump_backend);
}
