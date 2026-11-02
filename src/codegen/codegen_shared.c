// SPDX-License-Identifier: MIT
#include "codegen_shared.h"
#include "../parser/parser.h"
#include "../ast/primitives.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// String manipulation

char *strip_template_suffix(const char *name)
{
    if (!name)
    {
        return NULL;
    }
    char *lt = (char *)strchr(name, '<');
    if (lt)
    {
        ptrdiff_t len = lt - name;
        char *buf = xmalloc((size_t)(len + 1));
        strncpy(buf, name, (size_t)(len));
        buf[len] = 0;
        return buf;
    }
    return xstrdup(name);
}

const char *parse_original_method_name(const char *mangled)
{
    const char *sep = (char *)strstr(mangled, "__");
    if (!sep)
    {
        return mangled;
    }

    const char *last_double = NULL;
    const char *p = mangled;
    while ((p = strstr(p, "__")))
    {
        last_double = p;
        p += 2;
    }

    return last_double ? last_double + 2 : mangled;
}

char *replace_string_type(const char *args)
{
    if (!args)
    {
        return NULL;
    }
    char *res = xmalloc(strlen(args) * 2 + 16);
    res[0] = 0;
    const char *p = args;
    while (*p)
    {
        const char *match = (char *)strstr(p, "string");
        if (match)
        {
            size_t prefix_len = (size_t)(match - p);
            strncat(res, p, (size_t)(prefix_len));
            p = match;

            int before_ok = (match == args || (!isalnum(*(match - 1)) && *(match - 1) != '_'));
            int after_ok = (!isalnum(match[6]) && match[6] != '_');

            if (before_ok && after_ok)
            {
                strcat(res, "const char*");
                p += 6;
            }
            else
            {
                strncat(res, p, 1);
                p += 1;
            }
        }
        else
        {
            strcat(res, p);
            break;
        }
    }
    return res;
}

// Type name mapping

const char *map_to_c_type(const char *t)
{
    if (strcmp(t, "c_int") == 0)
    {
        return "int";
    }
    if (strcmp(t, "c_uint") == 0)
    {
        return "unsigned int";
    }
    if (strcmp(t, "c_long") == 0)
    {
        return "long";
    }
    if (strcmp(t, "c_ulong") == 0)
    {
        return "unsigned long";
    }
    if (strcmp(t, "c_longlong") == 0)
    {
        return "long long";
    }
    if (strcmp(t, "c_ulonglong") == 0)
    {
        return "unsigned long long";
    }
    if (strcmp(t, "c_short") == 0)
    {
        return "short";
    }
    if (strcmp(t, "c_ushort") == 0)
    {
        return "unsigned short";
    }
    if (strcmp(t, "c_char") == 0)
    {
        return "char";
    }
    if (strcmp(t, "c_uchar") == 0)
    {
        return "unsigned char";
    }
    if (strcmp(t, "uint") == 0)
    {
        return "unsigned int";
    }

    return normalize_type_name(t);
}
