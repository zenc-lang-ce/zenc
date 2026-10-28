// SPDX-License-Identifier: MIT
// f-strings: each piece is written with one printf-like call, into a buffer when the string
// is a value or straight to a stream when it is printed. The parser only splits the pieces
// (parse_fstring_parts); the format of each `{expr}` is chosen here from its type.

#include "../parser/parser.h"
#include "codegen.h"
#include "codegen_internal.h"
#include "../utils/format_expr.h"
#include "../constants.h"
#include <stdio.h>
#include <string.h>

// The start of the C call that writes one piece: to the stream when it is printed, or at the
// end of its buffer when it is used as a value. Each piece is then
// `<writer>"<format>", <arguments>);`. snprintf only gets the space left in the buffer, so a
// text longer than the buffer is cut instead of written past its end.
static char *fs_writer(int fs_id, const char *stream)
{
    char buf[256];
    if (stream)
    {
        snprintf(buf, sizeof(buf), "fprintf(%s, ", stream);
    }
    else
    {
        snprintf(buf, sizeof(buf),
                 "snprintf(_fs_buf_%d + strlen(_fs_buf_%d), "
                 "sizeof(_fs_buf_%d) - strlen(_fs_buf_%d), ",
                 fs_id, fs_id, fs_id, fs_id);
    }
    return xstrdup(buf);
}

// A struct with `data` and `len` fields whose data is characters: written as `%.*s`.
static int is_char_data_len_struct(ASTNode *def)
{
    if (!def || def->kind != NODE_STRUCT)
    {
        return 0;
    }
    int has_len = 0;
    char *data_type = NULL;
    ASTNode *field = def->strct.fields;
    while (field)
    {
        if (strcmp(field->field.name, "data") == 0)
        {
            data_type = field->field.type;
        }
        else if (strcmp(field->field.name, "len") == 0)
        {
            has_len = 1;
        }
        field = field->next;
    }
    return has_len && data_type &&
           (strstr(data_type, "char") || strstr(data_type, "u8") || strstr(data_type, "byte"));
}

// Whether a value of type `t` (or a pointer to one, *is_ptr), after aliases, is a slice or a
// struct with character `data` and `len`.
static int is_char_data_len_type(ParserContext *ctx, Type *t, int *is_ptr)
{
    Type *base = t;
    *is_ptr = 0;
    if (base->kind == TYPE_POINTER)
    {
        base = base->inner;
        *is_ptr = 1;
    }

    while (base)
    {
        if (base->kind == TYPE_ALIAS)
        {
            base = base->inner;
        }
        else if (base->kind == TYPE_STRUCT && base->name)
        {
            TypeAlias *ta = find_type_alias_node(ctx, base->name);
            if (!ta || !ta->type_info)
            {
                break;
            }
            base = ta->type_info;
        }
        else
        {
            break;
        }
    }

    if (base && base->kind == TYPE_ARRAY && base->array_size == 0)
    {
        char *inner_name = type_to_string(base->inner);
        char slice_name[MAX_TYPE_NAME_LEN];
        snprintf(slice_name, sizeof(slice_name), "Slice__%s", inner_name);
        zfree(inner_name);
        return is_char_data_len_struct(find_struct_def(ctx, slice_name));
    }
    if (base && base->kind == TYPE_STRUCT && base->name)
    {
        return is_char_data_len_struct(find_struct_def(ctx, base->name));
    }
    return 0;
}

