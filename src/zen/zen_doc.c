// SPDX-License-Identifier: MIT
#include "zen_doc.h"
#include "../ast/ast.h"
#include "../parser/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief Cleans a doc comment by removing leading markers.
 */
static char *clean_comment_content(const char *content)
{
    if (!content)
    {
        return NULL;
    }

    // Calculate total length (roughly)
    char *result = xmalloc(strlen(content) + 1);
    char *out = result;
    const char *in = content;
    int done = 0;

    while (*in)
    {
        // Skip leading whitespace on each line
        while (*in && isspace(*in))
        {
            in++;
        }

        // Remove markers at start of line
        if (strncmp(in, "*/", 2) == 0)
        {
            in += 2;
            break;
        }
        else if (strncmp(in, "///", 3) == 0)
        {
            in += 3;
        }
        else if (strncmp(in, "//", 2) == 0)
        {
            in += 2;
        }
        else if (strncmp(in, "/**", 3) == 0)
        {
            in += 3;
        }
        else if (strncmp(in, "/*", 2) == 0)
        {
            in += 2;
        }
        else if (*in == '*')
        {
            in++; // Star at start of block comment line
        }

        // Skip spaces after marker
        while (*in == ' ')
        {
            in++;
        }

        // Copy until end of line
        while (*in && *in != '\n' && *in != '\r')
        {
            // Avoid copying block comment end marker
            if (*in == '*' && in[1] == '/')
            {
                in += 2;
                done = 1;
                break;
            }
            *out++ = *in++;
        }

        if (done)
        {
            break;
        }

        if (*in == '\n' || *in == '\r')
        {
            *out++ = '\n';
            if (*in == '\r' && in[1] == '\n')
            {
                in++;
            }
            in++;
        }
    }
    *out = 0;

    // Trim trailing newlines
    while (out > result && isspace(*(out - 1)))
    {
        *(--out) = 0;
    }

    return result;
}

static void print_markdown_doc(const char *comment)
{
    if (!comment)
    {
        return;
    }
    char *cleaned = clean_comment_content(comment);
    if (cleaned && cleaned[0])
    {
        printf("\n%s\n\n", cleaned);
    }
    zfree(cleaned);
}

// The name a declaration writes (`helper`), from its name token: the node's own name can be its
// C name (`_geo__helper`, in a module).
static void print_declared_name(Token name_token, const char *name)
{
    if (name_token.start && name_token.len > 0)
    {
        printf("%.*s", (int)name_token.len, name_token.start);
    }
    else
    {
        printf("%s", name);
    }
}

// A function's signature as it declares it (`fn len(self) -> i32;`). The receiver is written as
// programs write it, `self` or `const self`, not by its type (`Point*`): it is the receiver by
// its name.
static void print_signature(struct ParserContext *ctx, ASTNode *fn, const char *name)
{
    printf("```zc\nfn ");
    print_declared_name(fn->token, name);
    printf("(");
    for (int i = 0; i < fn->func.count; i++)
    {
        const char *param = fn->func.param_names ? fn->func.param_names[i] : NULL;
        Type *type = fn->func.arg_types ? fn->func.arg_types[i] : NULL;
        if (param && strcmp(param, "self") == 0)
        {
            int is_const =
                type && type->kind == TYPE_POINTER && type->inner && type->inner->is_const;
            printf("%sself", is_const ? "const " : "");
        }
        else
        {
            if (param)
            {
                printf("%s: ", param);
            }
            if (type)
            {
                char *spelled = zen_c_type_spelling(ctx, type);
                printf("%s", spelled);
                zfree(spelled);
            }
            else
            {
                printf("?");
            }
        }
        if (i < fn->func.count - 1)
        {
            printf(", ");
        }
    }
    printf(")");
    if (fn->func.ret_type_info)
    {
        char *spelled = zen_c_type_spelling(ctx, fn->func.ret_type_info);
        printf(" -> %s", spelled);
        zfree(spelled);
    }
    printf(";\n```\n");
}

