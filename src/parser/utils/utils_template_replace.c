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
#include <stdlib.h>
#include <string.h>

static char *replace_in_string(const char *src, const char *old_w, const char *new_w)
{
    if (!src || !old_w || !new_w)
    {
        return src ? xstrdup(src) : NULL;
    }

    char *result;
    int i, cnt = 0;
    size_t newWlen = strlen(new_w);
    size_t oldWlen = strlen(old_w);

    // Pass 1: Count replacements
    int in_string = 0;
    for (i = 0; src[i] != '\0'; i++)
    {
        if (src[i] == '\"' && (i == 0 || src[i - 1] != '\\'))
        {
            in_string = !in_string;
        }

        if (!in_string && strstr(&src[i], old_w) == &src[i])
        {
            // Check boundaries
            int valid = 1;
            if (i > 0 && is_ident_char(src[i - 1]))
            {
                valid = 0;
            }
            if (valid && (is_ident_char(src[(size_t)(i) + (size_t)(oldWlen)]) ||
                          src[(size_t)(i) + (size_t)(oldWlen)] == '<'))
            {
                valid = 0;
            }

            if (valid)
            {
                cnt++;
                i = i + (int)oldWlen - 1;
            }
        }
    }

    // Allocate result buffer
    result = (char *)xmalloc((size_t)i + (size_t)cnt * (newWlen - oldWlen) + 1);

    // Pass 2: Perform replacement
    int j = 0;
    in_string = 0;

    int src_idx = 0;

    while (src[src_idx] != '\0')
    {
        if (src[src_idx] == '\"' && (src_idx == 0 || src[src_idx - 1] != '\\'))
        {
            in_string = !in_string;
        }

        int replaced = 0;
        if (!in_string && strstr(&src[src_idx], old_w) == &src[src_idx])
        {
            int valid = 1;
            if (src_idx > 0 && is_ident_char(src[src_idx - 1]))
            {
                valid = 0;
            }
            if (valid && (is_ident_char(src[(size_t)(src_idx) + (size_t)(oldWlen)]) ||
                          src[(size_t)(src_idx) + (size_t)(oldWlen)] == '<'))
            {
                valid = 0;
            }

            if (valid)
            {
                strcpy(&result[j], new_w);
                j = j + (int)newWlen;
                src_idx = src_idx + (int)oldWlen;
                replaced = 1;
            }
        }

        if (!replaced)
        {
            result[j++] = src[src_idx++];
        }
    }
    result[j] = '\0';
    return result;
}