// One `{expr}`. A temporary that is not of a simple type is first stored in `_z_interp_val`
// and dropped after it is written.
static void emit_fstring_expr(ParserContext *ctx, ASTNode *part, const char *writer)
{
    ASTNode *expr = part->fstring_part.expr;
    Type *t = expr->type_info;

    int is_temporary = !(expr->kind == NODE_EXPR_VAR || expr->kind == NODE_EXPR_MEMBER ||
                         expr->kind == NODE_EXPR_INDEX);
    int is_simple_type = t && t->kind != TYPE_STRUCT && t->kind != TYPE_ENUM &&
                         t->kind != TYPE_UNKNOWN && t->kind != TYPE_ALIAS;
    int force_simple = is_simple_type || !is_temporary;

    // A struct (or a pointer to one) with a `to_string` method is written with it.
    char *to_string = NULL;
    int to_string_is_ptr = 0;
    if (t)
    {
        char *struct_name = NULL;
        if (t->kind == TYPE_STRUCT)
        {
            struct_name = t->name;
        }
        else if (t->kind == TYPE_POINTER && t->inner && t->inner->kind == TYPE_STRUCT)
        {
            struct_name = t->inner->name;
            to_string_is_ptr = 1;
        }
        if (struct_name)
        {
            char mangled[MAX_TYPE_NAME_LEN];
            snprintf(mangled, sizeof(mangled), "%s__to_string", struct_name);
            if (find_func(ctx, mangled))
            {
                to_string = xstrdup(mangled);
            }
        }
    }

    char *rw = format_expression_as_c(ctx, expr);
    if (!rw)
    {
        rw = xstrdup("");
    }

    const char *fmt = part->fstring_part.fmt;
    if (fmt)
    {
        if (force_simple)
        {
            EMIT(ctx, "%s\"%%%s\", %s); ", writer, fmt, rw);
        }
        else
        {
            EMIT(ctx,
                 "({ ZC_AUTO_INIT(_z_interp_val, %s); %s\"%%%s\", _z_interp_val); "
                 "_z_drop(_z_interp_val); }); ",
                 rw, writer, fmt);
        }
        return;
    }

    int is_ptr = 0;
    if (t && !to_string && is_char_data_len_type(ctx, t, &is_ptr))
    {
        const char *acc = is_ptr ? "->" : ".";
        if (force_simple)
        {
            EMIT(ctx, "%s\"%%.*s\", (int)(%s)%slen, (%s)%sdata); ", writer, rw, acc, rw, acc);
        }
        else
        {
            EMIT(ctx,
                 "({ ZC_AUTO_INIT(_z_interp_val, %s); %s\"%%.*s\", "
                 "(int)(_z_interp_val)%slen, (_z_interp_val)%sdata); "
                 "_z_drop(_z_interp_val); }); ",
                 rw, writer, acc, acc);
        }
        return;
    }

    const char *format_spec = NULL;
    if (t && t->kind == TYPE_RUNE)
    {
        format_spec = "s";
        char *rune = xmalloc(strlen(rw) + 32);
        sprintf(rune, "_z_str_rune(%s)", rw); /* safe */
        rw = rune;
    }
    else
    {
        // A pointer that is not a string is written as an address.
        char *type_name = t ? type_to_string(t) : part->fstring_part.symbol_type;
        if (type_name && strstr(type_name, "*") && !strstr(type_name, "char") &&
            strcmp(type_name, "string") != 0 && strcmp(type_name, "str") != 0)
        {
            format_spec = "p";
        }
    }

    if (format_spec)
    {
        if (force_simple)
        {
            EMIT(ctx, "%s\"%%%s\", %s); ", writer, format_spec, rw);
        }
        else
        {
            EMIT(ctx,
                 "({ ZC_AUTO_INIT(_z_interp_val, %s); %s\"%%%s\", _z_interp_val); "
                 "_z_drop(_z_interp_val); }); ",
                 rw, writer, format_spec);
        }
    }
    else if (to_string)
    {
        const char *addr = to_string_is_ptr ? "" : "&";
        if (force_simple && !is_temporary)
        {
            EMIT(ctx, "%s\"%%s\", (char*)%s(%s%s)); ", writer, to_string, addr, rw);
        }
        else
        {
            EMIT(ctx,
                 "({ ZC_AUTO_INIT(_z_interp_val, %s); %s\"%%s\", (char*)%s(%s_z_interp_val)); "
                 "_z_drop(_z_interp_val); }); ",
                 rw, writer, to_string, addr);
        }
    }
    else if (force_simple)
    {
        EMIT(ctx, "%s_z_str(%s), _z_arg(%s)); ", writer, rw, rw);
    }
    else
    {
        EMIT(ctx,
             "({ ZC_AUTO_INIT(_z_interp_val, %s); %s_z_str(_z_interp_val), "
             "_z_arg(_z_interp_val)); _z_drop(_z_interp_val); }); ",
             rw, writer);
    }
}

void emit_fstring(ParserContext *ctx, ASTNode *node, const char *stream, int newline)
{
    int fs_id = ctx->cg.fstring_counter++;
    EMIT(ctx, "({ ");
    if (!stream)
    {
        EMIT(ctx, "static char _fs_buf_%d[8192]; _fs_buf_%d[0]=0; ", fs_id, fs_id);
    }

    char *writer = fs_writer(fs_id, stream);
    ASTNode *part = node->fstring.parts;
    while (part)
    {
        if (part->fstring_part.text)
        {
            char *escaped = escape_c_string(part->fstring_part.text);
            EMIT(ctx, "%s\"%%s\", \"%s\"); ", writer, escaped);
            zfree(escaped);
        }
        else
        {
            emit_fstring_expr(ctx, part, writer);
        }
        part = part->next;
    }

    if (newline)
    {
        EMIT(ctx, "%s\"\\n\"); ", writer);
    }
    else if (stream)
    {
        EMIT(ctx, "fflush(stdout); ");
    }

    if (stream)
    {
        EMIT(ctx, "0; })");
    }
    else
    {
        EMIT(ctx, "_fs_buf_%d; })", fs_id);
    }
    zfree(writer);
}

void handle_expr_fstring(ParserContext *ctx, ASTNode *node)
{
    emit_fstring(ctx, node, NULL, 0);
}

int emit_direct_print(ParserContext *ctx, ASTNode *node)
{
    if (node->kind != NODE_EXPR_CALL || !node->call.callee ||
        node->call.callee->kind != NODE_EXPR_VAR || !node->call.args ||
        node->call.args->kind != NODE_EXPR_FSTRING || node->call.args->next)
    {
        return 0;
    }

    static const char *const names[] = {"print", "println", "eprint", "eprintln"};
    const char *name = node->call.callee->var_ref.name;
    int which = -1;
    for (int i = 0; i < 4; i++)
    {
        if (strcmp(name, names[i]) == 0)
        {
            which = i;
        }
    }
    if (which < 0)
    {
        return 0;
    }

    FuncSig *sig = find_func(ctx, name);
    const char *file = sig ? sig->decl_token.file : NULL;
    size_t flen = file ? strlen(file) : 0;
    if (!file || flen < 9 || strcmp(file + flen - 9, "std/io.zc") != 0)
    {
        return 0;
    }

    int newline = (which == 1 || which == 3);
    emit_fstring(ctx, node->call.args, which >= 2 ? "stderr" : "stdout", newline);
    return 1;
}
