// SPDX-License-Identifier: MIT
// The C of `embed "file" as T`: the file's bytes as an initializer of T.

#include "../parser/parser.h"
#include "codegen.h"
#include "codegen_internal.h"
#include <stdio.h>
#include <string.h>

// The C an embed writes spells its types in C.
static char *embed_c_type(ParserContext *ctx, Type *t)
{
    return type_to_c_string(t, ctx->config->misra_mode);
}

void handle_embed(ParserContext *ctx, ASTNode *node)
{
    Type *target_type = node->embed.as_type;
    const unsigned char *b = node->embed.bytes;
    long len = (long)node->embed.len;
    size_t oc = (size_t)(len) * 6 + 256;
    char *o = xmalloc((size_t)(oc));

    int use_cpp_stmt = (ctx->config->use_cpp && node->embed.in_function);

    Type *slice_elem = slice_element(target_type);
    if (slice_elem)
    {
        char *slice_name = embed_c_type(ctx, target_type);
        char *inner_ts = embed_c_type(ctx, slice_elem);
        if (use_cpp_stmt)
        {
            snprintf(o, oc, "({ static const %s __tmp[] = {", inner_ts);
        }
        else if (ctx->config->use_cpp)
        {
            snprintf(o, oc, "(%s){(%s[]){", slice_name, inner_ts);
        }
        else
        {
            snprintf(o, oc, "(%s){.data=(%s[]){", slice_name, inner_ts);
        }
        zfree(inner_ts);
    }
    else
    {
        // Handle specific type
        char *ts = embed_c_type(ctx, target_type);

        if (target_type->kind == TYPE_ARRAY)
        {
            char *inner_ts = embed_c_type(ctx, target_type->inner);
            Type *ptr_type = type_new_ptr(target_type->inner); // Reuse inner
            target_type = ptr_type;
            if (use_cpp_stmt)
            {
                snprintf(o, oc, "({ static const %s __tmp[] = {", inner_ts);
            }
            else
            {
                snprintf(o, oc, "(%s[]){", inner_ts);
            }
            zfree(inner_ts);
        }
        else
        {
            if (strcmp(ts, "string") == 0 || strcmp(ts, "char*") == 0)
            {
                snprintf(o, oc, "(char*)\"");
            }
            else
            {
                snprintf(o, oc, "(%s){", ts);
            }
        }
        zfree(ts);
    }

    size_t cur_len = strlen(o);
    char *p = o + cur_len;

    // Check if string mode
    int is_string = target_type && (target_type->kind == TYPE_STRING ||
                                    (target_type->kind == TYPE_POINTER && target_type->inner &&
                                     target_type->inner->kind == TYPE_CHAR));

    for (int i = 0; i < len; i++)
    {
        if (cur_len + 16 >= oc)
        {
            break;
        }
        if (is_string)
        {
            // Hex escape for string
            int w = snprintf(p, oc - cur_len, "\\x%02X", b[i]);
            p += w;
            cur_len += (size_t)(w);
        }
        else
        {
            int w = snprintf(p, oc - cur_len, "0x%02X,", b[i]);
            p += w;
            cur_len += (size_t)(w);
        }
    }

    if (cur_len + 16 < oc)
    {
        if (is_string)
        {
            snprintf(p, oc - cur_len, "\"");
        }
        else
        {
            if (slice_elem)
            {
                if (use_cpp_stmt)
                {
                    char *ts = embed_c_type(ctx, target_type);
                    if (ctx->config->use_cpp)
                    {
                        char *inner_ts = embed_c_type(ctx, slice_elem);
#if FF_COMPATIBILITY_0_5
                        snprintf(p, oc - cur_len, "}; (%s){(%s*)__tmp, %ld, %ld}; })", ts, inner_ts,
                                 (long)len, (long)len);
#else
                        snprintf(p, oc - cur_len, "}; (%s){(%s*)__tmp, %ld}; })", ts, inner_ts,
                                 (long)len);
#endif
                        zfree(inner_ts);
                    }
                    else
                    {
#if FF_COMPATIBILITY_0_5
                        snprintf(p, oc - cur_len, "}; (%s){.data=__tmp, .len=%ld, .cap=%ld}; })",
                                 ts, (long)len, (long)len);
#else
                        snprintf(p, oc - cur_len, "}; (%s){.data=__tmp, .len=%ld}; })", ts,
                                 (long)len);
#endif
                    }
                    zfree(ts);
                }
                else
                {
                    if (ctx->config->use_cpp)
                    {
#if FF_COMPATIBILITY_0_5
                        snprintf(p, oc - cur_len, "}, %ld, %ld}", (long)len, (long)len);
#else
                        snprintf(p, oc - cur_len, "}, %ld}", (long)len);
#endif
                    }
                    else
                    {
#if FF_COMPATIBILITY_0_5
                        snprintf(p, oc - cur_len, "},.len=%ld,.cap=%ld}", (long)len, (long)len);
#else
                        snprintf(p, oc - cur_len, "},.len=%ld}", (long)len);
#endif
                    }
                }
            }
            else
            {
                if (use_cpp_stmt && target_type && target_type->kind == TYPE_POINTER &&
                    target_type->inner)
                {
                    char *inner_ts = embed_c_type(ctx, target_type->inner);
                    snprintf(p, oc - cur_len, "}; (%s*)__tmp; })", inner_ts);
                    zfree(inner_ts);
                }
                else
                {
                    snprintf(p, oc - cur_len, "}");
                }
            }
        }
    }

    EMIT(ctx, "%s", o);
}
