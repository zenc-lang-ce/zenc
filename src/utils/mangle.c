// SPDX-License-Identifier: MIT
#include "mangle.h"
#include "arena.h"
#include <stdio.h>
#include <string.h>

char *merge_underscores(const char *name)
{
    if (!name)
    {
        return NULL;
    }

    size_t len = strlen(name);
    char *res = xmalloc((size_t)(len + 1));
    char *out = res;
    const char *in = name;

    while (*in)
    {
        if (in[0] == '_' && in[1] == '_' && in[2] == '_' && in[3] == '_')
        {
            // Quadruple or more underscores -> collapse to double.
            // A run of exactly three underscores is kept: it encodes the "__"
            // separator plus an identifier that begins with "_" (e.g.
            // `Struct::_method` vs `Struct::method`), so collapsing it would
            // make distinct symbols collide.
            *out++ = '_';
            *out++ = '_';
            in += 4;
            while (*in == '_')
            {
                in++;
            }
        }
        else
        {
            *out++ = *in++;
        }
    }
    *out = '\0';
    return res;
}

// Canonical symbol for a method: `Struct__Method` (or `Struct__Trait__Method`).
// A method whose name starts with `_` must keep the resulting run of three
// underscores (separator "__" plus the method's leading "_"), otherwise
// merge_underscores would collapse it to two and collide with the plain
// `method` symbol on the same struct. This preserves injectivity while keeping
// mangled names readable.
char *mangle_method_symbol(const char *struct_name, const char *trait_name, const char *method_name)
{
    // Measure first (snprintf with no buffer only counts), so names of any length fit.
    int len = trait_name ? snprintf(NULL, 0, "%s__%s__%s", struct_name, trait_name, method_name)
                         : snprintf(NULL, 0, "%s__%s", struct_name, method_name);

    char *tmp = xmalloc((size_t)len + 1);
    if (trait_name)
    {
        snprintf(tmp, (size_t)len + 1, "%s__%s__%s", struct_name, trait_name, method_name);
    }
    else
    {
        snprintf(tmp, (size_t)len + 1, "%s__%s", struct_name, method_name);
    }
    if (method_name && method_name[0] == '_')
    {
        return tmp;
    }
    return merge_underscores(tmp);
}

// The C name of a symbol declared in a module: `prefix__name`. Underscores are not
// merged: this is the name the declaration registers, so every lookup must build
// exactly the same one.
char *mangle_module_symbol(const char *module_prefix, const char *name)
{
    int len = snprintf(NULL, 0, "%s__%s", module_prefix, name);
    char *out = xmalloc((size_t)len + 1);
    snprintf(out, (size_t)len + 1, "%s__%s", module_prefix, name);
    return out;
}

// The C name of an enum variant: `Enum__Variant`, the constructor function or, for a
// variant without data, its value; its tag is that name plus `_Tag`. enum_c_name is the
// name the enum has in C (its link_name, if it has one).
char *mangle_variant_symbol(const char *enum_c_name, const char *variant_name)
{
    int len = snprintf(NULL, 0, "%s__%s", enum_c_name, variant_name);
    char *tmp = xmalloc((size_t)len + 1);
    snprintf(tmp, (size_t)len + 1, "%s__%s", enum_c_name, variant_name);
    return merge_underscores(tmp);
}

// Helper to sanitize type names for mangling (e.g. "int*" -> "intPtr")
char *sanitize_mangled_name(const char *s)
{
    char *buf = xmalloc(strlen(s) * 4 + 1);

    // Skip "struct " prefix if present to avoid "struct_" in mangled names
    if (strncmp(s, "struct ", 7) == 0)
    {
        s += 7;
    }

    char *p = buf;
    while (*s)
    {
        if (*s == '*')
        {
            strcpy(p, "Ptr");
            p += 3;
        }
        else if (*s == '<' || *s == ',' || *s == ' ')
        {
            *p++ = '_';
            *p++ = '_';
        }
        else if (*s == '>' || *s == '&')
        {
            // Skip > and & (often used in references) to keep names clean
        }
        else if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') ||
                 *s == '_')
        {
            *p++ = *s;
        }
        else
        {
            *p++ = '_';
        }
        s++;
    }
    *p = 0;
    return buf;
}

// The C name of an instance of a generic, as instantiate_generic registers it: the base
// without trailing underscores, then `__` and each argument sanitized, without merging.
// Vec<Foo*> is Vec__FooPtr and Map<K, V> is Map__K__V.
char *mangle_generic_name(const char *base, char **args, int count)
{
    size_t base_len = strlen(base);
    while (base_len > 0 && base[base_len - 1] == '_')
    {
        base_len--;
    }
    char *out = xmalloc(base_len + 1);
    memcpy(out, base, base_len);
    out[base_len] = '\0';

    for (int i = 0; i < count; i++)
    {
        char *clean = sanitize_mangled_name(args[i]);
        size_t size = strlen(out) + 2 + strlen(clean) + 1;
        char *next = xmalloc(size);
        snprintf(next, size, "%s__%s", out, clean);
        out = next;
    }
    return out;
}

// mangle_generic_name from the argument list as written between `<` and `>`: "K, Vec<A, B>"
// has two arguments, split at the commas outside a nested `<>`, each without surrounding
// spaces.
char *mangle_generic_name_from_arg_list(const char *base, const char *arg_list)
{
    int count = 1;
    int depth = 0;
    for (const char *p = arg_list; *p; p++)
    {
        if (*p == '<')
        {
            depth++;
        }
        else if (*p == '>')
        {
            depth--;
        }
        else if (*p == ',' && depth == 0)
        {
            count++;
        }
    }

    char **args = xmalloc((size_t)count * sizeof(char *));
    int n = 0;
    const char *start = arg_list;
    depth = 0;
    for (const char *p = arg_list;; p++)
    {
        if (*p == '<')
        {
            depth++;
        }
        else if (*p == '>')
        {
            depth--;
        }
        else if (*p == '\0' || (*p == ',' && depth == 0))
        {
            const char *end = p;
            while (start < end && *start == ' ')
            {
                start++;
            }
            while (end > start && end[-1] == ' ')
            {
                end--;
            }
            size_t len = (size_t)(end - start);
            args[n] = xmalloc(len + 1);
            memcpy(args[n], start, len);
            args[n][len] = '\0';
            n++;
            if (*p == '\0')
            {
                break;
            }
            start = p + 1;
        }
    }
    return mangle_generic_name(base, args, n);
}

// The C name of the vtable of a trait for a struct: `Struct__Trait__VTable`, as codegen
// defines it (without merging underscores), so a trait object built in the parser points at
// the same symbol.
char *mangle_vtable_symbol(const char *struct_name, const char *trait_name)
{
    int len = snprintf(NULL, 0, "%s__%s__VTable", struct_name, trait_name);
    char *out = xmalloc((size_t)len + 1);
    snprintf(out, (size_t)len + 1, "%s__%s__VTable", struct_name, trait_name);
    return out;
}