Type *type_from_string_helper(ParserContext *ctx, const char *c)
{
    if (!c)
    {
        return NULL;
    }

    // Check for pointer suffix '*'
    size_t len = strlen(c);
    if (len > 0 && c[len - 1] == '*')
    {
        size_t base_len = len - 1;
        char *base = xmalloc((size_t)(base_len + 1));
        strncpy(base, c, (size_t)(base_len));
        base[base_len] = 0;

        Type *inner = type_from_string_helper(ctx, base);
        zfree(base);

        return type_new_ptr(inner);
    }

    // Check for 'const ' prefix
    if (strncmp(c, "const ", 6) == 0)
    {
        Type *inner = type_from_string_helper(ctx, c + 6);
        if (inner)
        {
            inner->is_const = 1;
        }
        return inner;
    }

    if (strncmp(c, "struct ", 7) == 0)
    {
        Type *n = type_new(TYPE_STRUCT);
        n->name = sanitize_mangled_name(c + 7);
        n->is_explicit_struct = 1;
        return n;
    }

    if (strcmp(c, "int") == 0)
    {
        return type_new(TYPE_I32);
    }
    if (strcmp(c, "float") == 0)
    {
        return type_new(TYPE_F32);
    }
    if (strcmp(c, "void") == 0)
    {
        return type_new(TYPE_VOID);
    }
    if (strcmp(c, "string") == 0)
    {
        return type_new(TYPE_STRING);
    }
    if (strcmp(c, "bool") == 0)
    {
        return type_new(TYPE_BOOL);
    }
    if (strcmp(c, "char") == 0)
    {
        return type_new(TYPE_CHAR);
    }
    if (strcmp(c, "I8") == 0 || strcmp(c, "i8") == 0)
    {
        return type_new(TYPE_I8);
    }
    if (strcmp(c, "U8") == 0 || strcmp(c, "u8") == 0)
    {
        return type_new(TYPE_U8);
    }
    if (strcmp(c, "I16") == 0 || strcmp(c, "i16") == 0)
    {
        return type_new(TYPE_I16);
    }
    if (strcmp(c, "U16") == 0 || strcmp(c, "u16") == 0)
    {
        return type_new(TYPE_U16);
    }
    if (strcmp(c, "I32") == 0 || strcmp(c, "i32") == 0 || strcmp(c, "int32_t") == 0)
    {
        return type_new(TYPE_I32);
    }
    if (strcmp(c, "U32") == 0 || strcmp(c, "u32") == 0 || strcmp(c, "uint32_t") == 0)
    {
        return type_new(TYPE_U32);
    }
    if (strcmp(c, "I64") == 0 || strcmp(c, "i64") == 0 || strcmp(c, "int64_t") == 0)
    {
        return type_new(TYPE_I64);
    }
    if (strcmp(c, "U64") == 0 || strcmp(c, "u64") == 0 || strcmp(c, "uint64_t") == 0)
    {
        return type_new(TYPE_U64);
    }
    if (strcmp(c, "float") == 0 || strcmp(c, "f32") == 0)
    {
        return type_new(TYPE_F32);
    }
    if (strcmp(c, "double") == 0 || strcmp(c, "f64") == 0)
    {
        return type_new(TYPE_F64);
    }
    if (strcmp(c, "I128") == 0 || strcmp(c, "i128") == 0)
    {
        return type_new(TYPE_I128);
    }
    if (strcmp(c, "U128") == 0 || strcmp(c, "u128") == 0)
    {
        return type_new(TYPE_U128);
    }
    if (strcmp(c, "rune") == 0)
    {
        return type_new(TYPE_RUNE);
    }
    if (strcmp(c, "uint") == 0)
    {
        return type_new(TYPE_UINT);
    }

    if (strcmp(c, "byte") == 0)
    {
        return type_new(TYPE_BYTE);
    }
    if (strcmp(c, "usize") == 0)
    {
        return type_new(TYPE_USIZE);
    }
    if (strcmp(c, "isize") == 0)
    {
        return type_new(TYPE_ISIZE);
    }

    char *name = sanitize_mangled_name(c);
    Type *n = type_from_name(ctx, name);
    zfree(name);
    return n;
}

char *join_with_commas(char **parts, int count)
{
    size_t size = 1;
    for (int i = 0; i < count; i++)
    {
        size += strlen(parts[i]) + 1;
    }
    char *out = xmalloc(size);
    out[0] = '\0';
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            strcat(out, ",");
        }
        strcat(out, parts[i]);
    }
    return out;
}

Bindings bindings_new(const char *params, Type **args, int count)
{
    Bindings b = {0};
    int param_count = 1;
    for (const char *s = params; *s; s++)
    {
        param_count += (*s == ',');
    }
    b.count = param_count < count ? param_count : count;
    b.params = xmalloc(sizeof(char *) * (size_t)param_count);
    const char *start = params;
    for (int i = 0; i < param_count; i++)
    {
        const char *end = strchr(start, ',');
        size_t len = end ? (size_t)(end - start) : strlen(start);
        b.params[i] = xmalloc(len + 1);
        memcpy(b.params[i], start, len);
        b.params[i][len] = '\0';
        start = end ? end + 1 : start + len;
    }
    b.args = args;

    return b;
}