static void generate_docs_internal(struct ParserContext *ctx, ASTNode *node, int level)
{
    while (node)
    {
        if (node->doc_comment)
        {
            // Associated with this node
        }

        switch (node->kind)
        {
        case NODE_FUNCTION:
        {
            // The title as programs write the name (`geo::helper`), the signature as declared.
            printf("\n## function `%s`\n",
                   node->func.written_name ? node->func.written_name : node->func.name);
            if (node->doc_comment)
            {
                print_markdown_doc(node->doc_comment);
            }

            print_signature(ctx, node, node->func.name);
            break;
        }

        case NODE_STRUCT:
        {
            printf("\n## struct `%s`\n",
                   node->strct.written_name ? node->strct.written_name : node->strct.name);
            if (node->doc_comment)
            {
                print_markdown_doc(node->doc_comment);
            }

            printf("```zc\nstruct ");
            print_declared_name(node->token, node->strct.name);
            printf(" {\n");
            ASTNode *field = node->strct.fields;
            while (field)
            {
                if (field->kind == NODE_FIELD)
                {
                    printf("    %s: %s,\n", field->field.name,
                           zen_c_type_spelling(ctx, field->type_info));
                }
                field = field->next;
            }
            printf("}\n```\n");

            // Print field docs if any
            field = node->strct.fields;
            while (field)
            {
                if (field->kind == NODE_FIELD && field->doc_comment)
                {
                    printf("\n### field `%s`\n", field->field.name);
                    print_markdown_doc(field->doc_comment);
                }
                field = field->next;
            }
            break;
        }

        case NODE_ENUM:
        {
            printf("\n## enum `%s`\n",
                   node->enm.written_name ? node->enm.written_name : node->enm.name);
            if (node->doc_comment)
            {
                print_markdown_doc(node->doc_comment);
            }

            printf("```zc\nenum ");
            print_declared_name(node->token, node->enm.name);
            printf(" {\n");
            ASTNode *variant = node->enm.variants;
            while (variant)
            {
                if (variant->kind == NODE_ENUM_VARIANT)
                {
                    printf("    %s,\n", variant->variant.name);
                }
                variant = variant->next;
            }
            printf("}\n```\n");

            // Print variant docs if any
            variant = node->enm.variants;
            while (variant)
            {
                if (variant->kind == NODE_ENUM_VARIANT && variant->doc_comment)
                {
                    printf("\n### variant `%s`\n", variant->variant.name);
                    print_markdown_doc(variant->doc_comment);
                }
                variant = variant->next;
            }
            break;
        }

        case NODE_CONST:
        {
            const char *display_name = node->var_decl.name;
            printf("\n## const `%s`\n", display_name);
            if (node->doc_comment)
            {
                print_markdown_doc(node->doc_comment);
            }
            printf("```zc\nconst %s: %s;\n```\n", display_name,
                   node->type_info ? zen_c_type_spelling(ctx, node->type_info) : "?");
            break;
        }

        case NODE_TYPE_ALIAS:
        {
            const char *display_name = node->type_alias.alias;
            printf("\n## alias `%s`\n", display_name);
            if (node->doc_comment)
            {
                print_markdown_doc(node->doc_comment);
            }
            printf("```zc\nalias %s = %s;\n```\n", display_name,
                   node->type_info ? zen_c_type_spelling(ctx, node->type_info) : "?");
            break;
        }

        case NODE_IMPL:
        case NODE_IMPL_TRAIT:
        {
            Type *target = (node->kind == NODE_IMPL) ? node->impl.target : node->impl_trait.target;
            printf("\n## impl for `%s`\n", zen_c_type_spelling(ctx, target));
            if (node->doc_comment)
            {
                print_markdown_doc(node->doc_comment);
            }

            ASTNode *method =
                (node->kind == NODE_IMPL) ? node->impl.methods : node->impl_trait.methods;
            while (method)
            {
                if (method->kind == NODE_FUNCTION)
                {
                    const char *mname = method->func.method_name;
                    printf("\n### method `%s`\n", mname);
                    if (method->doc_comment)
                    {
                        print_markdown_doc(method->doc_comment);
                    }
                    print_signature(ctx, method, mname);
                }
                method = method->next;
            }
            break;
        }

        case NODE_IMPORT:
        {
            if (ctx->config->recursive_doc)
            {
                printf("\n---\n# Module: %s\n", node->import_stmt.path);
                if (node->doc_comment)
                {
                    print_markdown_doc(node->doc_comment);
                }
                generate_docs_internal(ctx, node->import_stmt.module_root, level + 1);
                printf("\n---\n");
            }
            break;
        }

        default:
            break;
        }

        node = node->next;
    }
}

void generate_docs(struct ParserContext *ctx, ASTNode *root)
{
    if (root->kind != NODE_ROOT)
    {
        return;
    }

    printf("# Module: %s\n", ctx->config->input_file ? ctx->config->input_file : "Unknown");
    if (root->doc_comment)
    {
        print_markdown_doc(root->doc_comment);
    }

    generate_docs_internal(ctx, root->root.children, 0);

    // Generic impl blocks (`impl Box<T>`) are registered as templates instead
    // of top-level AST nodes, so the walk above skips them. Render their
    // methods here (isolating each impl node so the iterator stops cleanly).
    for (GenericImplTemplate *it = ctx->impl_templates; it; it = it->next)
    {
        if (it->impl_node && it->impl_node->kind == NODE_IMPL)
        {
            ASTNode *saved_next = it->impl_node->next;
            it->impl_node->next = NULL;
            generate_docs_internal(ctx, it->impl_node, 0);
            it->impl_node->next = saved_next;
        }
    }
}
