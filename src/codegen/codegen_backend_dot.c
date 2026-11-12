// SPDX-License-Identifier: MIT
// Graphviz DOT backend
#include "codegen_backend.h"
#include "../parser/parser.h"
#include "../ast/ast.h"
#include <string.h>

static int node_id = 0;

static void dot_escape_label(ParserContext *ctx, const char *s)
{
    if (!s)
    {
        return;
    }
    for (const char *p = s; *p; p++)
    {
        char c = *p;
        if (c == '"')
        {
            emitter_printf(&ctx->cg.emitter, "\\\"");
        }
        else if (c == '\\')
        {
            emitter_printf(&ctx->cg.emitter, "\\\\");
        }
        else if (c == '\n')
        {
            emitter_printf(&ctx->cg.emitter, "\\n");
        }
        else if (c == '>')
        {
            emitter_printf(&ctx->cg.emitter, "\\>");
        }
        else if (c == '<')
        {
            emitter_printf(&ctx->cg.emitter, "\\<");
        }
        else if (c == '{' || c == '}' || c == '|')
        {
            emitter_printf(&ctx->cg.emitter, "\\%c", c);
        }
        else if (c >= 32 && c < 127)
        {
            emitter_printf(&ctx->cg.emitter, "%c", c);
        }
    }
}

static int dot_emit_node(ParserContext *ctx, ASTNode *node);
static int dot_emit_children(ParserContext *ctx, ASTNode *children)
{
    if (!children)
    {
        return -1;
    }
    int first_child = -1;
    int prev = -1;
    for (ASTNode *c = children; c; c = c->next)
    {
        int cid = dot_emit_node(ctx, c);
        if (first_child < 0)
        {
            first_child = cid;
        }
        if (prev >= 0)
        {
            emitter_printf(&ctx->cg.emitter, "  %d -> %d [style=dotted,arrowhead=none];\n", prev,
                           cid);
        }
        prev = cid;
    }
    return first_child;
}

// The label of the edge to slot `slot` of ast_children(node), or NULL: it
// relies on the order in which ast_children() gives the slots.
static const char *dot_edge_label(ASTNode *node, int slot)
{
    static const char *const if_labels[] = {"cond", "then", "else"};
    static const char *const binary_labels[] = {"L", "R"};
    static const char *const while_labels[] = {"cond", NULL};
    static const char *const for_labels[] = {"init", "cond", "step", NULL};
    static const char *const for_range_labels[] = {"from", "to", NULL};

    switch (node->kind)
    {
    case NODE_IF:
        return if_labels[slot];
    case NODE_EXPR_BINARY:
        return binary_labels[slot];
    case NODE_WHILE:
        return while_labels[slot];
    case NODE_FOR:
        return for_labels[slot];
    case NODE_FOR_RANGE:
        return for_range_labels[slot];
    default:
        return NULL;
    }
}

static int dot_emit_node(ParserContext *ctx, ASTNode *node)
{
    if (!node)
    {
        return -1;
    }

    int id = node_id++;
    emitter_printf(&ctx->cg.emitter, "  %d [label=\"%s", id, ast_kind_name(node->kind));

    // Name
    const char *name = NULL;
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
    case NODE_LABEL:
        name = node->var_decl.name;
        break;
    case NODE_IMPL:
        name = type_impl_name(node->impl.target);
        break;
    case NODE_IMPORT:
        name = node->import_stmt.path;
        break;
    case NODE_EXPR_BINARY:
    case NODE_EXPR_UNARY:
        name = node->unary.op;
        break;
    default:
        break;
    }
    if (name)
    {
        emitter_printf(&ctx->cg.emitter, "|");
        dot_escape_label(ctx, name);
    }

    // Literal value
    if (node->kind == NODE_EXPR_LITERAL)
    {
        emitter_printf(&ctx->cg.emitter, "|");
        switch (node->literal.kind)
        {
        case LITERAL_INT:
            emitter_printf(&ctx->cg.emitter, "%llu", node->literal.int_val);
            break;
        case LITERAL_FLOAT:
            emitter_printf(&ctx->cg.emitter, "%g", node->literal.float_val);
            break;
        case LITERAL_STRING:
        case LITERAL_RAW_STRING:
            if (node->literal.string_val)
            {
                emitter_printf(&ctx->cg.emitter, "\\\"");
                dot_escape_label(ctx, node->literal.string_val);
                emitter_printf(&ctx->cg.emitter, "\\\"");
            }
            break;
        case LITERAL_CHAR:
        {
            char ch[2] = {(char)node->literal.int_val, '\0'};
            emitter_printf(&ctx->cg.emitter, "'");
            dot_escape_label(ctx, ch);
            emitter_printf(&ctx->cg.emitter, "'");
            break;
        }
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
            emitter_printf(&ctx->cg.emitter, "|");
            dot_escape_label(ctx, tn);
            zfree(tn);
        }
    }

    emitter_printf(&ctx->cg.emitter, "\"];\n");

    // Children: a list hangs from its first node (see dot_emit_children).
    ASTChildren children = ast_children(node);
    for (int i = 0; i < children.count; i++)
    {
        int cid = dot_emit_children(ctx, *children.slots[i]);
        if (cid < 0)
        {
            continue;
        }
        const char *label = dot_edge_label(node, i);
        if (label)
        {
            emitter_printf(&ctx->cg.emitter, "  %d -> %d [label=\"%s\"];\n", id, cid, label);
        }
        else
        {
            emitter_printf(&ctx->cg.emitter, "  %d -> %d;\n", id, cid);
        }
    }

    return id;
}

static void dot_emit_program(ParserContext *ctx, ASTNode *root)
{
    node_id = 0;
    emitter_printf(&ctx->cg.emitter, "digraph AST {\n");
    emitter_printf(&ctx->cg.emitter, "  rankdir=LR;\n");
    emitter_printf(&ctx->cg.emitter, "  node [shape=record,style=filled,fillcolor=lightyellow];\n");
    dot_emit_node(ctx, root);
    emitter_printf(&ctx->cg.emitter, "}\n");
}

static void dot_emit_preamble(ParserContext *ctx)
{
    (void)ctx;
}

static const BackendOptAlias dot_aliases[] = {
    {NULL, NULL, NULL},
};

static const CodegenBackend dot_backend = {
    .name = "dot",
    .extension = ".dot",
    .emit_program = dot_emit_program,
    .emit_preamble = dot_emit_preamble,
    .needs_cc = 0,
    .aliases = dot_aliases,
};

void codegen_register_dot_backend(void)
{
    codegen_register_backend(&dot_backend);
}