Type *replace_type_formal(ParserContext *ctx, Type *t, const Bindings *b, const char *os,
                          const char *ns)
{
    if (!t)
    {
        return NULL;
    }

    if ((t->kind == TYPE_STRUCT || t->kind == TYPE_GENERIC) && t->name && t->count == 0 && b)
    {
        for (int i = 0; i < b->count; i++)
        {
            if (strcmp(t->name, b->params[i]) == 0)
            {
                return type_clone(b->args[i]);
            }
        }
    }

    Type *n = type_clone(t);

    // The template's own name without arguments (`Self`, a method's receiver) is the
    // instance; with arguments (`Vec<Vec<T>>`), they are substituted below.
    if (os && ns && t->name && t->count == 0 && strcmp(t->name, os) == 0)
    {
        n->name = xstrdup(ns);
        n->kind = TYPE_STRUCT;
    }

    n->inner = replace_type_formal(ctx, t->inner, b, os, ns);
    if (t->count > 0 && t->args)
    {
        n->args = xmalloc(sizeof(Type *) * (size_t)(t->count));
        for (int i = 0; i < t->count; i++)
        {
            n->args[i] = replace_type_formal(ctx, t->args[i], b, os, ns);
        }
    }
    return n;
}

// A piece of a match pattern for an instance: the template's name `os` becomes the
// instance's `ns` (`Maybe::Some` inside `impl Maybe<T>`).
static char *rename_template(const char *text, const char *os, const char *ns)
{
    return os && ns ? replace_in_string(text, os, ns) : xstrdup(text);
}

// A copy of the alternatives of a match case, with rename_template applied to their text.
static MatchPattern *copy_patterns_replacing(MatchPattern *patterns, const char *os, const char *ns)
{
    MatchPattern *head = NULL;
    MatchPattern *last = NULL;
    for (MatchPattern *src = patterns; src; src = src->next)
    {
        MatchPattern *copy = xcalloc(1, sizeof(MatchPattern));
        copy->kind = src->kind;
        copy->is_inclusive = src->is_inclusive;
        copy->name_token = src->name_token;
        copy->scope_token = src->scope_token;
        if (src->name.scope)
        {
            copy->name.scope = rename_template(src->name.scope, os, ns);
        }
        if (src->name.name)
        {
            copy->name.name = rename_template(src->name.name, os, ns);
        }
        if (src->literal)
        {
            copy->literal = rename_template(src->literal, os, ns);
        }
        if (src->range_start)
        {
            copy->range_start = rename_template(src->range_start, os, ns);
        }
        if (src->range_end)
        {
            copy->range_end = rename_template(src->range_end, os, ns);
        }
        if (last)
        {
            last->next = copy;
        }
        else
        {
            head = copy;
        }
        last = copy;
    }
    return head;
}

