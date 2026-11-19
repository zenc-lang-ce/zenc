// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include "ast/ast.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int token_is_field_name(Token t)
{
    if (t.kind == TOK_IDENT || t.kind == TOK_INT)
    {
        return 1;
    }
    if (!t.start || t.len == 0)
    {
        return 0;
    }
    char c = t.start[0];
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int token_is_only_stars(Token t)
{
    if (t.kind != TOK_OP || t.len == 0)
    {
        return 0;
    }
    for (const char *c = t.start; c < t.start + t.len; c++)
    {
        if (*c != '*')
        {
            return 0;
        }
    }
    return 1;
}

int tokens_could_be_generic_args(Lexer *l)
{
    Lexer scan = *l;
    lexer_next(&scan); // eat <

    Token first = lexer_peek(&scan);
    if (first.kind != TOK_IDENT && first.kind != TOK_LPAREN && first.kind != TOK_LBRACKET)
    {
        return 0;
    }

    int angles = 1;
    int parens = 0;
    int brackets = 0;
    while (1)
    {
        Token tk = lexer_next(&scan);
        if (tk.kind == TOK_EOF || tk.kind == TOK_LBRACE || tk.kind == TOK_RBRACE)
        {
            return 0;
        }

        // An array size is an expression: accept anything up to the matching ']'.
        if (brackets > 0)
        {
            if (tk.kind == TOK_LBRACKET)
            {
                brackets++;
            }
            else if (tk.kind == TOK_RBRACKET)
            {
                brackets--;
            }
            continue;
        }

        switch (tk.kind)
        {
        default:
            // Literals, `{`, `;`, `=`, `&&`, `.`... cannot appear in a type list.
            return 0;
        case TOK_IDENT:
        case TOK_COMMA:
        case TOK_DCOLON:
        case TOK_ELLIPSIS:
            break;
        case TOK_LANGLE:
            angles++;
            break;
        case TOK_RANGLE:
            angles--;
            if (angles == 0)
            {
                return parens == 0;
            }
            break;
        case TOK_LPAREN:
            parens++;
            break;
        case TOK_RPAREN:
            if (parens == 0)
            {
                return 0;
            }
            parens--;
            break;
        case TOK_LBRACKET:
            brackets++;
            break;
        case TOK_ARROW:
            break;
        case TOK_OP:
            if (token_is_only_stars(tk))
            {
                break;
            }
            if (tk.len == 2 && strncmp(tk.start, ">>", 2) == 0)
            {
                angles -= 2;
                if (angles == 0)
                {
                    return parens == 0;
                }
                if (angles < 0)
                {
                    return 0;
                }
                break;
            }
            return 0;
        }
    }
}

int check_opaque_alias_compat(ParserContext *ctx, Type *a, Type *b)
{
    if (!a || !b)
    {
        return 0;
    }

    int a_is_opaque = (a->kind == TYPE_ALIAS && a->alias.is_opaque_alias);
    int b_is_opaque = (b->kind == TYPE_ALIAS && b->alias.is_opaque_alias);

    if (!a_is_opaque && !b_is_opaque)
    {
        return 1;
    }

    // Two opaque aliases are the same type only if they are the same alias, even
    // in the module that defines both: there each converts to its underlying
    // type, not to the other.
    if (a_is_opaque && b_is_opaque)
    {
        return a->name && b->name && strcmp(a->name, b->name) == 0;
    }

    // One opaque alias converts to and from its underlying type only where that is what it is.
    Type *seen = type_seen_from(a_is_opaque ? a : b, ctx->current_filename);
    return !(seen->kind == TYPE_ALIAS && seen->alias.is_opaque_alias);
}

#include "../zen/zen_facts.h"
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include "../constants.h"
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include "parser.h"
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include <ctype.h>
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include <libgen.h>
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include <stdio.h>
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"

#include <stdlib.h>
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"
#include <string.h>
#include "constants.h"
#include "expr_internal.h"
#include "analysis/move_check.h"
#include "utils/utils.h"

ASTNode *find_function_definition(ParserContext *ctx, const char *name)
{
    StructRef *curr = ctx->parsed_funcs_list;
    while (curr)
    {
        if (curr->node && function_c_name(curr->node) &&
            strcmp(function_c_name(curr->node), name) == 0)
        {
            return curr->node;
        }
        curr = curr->next;
    }
    return NULL;
}

// Kinds the type check of `let` lets through whatever the other side is.
static int skips_value_check(Type *t)
{
    return t->kind == TYPE_POINTER || t->kind == TYPE_FUNCTION || t->kind == TYPE_STRUCT ||
           t->kind == TYPE_ARRAY;
}

// The instance of a generic struct or enum a type names once non-opaque aliases are followed
// (`Handle<PropKind>` for `alias PropId = Handle<PropKind>;`), or NULL.
static Type *instance_type(Type *t)
{
    while (t && t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        t = t->inner;
    }
    if (!t || (t->kind != TYPE_STRUCT && t->kind != TYPE_ENUM) || !t->name || t->count == 0)
    {
        return NULL;
    }
    return t;
}

// Two instances of one generic with different arguments (`Handle<A>` and
// `Handle<B>`) are different types even when their fields are the same, also
// behind pointers (where C only warns, and `-w` hides it).
static void check_generic_instance_argument(ParserContext *ctx, Type *expected, ASTNode *arg)
{
    Type *want_type = expected;
    Type *have_type = arg->type_info;
    int depth = 0;
    while (want_type && have_type && want_type->kind == TYPE_POINTER &&
           have_type->kind == TYPE_POINTER && depth < 8)
    {
        want_type = want_type->inner;
        have_type = have_type->inner;
        depth++;
    }
    Type *want = instance_type(want_type);
    Type *have = instance_type(have_type);
    if (!want || !have || strcmp(want->name, have->name) != 0 ||
        strcmp(type_def_name(want), type_def_name(have)) == 0)
    {
        return;
    }
    static const char STARS[] = "********";
    const char *stars = STARS + (8 - depth);
    char want_text[MAX_SHORT_MSG_LEN];
    char have_text[MAX_SHORT_MSG_LEN];
    snprintf(want_text, sizeof(want_text), "%s%s", zen_c_type_spelling(ctx, want), stars);
    snprintf(have_text, sizeof(have_text), "%s%s", zen_c_type_spelling(ctx, have), stars);
    zpanic_at(arg->token, "Type validation failed. Expected '%s', but got '%s'", want_text,
              have_text);
    return;
}

void check_argument_type(ParserContext *ctx, Type *expected, ASTNode *arg)
{
    Type *got = arg ? arg->type_info : NULL;
    if (!expected || !got || expected->kind == TYPE_GENERIC || got->kind == TYPE_GENERIC)
    {
        return;
    }
    check_generic_instance_argument(ctx, expected, arg);
    int expected_opaque = expected->kind == TYPE_ALIAS && expected->alias.is_opaque_alias;
    int got_opaque = got->kind == TYPE_ALIAS && got->alias.is_opaque_alias;
    if (!expected_opaque && !got_opaque)
    {
        return;
    }
    if (skips_value_check(expected) || skips_value_check(got))
    {
        return;
    }
    if (type_eq(expected, got) || check_opaque_alias_compat(ctx, expected, got))
    {
        return;
    }
    char *want = zen_c_type_spelling(ctx, expected);
    char *have = zen_c_type_spelling(ctx, got);
    zpanic_at(arg->token, "Type validation failed. Expected '%s', but got '%s'", want, have);
    return;
}

CallArgs parse_call_args(ParserContext *ctx, Lexer *l, FuncSig *sig)
{
    CallArgs res = {NULL, NULL, NULL, 0, 0};
    (void)ctx;

    if (lexer_peek(l).kind != TOK_RPAREN)
    {
        while (1)
        {
            if (lexer_peek(l).kind == TOK_EOF)
            {
                break;
            }
            char *arg_name = NULL;
            Token t1 = lexer_peek(l);
            if (t1.kind == TOK_IDENT)
            {
                Token t2 = lexer_peek2(l);
                if (t2.kind == TOK_COLON)
                {
                    arg_name = token_strdup(t1);
                    res.has_named = 1;
                    lexer_next(l); // eat IDENT
                    lexer_next(l); // eat :
                }
            }

            ASTNode *arg = parse_expression(ctx, l);
            if (sig && sig->arg_types && res.count < sig->total_args && arg && !res.has_named)
            {
                check_argument_type(ctx, sig->arg_types[res.count], arg);
            }

            if (!res.head)
            {
                res.head = arg;
                res.tail = arg;
            }
            else
            {
                res.tail->next = arg;
                res.tail = arg;
            }

            res.arg_names = xrealloc(res.arg_names, (size_t)(res.count + 1) * sizeof(char *));
            res.arg_names[res.count] = arg_name;
            res.count++;

            if (lexer_peek(l).kind == TOK_COMMA)
            {
                lexer_next(l);
            }
            else
            {
                break;
            }
        }
    }

    return res;
}

void validate_named_arguments(Token call_token, const char *func_name, char **arg_names,
                              int args_count, ASTNode *func_def)
{
    if (!func_def || !arg_names)
    {
        (void)func_name;
        return;
    }

    for (int i = 0; i < args_count; i++)
    {
        // Skip positional arguments (NULL name)
        if (!arg_names[i])
        {
            continue;
        }

        // Check bounds
        if (i >= func_def->func.count)
        {
            continue;
        }

        // Check parameter name match
        const char *expected_name = func_def->func.param_names[i];
        if (!expected_name)
        {
            continue;
        }

        if (strcmp(arg_names[i], expected_name) != 0)
        {
            char msg[MAX_SHORT_MSG_LEN];
            snprintf(
                msg, sizeof(msg),
                "Named arguments must follow function parameter order. Expected '%s' but got '%s'",
                expected_name, arg_names[i]);
            zpanic_at(call_token, "%s", msg);
            return;
        }
    }
}

// Helper to check if a type is a struct type

// The sscanf conversion that writes a value of type `t`: its length modifier
// must match the size of the variable, or the value overwrites what follows it.
static const char *scan_conversion(Type *t)
{
    if (!t)
    {
        return "%d";
    }
    switch (t->kind)
    {
    case TYPE_BOOL:
        return "%hhu";
    case TYPE_CHAR:
    case TYPE_I8:
    case TYPE_U8:
    case TYPE_BYTE:
    case TYPE_C_CHAR:
    case TYPE_C_UCHAR:
        return " %c"; // The space skips whitespace
    case TYPE_I16:
    case TYPE_C_SHORT:
        return "%hd";
    case TYPE_U16:
    case TYPE_C_USHORT:
        return "%hu";
    case TYPE_U32:
    case TYPE_C_UINT:
        return "%u";
    case TYPE_I64:
    case TYPE_C_LONGLONG:
        return "%lld";
    case TYPE_U64:
    case TYPE_C_ULONGLONG:
        return "%llu";
    case TYPE_C_LONG:
        return "%ld";
    case TYPE_C_ULONG:
        return "%lu";
    case TYPE_ISIZE:
        return "%td";
    case TYPE_USIZE:
        return "%zu";
    case TYPE_F32:
        return "%f";
    case TYPE_F64:
        return "%lf";
    case TYPE_STRING:
        // A pointer gets a new string with the word (`_z_scan_helper` does the
        // `%ms`); a `%s` would write the text over the pointer.
        return "%ms";
    case TYPE_POINTER:
        return t->inner && t->inner->kind == TYPE_CHAR ? "%ms" : "%d";
    case TYPE_ARRAY:
        if (t->inner &&
            (t->inner->kind == TYPE_CHAR || t->inner->kind == TYPE_U8 || t->inner->kind == TYPE_I8))
        {
            return "%s";
        }
        return "%d";
    default:
        return "%d";
    }
}

char *infer_scan_format(ParserContext *ctx, ASTNode **args, int ac)
{
    char *fmt = xmalloc(MAX_SHORT_MSG_LEN);
    fmt[0] = 0;
    for (int i = 0; i < ac; i++)
    {
        Type *inner_t = args[i]->type_info;
        if (!inner_t && args[i]->kind == NODE_EXPR_VAR)
        {
            inner_t = find_symbol_type_info(ctx, args[i]->var_ref.name);
        }
        strcat(fmt, scan_conversion(inner_t));
        if (i < ac - 1)
        {
            strcat(fmt, " ");
        }
    }
    return fmt;
}

int slice_follows(Lexer *l)
{
    ZenTokenType k = lexer_peek(l).kind;
    return k == TOK_DOTDOT || k == TOK_DOTDOT_LT;
}

// A literal slice bound past the end of a fixed-size array (`a[2..9]` of an `int[5]`).
static void check_slice_bound(ASTNode *array, ASTNode *bound)
{
    Type *t = array->type_info;
    if (bound && bound->kind == NODE_EXPR_LITERAL && bound->literal.kind == LITERAL_INT && t &&
        t->kind == TYPE_ARRAY && t->array_size > 0)
    {
        if (bound->literal.int_val > (unsigned long long)t->array_size)
        {
            warn_slice_bounds(bound->token, (int)bound->literal.int_val, t->array_size);
        }
    }
}

ASTNode *parse_slice_rest(ParserContext *ctx, Lexer *l, ASTNode *array, ASTNode *start, Token token)
{
    lexer_next(l); // `..` or `..<`
    ASTNode *end = lexer_peek(l).kind != TOK_RBRACKET ? parse_expression(ctx, l) : NULL;
    if (lexer_next(l).kind != TOK_RBRACKET)
    {
        zpanic_at(lexer_peek(l), "Expected ]");
        return NULL;
    }
    check_slice_bound(array, start);
    check_slice_bound(array, end);

    ASTNode *node = ast_create(NODE_EXPR_SLICE);
    node->token = token;
    node->slice.array = array;
    node->slice.start = start;
    node->slice.end = end;

    // `a[i..j]` is a slice of the elements of `a`.
    if (array->type_info)
    {
        Type *inner = slice_element(array->type_info);
        if (!inner &&
            (array->type_info->kind == TYPE_ARRAY || array->type_info->kind == TYPE_POINTER))
        {
            inner = array->type_info->inner;
        }
        if (inner)
        {
            node->type_info = instantiate_slice(ctx, inner, token);
        }
    }
    return node;
}
