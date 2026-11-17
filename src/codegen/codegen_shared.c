// SPDX-License-Identifier: MIT
#include "codegen_shared.h"
#include "../parser/parser.h"
#include "../ast/primitives.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// String manipulation

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