ASTNode *copy_ast_replacing(ParserContext *ctx, ASTNode *n, const Bindings *b, const char *os,
                            const char *ns)
{
    if (!n)
    {
        return NULL;
    }
    ASTNode *new_node = ast_create(n->kind);
    ASTNode *old_next =
        new_node->next; // Preserve next if ast_create did something (it doesn't currently)
    *new_node = *n;
    new_node->next = old_next; // Restore next before recursion

    new_node->type_info = replace_type_formal(ctx, n->type_info, b, os, ns);

    new_node->next = copy_ast_replacing(ctx, n->next, b, os, ns);

    switch (n->kind)
    {
    case NODE_FUNCTION:
        new_node->func.name = n->func.name ? xstrdup(n->func.name) : NULL;

        new_node->func.count = n->func.count;
        if (n->func.count > 0 && n->func.arg_types)
        {
            new_node->func.arg_types = xmalloc(sizeof(Type *) * (size_t)(n->func.count));
            for (int i = 0; i < n->func.count; i++)
            {
                new_node->func.arg_types[i] =
                    replace_type_formal(ctx, n->func.arg_types[i], b, os, ns);
            }
        }
        else
        {
            new_node->func.arg_types = NULL;
        }

        new_node->func.ret_type_info = replace_type_formal(ctx, n->func.ret_type_info, b, os, ns);

        // Deep copy default values AST if present
        if (n->func.default_values && n->func.count > 0)
        {
            new_node->func.default_values = xmalloc(sizeof(ASTNode *) * (size_t)(n->func.count));
            for (int i = 0; i < n->func.count; i++)
            {
                new_node->func.default_values[i] =
                    copy_ast_replacing(ctx, n->func.default_values[i], b, os, ns);
            }
        }

        new_node->func.body = copy_ast_replacing(ctx, n->func.body, b, os, ns);
        break;
    case NODE_BLOCK:
        new_node->block.statements = copy_ast_replacing(ctx, n->block.statements, b, os, ns);
        break;
    case NODE_RAW_STMT:
        new_node->raw_stmt.content = xstrdup(n->raw_stmt.content);
        break;
    case NODE_VAR_DECL:
    case NODE_FIELD_INIT:
        new_node->var_decl.name = n->var_decl.name ? xstrdup(n->var_decl.name) : NULL;
        new_node->var_decl.type_info = replace_type_formal(ctx, n->var_decl.type_info, b, os, ns);
        new_node->var_decl.init_expr = copy_ast_replacing(ctx, n->var_decl.init_expr, b, os, ns);
        break;
    case NODE_RETURN:
        new_node->ret.value = copy_ast_replacing(ctx, n->ret.value, b, os, ns);
        break;
    case NODE_EXPR_BINARY:
        new_node->binary.left = copy_ast_replacing(ctx, n->binary.left, b, os, ns);
        new_node->binary.right = copy_ast_replacing(ctx, n->binary.right, b, os, ns);
        new_node->binary.op = n->binary.op;
        break;
    case NODE_EXPR_UNARY:
        new_node->unary.op = n->unary.op;
        new_node->unary.operand = copy_ast_replacing(ctx, n->unary.operand, b, os, ns);
        break;
    case NODE_EXPR_CALL:
        new_node->call.callee = copy_ast_replacing(ctx, n->call.callee, b, os, ns);
        new_node->call.args = copy_ast_replacing(ctx, n->call.args, b, os, ns);
        new_node->call.arg_names = n->call.arg_names; // Share pointer (shallow copy)
        new_node->call.count = n->call.count;
        break;
    case NODE_EXPR_VAR:
    {
        if (n->var_ref.instance.name)
        {
            FunctionInstance *instance = &new_node->var_ref.instance;
            instance->args = xmalloc(sizeof(Type *) * (size_t)instance->count);
            for (int i = 0; i < instance->count; i++)
            {
                instance->args[i] =
                    replace_type_formal(ctx, n->var_ref.instance.args[i], b, os, ns);
            }
            new_node->var_ref.name = function_instance_name(*instance);
            break;
        }
        if (n->var_ref.owner)
        {
            Type *owner = replace_type_formal(ctx, n->var_ref.owner, b, os, ns);
            char *owner_name = type_mangle_spelling(owner);
            new_node->var_ref.owner = owner;
            if (n->var_ref.variant.member)
            {
                new_node->var_ref.variant.owner = owner;
                new_node->var_ref.name = mangle_variant_symbol(owner_name, n->var_ref.member);
            }
            else
            {
                new_node->var_ref.name = mangle_method_symbol(owner_name, NULL, n->var_ref.member);
            }
            break;
        }
        new_node->var_ref.name = n->var_ref.name ? xstrdup(n->var_ref.name) : NULL;
    }
    break;
    case NODE_FIELD:
        new_node->field.name = n->field.name ? xstrdup(n->field.name) : NULL;
        break;
    case NODE_EXPR_LITERAL:
        if (n->literal.kind == LITERAL_STRING)
        {
            new_node->literal.string_val =
                n->literal.string_val ? xstrdup(n->literal.string_val) : NULL;
        }
        break;
    case NODE_EXPR_MEMBER:
        new_node->member.target = copy_ast_replacing(ctx, n->member.target, b, os, ns);
        new_node->member.field = n->member.field ? xstrdup(n->member.field) : NULL;
        break;
    case NODE_EXPR_INDEX:
        new_node->index.array = copy_ast_replacing(ctx, n->index.array, b, os, ns);
        new_node->index.index = copy_ast_replacing(ctx, n->index.index, b, os, ns);
        break;
    case NODE_EXPR_CAST:
        new_node->cast.expr = copy_ast_replacing(ctx, n->cast.expr, b, os, ns);
        break;
    case NODE_EXPR_STRUCT_INIT:
    {
        // The literal's type is substituted above: `T{}` with T a pointer or a primitive is a
        // zero.
        Type *literal_type = new_node->type_info;
        if (!n->struct_init.fields && literal_type &&
            (literal_type->kind == TYPE_POINTER ||
             is_primitive_type_name(type_mangle_spelling(literal_type))))
        {
            new_node->kind = NODE_EXPR_LITERAL;
            new_node->literal.kind = LITERAL_INT;
            new_node->literal.int_val = 0;
        }
        else
        {
            new_node->struct_init.struct_name = type_def_name(literal_type);
            ASTNode *h = NULL, *t = NULL, *curr = n->struct_init.fields;
            while (curr)
            {
                ASTNode *cp = copy_ast_replacing(ctx, curr, b, os, ns);
                cp->next = NULL;
                if (!h)
                {
                    h = cp;
                }
                else
                {
                    t->next = cp;
                }
                t = cp;
                curr = curr->next;
            }
            new_node->struct_init.fields = h;
        }
        break;
    }
    case NODE_IF:
        new_node->if_stmt.condition = copy_ast_replacing(ctx, n->if_stmt.condition, b, os, ns);
        new_node->if_stmt.then_body = copy_ast_replacing(ctx, n->if_stmt.then_body, b, os, ns);
        new_node->if_stmt.else_body = copy_ast_replacing(ctx, n->if_stmt.else_body, b, os, ns);
        break;
    case NODE_WHILE:
        new_node->while_stmt.condition =
            copy_ast_replacing(ctx, n->while_stmt.condition, b, os, ns);
        new_node->while_stmt.body = copy_ast_replacing(ctx, n->while_stmt.body, b, os, ns);
        break;
    case NODE_FOR:
        new_node->for_stmt.init = copy_ast_replacing(ctx, n->for_stmt.init, b, os, ns);
        new_node->for_stmt.condition = copy_ast_replacing(ctx, n->for_stmt.condition, b, os, ns);
        new_node->for_stmt.step = copy_ast_replacing(ctx, n->for_stmt.step, b, os, ns);
        new_node->for_stmt.body = copy_ast_replacing(ctx, n->for_stmt.body, b, os, ns);
        break;
    case NODE_FOR_RANGE:
        new_node->for_range.start = copy_ast_replacing(ctx, n->for_range.start, b, os, ns);
        new_node->for_range.end = copy_ast_replacing(ctx, n->for_range.end, b, os, ns);
        new_node->for_range.body = copy_ast_replacing(ctx, n->for_range.body, b, os, ns);
        break;

    case NODE_MATCH_CASE:
        new_node->match_case.patterns = copy_patterns_replacing(n->match_case.patterns, os, ns);
        new_node->match_case.binding_count = n->match_case.binding_count;
        if (n->match_case.binding_names)
        {
            new_node->match_case.binding_names =
                xmalloc(sizeof(char *) * (size_t)(n->match_case.binding_count));
            for (int i = 0; i < n->match_case.binding_count; i++)
            {
                if (n->match_case.binding_names[i])
                {
                    new_node->match_case.binding_names[i] = xstrdup(n->match_case.binding_names[i]);
                }
                else
                {
                    new_node->match_case.binding_names[i] = NULL;
                }
            }
        }
        if (n->match_case.binding_refs)
        {
            new_node->match_case.binding_refs =
                xmalloc(sizeof(int) * (size_t)(n->match_case.binding_count));
            memcpy(new_node->match_case.binding_refs, n->match_case.binding_refs,
                   sizeof(int) * (size_t)(n->match_case.binding_count));
        }
        new_node->match_case.is_default = n->match_case.is_default;
        new_node->match_case.is_destructuring = n->match_case.is_destructuring;

        new_node->match_case.body = copy_ast_replacing(ctx, n->match_case.body, b, os, ns);
        if (n->match_case.guard)
        {
            new_node->match_case.guard = copy_ast_replacing(ctx, n->match_case.guard, b, os, ns);
        }
        break;

    case NODE_IMPL:
        // instantiate_methods names the copy after the instance.
        new_node->impl.target = n->impl.target;
        new_node->impl.methods = copy_ast_replacing(ctx, n->impl.methods, b, os, ns);
        break;
    case NODE_IMPL_TRAIT:
        new_node->impl_trait.trait_name =
            n->impl_trait.trait_name ? xstrdup(n->impl_trait.trait_name) : NULL;
        new_node->impl_trait.target = n->impl_trait.target;
        new_node->impl_trait.methods = copy_ast_replacing(ctx, n->impl_trait.methods, b, os, ns);
        break;
    case NODE_TYPEOF:
    case NODE_EXPR_SIZEOF:
        new_node->size_of.expr = copy_ast_replacing(ctx, n->size_of.expr, b, os, ns);
        new_node->size_of.is_type = n->size_of.is_type;
        if (n->size_of.target_type_info)
        {
            new_node->size_of.target_type_info =
                replace_type_formal(ctx, n->size_of.target_type_info, b, os, ns);
        }
        break;
    case NODE_LAMBDA:
        // Use a new lambda ID for each instantiation to ensure unique C function names
        new_node->lambda.lambda_id = ctx->lambda_counter++;
        new_node->lambda.count = n->lambda.count;
        if (n->lambda.count > 0)
        {
            new_node->lambda.param_names = xmalloc(sizeof(char *) * (size_t)(n->lambda.count));
            for (int i = 0; i < n->lambda.count; i++)
            {
                new_node->lambda.param_names[i] = xstrdup(n->lambda.param_names[i]);
            }
        }
        new_node->lambda.capture_count = n->lambda.capture_count;
        if (n->lambda.capture_count > 0)
        {
            new_node->lambda.captured_vars =
                xmalloc(sizeof(char *) * (size_t)(n->lambda.capture_count));
            new_node->lambda.captured_types_info =
                xmalloc(sizeof(Type *) * (size_t)(n->lambda.capture_count));
            if (n->lambda.capture_modes)
            {
                new_node->lambda.capture_modes =
                    xmalloc(sizeof(int) * (size_t)(n->lambda.capture_count));
            }

            for (int i = 0; i < n->lambda.capture_count; i++)
            {
                new_node->lambda.captured_vars[i] = xstrdup(n->lambda.captured_vars[i]);
                new_node->lambda.captured_types_info[i] =
                    replace_type_formal(ctx, n->lambda.captured_types_info[i], b, os, ns);
                if (n->lambda.capture_modes)
                {
                    new_node->lambda.capture_modes[i] = n->lambda.capture_modes[i];
                }
            }
        }
        new_node->lambda.body = copy_ast_replacing(ctx, n->lambda.body, b, os, ns);
        new_node->lambda.is_bare = n->lambda.is_bare;
        register_lambda(ctx, new_node);
        break;
    case NODE_DESTRUCT_VAR:
        if (n->destruct.count > 0)
        {
            new_node->destruct.names = xmalloc(sizeof(char *) * (size_t)(n->destruct.count));
            new_node->destruct.type_infos = xmalloc(sizeof(Type *) * (size_t)(n->destruct.count));
            if (n->destruct.field_names)
            {
                new_node->destruct.field_names =
                    xmalloc(sizeof(char *) * (size_t)(n->destruct.count));
            }

            for (int i = 0; i < n->destruct.count; i++)
            {
                new_node->destruct.names[i] = xstrdup(n->destruct.names[i]);
                new_node->destruct.type_infos[i] =
                    replace_type_formal(ctx, n->destruct.type_infos[i], b, os, ns);
                if (n->destruct.field_names && n->destruct.field_names[i])
                {
                    new_node->destruct.field_names[i] = xstrdup(n->destruct.field_names[i]);
                }
                else if (n->destruct.field_names)
                {
                    new_node->destruct.field_names[i] = NULL;
                }
            }
        }
        new_node->destruct.init_expr = copy_ast_replacing(ctx, n->destruct.init_expr, b, os, ns);
        new_node->destruct.struct_name =
            n->destruct.struct_name ? rename_template(n->destruct.struct_name, os, ns) : NULL;
        new_node->destruct.else_block = copy_ast_replacing(ctx, n->destruct.else_block, b, os, ns);
        break;
    case NODE_MATCH:
        new_node->match_stmt.expr = copy_ast_replacing(ctx, n->match_stmt.expr, b, os, ns);
        new_node->match_stmt.cases = copy_ast_replacing(ctx, n->match_stmt.cases, b, os, ns);
        break;
    case NODE_LOOP:
        new_node->loop_stmt.body = copy_ast_replacing(ctx, n->loop_stmt.body, b, os, ns);
        break;
    case NODE_BREAK:
    case NODE_CONTINUE:
        // No members to copy besides next (handled at end)
        break;
    case NODE_EXPR_ARRAY_LITERAL:
        new_node->array_literal.elements =
            copy_ast_replacing(ctx, n->array_literal.elements, b, os, ns);
        new_node->array_literal.count = n->array_literal.count;
#if FF_COMPATIBILITY_0_5
        new_node->array_literal.returned_as_slice = n->array_literal.returned_as_slice;
#endif
        break;
    case NODE_EXPR_TUPLE_LITERAL:
        new_node->tuple_literal.elements =
            copy_ast_replacing(ctx, n->tuple_literal.elements, b, os, ns);
        new_node->tuple_literal.count = n->tuple_literal.count;
        break;
    case NODE_EXPR_SLICE:
        new_node->slice.array = copy_ast_replacing(ctx, n->slice.array, b, os, ns);
        new_node->slice.start = copy_ast_replacing(ctx, n->slice.start, b, os, ns);
        new_node->slice.end = copy_ast_replacing(ctx, n->slice.end, b, os, ns);
        break;
    case NODE_EXPECT:
    case NODE_ASSERT:
        new_node->assert_stmt.condition =
            copy_ast_replacing(ctx, n->assert_stmt.condition, b, os, ns);
        new_node->assert_stmt.message =
            n->assert_stmt.message ? xstrdup(n->assert_stmt.message) : NULL;
        new_node->assert_stmt.message_fstring =
            copy_ast_replacing(ctx, n->assert_stmt.message_fstring, b, os, ns);
        break;
    case NODE_DEFER:
        new_node->defer_stmt.stmt = copy_ast_replacing(ctx, n->defer_stmt.stmt, b, os, ns);
        break;
    case NODE_EXPR_FSTRING:
        new_node->fstring.parts = copy_ast_replacing(ctx, n->fstring.parts, b, os, ns);
        break;
    case NODE_FSTRING_PART:
        new_node->fstring_part.expr = copy_ast_replacing(ctx, n->fstring_part.expr, b, os, ns);
        break;
    case NODE_TERNARY:
        new_node->ternary.cond = copy_ast_replacing(ctx, n->ternary.cond, b, os, ns);
        new_node->ternary.true_expr = copy_ast_replacing(ctx, n->ternary.true_expr, b, os, ns);
        new_node->ternary.false_expr = copy_ast_replacing(ctx, n->ternary.false_expr, b, os, ns);
        break;
    case NODE_ASM:
        new_node->asm_stmt.code = n->asm_stmt.code ? xstrdup(n->asm_stmt.code) : NULL;
        new_node->asm_stmt.is_volatile = n->asm_stmt.is_volatile;
        new_node->asm_stmt.output_count = n->asm_stmt.output_count;
        new_node->asm_stmt.input_count = n->asm_stmt.input_count;
        new_node->asm_stmt.clobber_count = n->asm_stmt.clobber_count;
        // ASM usually doesn't contain generic parameters in constraints, but we could harden here
        // if needed
        break;
    case NODE_GOTO:
        new_node->goto_stmt.label_name =
            n->goto_stmt.label_name ? xstrdup(n->goto_stmt.label_name) : NULL;
        break;
    case NODE_LABEL:
        new_node->label_stmt.label_name =
            n->label_stmt.label_name ? xstrdup(n->label_stmt.label_name) : NULL;
        break;
    case NODE_TRY:
        new_node->try_stmt.expr = copy_ast_replacing(ctx, n->try_stmt.expr, b, os, ns);
        break;
    case NODE_REFLECTION:
        new_node->reflection.kind = n->reflection.kind;
        new_node->reflection.target_type =
            replace_type_formal(ctx, n->reflection.target_type, b, os, ns);
        break;
    case NODE_REPL_PRINT:
        new_node->repl_print.expr = copy_ast_replacing(ctx, n->repl_print.expr, b, os, ns);
        break;
    case NODE_CUDA_LAUNCH:
        new_node->cuda_launch.call = copy_ast_replacing(ctx, n->cuda_launch.call, b, os, ns);
        new_node->cuda_launch.grid = copy_ast_replacing(ctx, n->cuda_launch.grid, b, os, ns);
        new_node->cuda_launch.block = copy_ast_replacing(ctx, n->cuda_launch.block, b, os, ns);
        new_node->cuda_launch.shared_mem =
            copy_ast_replacing(ctx, n->cuda_launch.shared_mem, b, os, ns);
        new_node->cuda_launch.stream = copy_ast_replacing(ctx, n->cuda_launch.stream, b, os, ns);
        break;
    case NODE_VA_START:
        new_node->va_start_args.ap = copy_ast_replacing(ctx, n->va_start_args.ap, b, os, ns);
        new_node->va_start_args.last_arg =
            copy_ast_replacing(ctx, n->va_start_args.last_arg, b, os, ns);
        break;
    case NODE_VA_END:
        new_node->va_end_args.ap = copy_ast_replacing(ctx, n->va_end_args.ap, b, os, ns);
        break;
    case NODE_VA_COPY:
        new_node->va_copy_args.dest = copy_ast_replacing(ctx, n->va_copy_args.dest, b, os, ns);
        new_node->va_copy_args.src = copy_ast_replacing(ctx, n->va_copy_args.src, b, os, ns);
        break;
    case NODE_VA_ARG:
        new_node->va_arg_val.ap = copy_ast_replacing(ctx, n->va_arg_val.ap, b, os, ns);
        new_node->va_arg_val.type_info =
            replace_type_formal(ctx, n->va_arg_val.type_info, b, os, ns);
        break;
    default:
        break;
    }
    return new_node;
}

ASTNode *copy_default_value(ParserContext *ctx, FuncSig *sig, int index, Token call)
{
    ASTNode *def = sig->default_values[index];
    char **refs = NULL;
    int ref_count = 0;
    find_var_refs(def, &refs, &ref_count);
    for (int i = 0; i < ref_count; i++)
    {
        ZenSymbol *s = find_symbol_entry(ctx, refs[i]);
        if (s && s->is_local)
        {
            zerror_at(call, "A default value of '%s' uses '%s', which is a local variable here",
                      sig->own_name ? sig->own_name : sig->name, refs[i]);
        }
    }
    return copy_ast_replacing(ctx, def, NULL, NULL, NULL);
}
