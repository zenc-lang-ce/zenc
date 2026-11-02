// SPDX-License-Identifier: MIT
// The types of the C functions a program calls without declaring them: read from the C headers
// it includes and from those of the hosted preamble, and turned from C text into Types.
#include "parser.h"
#include "utils/c_headers.h"
#include "utils/zalloc.h"
#include "ast/primitives.h"
#include <string.h>

// The headers the hosted preamble includes (emit_preamble), where its C functions are declared.
static const char *const PREAMBLE_HEADERS[] = {"stdio.h",  "stdlib.h", "stddef.h",  "string.h",
                                               "stdarg.h", "stdint.h", "stdbool.h", "unistd.h",
                                               "fcntl.h",  NULL};

// The Type of a C integer, char or floating type written with C's own words (`unsigned long`,
// `long int`, `char`), or NULL when the words are not one.
static Type *c_arithmetic_type(const char *words)
{
    int is_unsigned = 0;
    int is_signed = 0;
    int longs = 0;
    int shorts = 0;
    int has_int = 0;
    int has_char = 0;
    int others = 0;
    char copy[128];
    snprintf(copy, sizeof(copy), "%s", words);
    for (char *word = strtok(copy, " "); word; word = strtok(NULL, " "))
    {
        if (strcmp(word, "unsigned") == 0)
        {
            is_unsigned = 1;
        }
        else if (strcmp(word, "signed") == 0)
        {
            is_signed = 1;
        }
        else if (strcmp(word, "long") == 0)
        {
            longs++;
        }
        else if (strcmp(word, "short") == 0)
        {
            shorts++;
        }
        else if (strcmp(word, "int") == 0)
        {
            has_int = 1;
        }
        else if (strcmp(word, "char") == 0)
        {
            has_char = 1;
        }
        else
        {
            others++;
        }
    }
    if (others || !(is_unsigned || is_signed || longs || shorts || has_int || has_char))
    {
        return NULL;
    }
    if (has_char)
    {
        if (longs || shorts || has_int)
        {
            return NULL;
        }
        return type_new(is_unsigned ? TYPE_C_UCHAR : (is_signed ? TYPE_I8 : TYPE_C_CHAR));
    }
    if (shorts)
    {
        return type_new(is_unsigned ? TYPE_C_USHORT : TYPE_C_SHORT);
    }
    if (longs >= 2)
    {
        return type_new(is_unsigned ? TYPE_C_ULONGLONG : TYPE_C_LONGLONG);
    }
    if (longs == 1)
    {
        return type_new(is_unsigned ? TYPE_C_ULONG : TYPE_C_LONG);
    }
    return type_new(is_unsigned ? TYPE_C_UINT : TYPE_C_INT);
}

Type *c_type_from_text(ParserContext *ctx, const char *text)
{
    if (!text)
    {
        return NULL;
    }
    // The words of the base type, its `const`, and the pointers after it (`const char *`).
    char words[128];
    size_t used = 0;
    int is_const = 0;
    int pointers = 0;
    int tag = 0; // 1 for `struct`, 2 for `union`
    char copy[256];
    snprintf(copy, sizeof(copy), "%s", text);
    for (char *p = copy; *p; p++)
    {
        if (*p == '*')
        {
            *p = ' ';
            pointers++;
        }
    }
    for (char *word = strtok(copy, " "); word; word = strtok(NULL, " "))
    {
        if (strcmp(word, "const") == 0 || strcmp(word, "volatile") == 0)
        {
            is_const = is_const || strcmp(word, "const") == 0;
            continue;
        }
        if (strcmp(word, "struct") == 0 || strcmp(word, "union") == 0)
        {
            tag = word[0] == 's' ? 1 : 2;
            continue;
        }
        size_t length = strlen(word);
        if (used + length + 2 > sizeof(words))
        {
            return NULL;
        }
        if (used)
        {
            words[used++] = ' ';
        }
        memcpy(words + used, word, length);
        used += length;
    }
    words[used] = 0;
    if (!used)
    {
        return NULL;
    }

    Type *base = NULL;
    if (tag)
    {
        // `struct tm`: a C struct, written with its keyword.
        if (strchr(words, ' '))
        {
            return NULL;
        }
        base = type_new(TYPE_STRUCT);
        base->name = xstrdup(words);
        base->is_explicit_struct = 1;
        base->is_union = tag == 2;
    }
    else if (strcmp(words, "void") == 0)
    {
        base = type_new(TYPE_VOID);
    }
    else if (strcmp(words, "_Bool") == 0)
    {
        base = type_new(TYPE_BOOL);
    }
    else if (!(base = c_arithmetic_type(words)))
    {
        // A name: the standard typedefs (`size_t`, `uint8_t`, `bool`) are primitives, and
        // `float` and `double` too; any other name is only known behind a pointer (`FILE *`),
        // as an opaque C struct. Alone it may be an integer typedef (`time_t`): no type.
        TypeKind kind = strchr(words, ' ') ? TYPE_UNKNOWN : find_primitive_kind(words);
        if (kind != TYPE_UNKNOWN && kind != TYPE_STRING && kind != TYPE_VOID)
        {
            base = type_new(kind);
        }
        else if (!strchr(words, ' ') && pointers > 0)
        {
            TypeAlias *alias = find_type_alias_node(ctx, words);
            base = alias ? alias_type(alias) : type_new(TYPE_STRUCT);
            if (!alias)
            {
                base->name = xstrdup(words);
            }
        }
        else
        {
            return NULL;
        }
    }
    base->is_const = is_const;
    for (int i = 0; i < pointers; i++)
    {
        base = type_new_ptr(base);
    }
    return base;
}

static void remember_c_function(ParserContext *ctx, const char *name, const char *c_type)
{
    if (!ctx->c_function_types.hash_func)
    {
        ctx->c_function_types = zmap_init(FileSet, zmap_hash_cstr, zmap_cmp_cstr);
    }
    if (!zmap_get(&ctx->c_function_types, name))
    {
        zmap_put(&ctx->c_function_types, xstrdup(name), c_type ? xstrdup(c_type) : "");
    }
}

void register_c_header_functions(ParserContext *ctx, const CHeaderFile *header)
{
    for (int i = 0; i < header->symbol_count; i++)
    {
        const CHeaderEntry *entry = &header->symbols[i];
        if (entry->kind == C_SYMBOL_FUNCTION && entry->c_type)
        {
            remember_c_function(ctx, entry->name, entry->c_type);
        }
    }
}

// Remembers the functions of the header at `path` and of every header it includes.
static void read_header_tree(ParserContext *ctx, const char *path, const CSearchDirList *dirs,
                             StringList *visited, int depth)
{
    if (depth > C_HEADERS_MAX_INCLUDE_DEPTH || string_list_contains(visited, path))
    {
        return;
    }
    string_list_add(visited, path);
    CHeaderFile *header = c_headers_get(path);
    if (!header)
    {
        return;
    }
    register_c_header_functions(ctx, header);
    char header_dir[MAX_PATH_LEN];
    c_headers_directory_of(path, header_dir, sizeof(header_dir));
    for (int i = 0; i < header->include_count; i++)
    {
        char *child =
            c_headers_find(header->includes[i].name,
                           header->includes[i].is_system ? NULL : header_dir, dirs, NULL, 0);
        if (child)
        {
            read_header_tree(ctx, child, dirs, visited, depth + 1);
            libc_free(child);
        }
    }
}

// Reads the headers of the hosted preamble, once, as the C compiler will find them: the
// configured include paths, then the compiler's own directories.
static void read_preamble_headers(ParserContext *ctx)
{
    if (ctx->c_preamble_headers_read)
    {
        return;
    }
    ctx->c_preamble_headers_read = 1;
    CSearchDirList dirs = {0};
    for (size_t i = 0; i < ctx->config->include_paths.length; i++)
    {
        c_search_dirs_add(&dirs, ctx->config->include_paths.data[i], "");
    }
    const StringList *compiler_dirs = c_headers_compiler_dirs(ctx->config->cc, NULL);
    for (int i = 0; i < compiler_dirs->count; i++)
    {
        c_search_dirs_add(&dirs, compiler_dirs->items[i], "");
    }
    StringList visited = {0};
    for (int i = 0; PREAMBLE_HEADERS[i]; i++)
    {
        char *path = c_headers_find(PREAMBLE_HEADERS[i], NULL, &dirs, NULL, 0);
        if (path)
        {
            read_header_tree(ctx, path, &dirs, &visited, 0);
            libc_free(path);
        }
    }
    string_list_free(&visited);
    c_search_dirs_free(&dirs);
}

Type *c_function_return_type(ParserContext *ctx, const char *name)
{
    // The MISRA and freestanding preambles include other headers.
    if (ctx->config->misra_mode || ctx->config->is_freestanding)
    {
        return NULL;
    }
    if (is_preamble_c_function(ctx, name))
    {
        read_preamble_headers(ctx);
    }
    const char **c_type =
        ctx->c_function_types.hash_func ? zmap_get(&ctx->c_function_types, name) : NULL;
    return (c_type && (*c_type)[0]) ? c_type_from_text(ctx, *c_type) : NULL;
}
