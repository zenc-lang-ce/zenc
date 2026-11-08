// SPDX-License-Identifier: MIT
// The types of the C functions a program calls without declaring them: read from the C headers
// it includes (`include "x.h"` and `include <x.h>`) and from those of the hosted preamble, and
// turned from C text into Types.
#include "parser.h"
#include "utils/c_headers.h"
#include "utils/c_header_cache.h"
#include "platform/os.h"
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
        else if (strcmp(word, "signed") == 0 || strcmp(word, "__signed") == 0 ||
                 strcmp(word, "__signed__") == 0)
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

// How many typedefs of typedefs are followed (`time_t` -> `__darwin_time_t` -> `long`).
#define MAX_TYPEDEF_DEPTH 8

static Type *c_type_from_text_at(ParserContext *ctx, const char *text, int depth);

// The arithmetic type the C typedef `name` stands for (`time_t` is a `long`), written in C by
// that name, or NULL.
static Type *c_typedef_arithmetic_type(ParserContext *ctx, const char *name, int depth)
{
    const char **c_type =
        ctx->c_typedef_types.hash_func ? zmap_get(&ctx->c_typedef_types, name) : NULL;
    if (!c_type || depth >= MAX_TYPEDEF_DEPTH)
    {
        return NULL;
    }
    Type *named = c_type_from_text_at(ctx, *c_type, depth + 1);
    if (named && named->kind != TYPE_STRUCT && (is_integer_type(named) || is_float_type(named)))
    {
        named->c_typedef_name = xstrdup(name);
        return named;
    }
    return NULL;
}

Type *c_type_from_text(ParserContext *ctx, const char *text)
{
    return c_type_from_text_at(ctx, text, 0);
}

static Type *c_type_from_text_at(ParserContext *ctx, const char *text, int depth)
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
        // as an opaque C struct, or alone when a header defines it as an arithmetic type
        // (`time_t`), which C keeps writing by that name.
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
        else if (strchr(words, ' ') || !(base = c_typedef_arithmetic_type(ctx, words, depth)))
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

// Where the C declarations of `kind` are remembered: functions with their return types,
// variables and typedefs with their types, macros with the name they stand for. NULL for
// any other kind.
static zmap_FileSet *c_declarations_of(ParserContext *ctx, CSymbolKind kind)
{
    switch (kind)
    {
    case C_SYMBOL_FUNCTION:
        return &ctx->c_function_types;
    case C_SYMBOL_VARIABLE:
        return &ctx->c_variable_types;
    case C_SYMBOL_TYPE:
        return &ctx->c_typedef_types;
    case C_SYMBOL_CONSTANT:
        return &ctx->c_macro_names;
    default:
        return NULL;
    }
}

// Remembers what a header says of `name`, unless a header read before said it.
static void remember_c_declaration(ParserContext *ctx, CSymbolKind kind, const char *name,
                                   const char *c_type)
{
    zmap_FileSet *declarations = c_declarations_of(ctx, kind);
    if (!declarations || !c_type)
    {
        return;
    }
    if (!declarations->hash_func)
    {
        *declarations = zmap_init(FileSet, zmap_hash_cstr, zmap_cmp_cstr);
    }
    if (!zmap_get(declarations, name))
    {
        zmap_put(declarations, xstrdup(name), xstrdup(c_type));
    }
}

// What the headers said of `name` among the declarations of `kind`, or NULL.
static const char *known_c_declaration(ParserContext *ctx, CSymbolKind kind, const char *name)
{
    zmap_FileSet *declarations = c_declarations_of(ctx, kind);
    const char **c_type =
        (declarations && declarations->hash_func) ? zmap_get(declarations, name) : NULL;
    return c_type ? *c_type : NULL;
}

void register_c_header_types(ParserContext *ctx, const CHeaderFile *header)
{
    for (int i = 0; i < header->symbol_count; i++)
    {
        const CHeaderEntry *entry = &header->symbols[i];
        remember_c_declaration(ctx, entry->kind, entry->name, entry->c_type);
    }
}

// Remembers what the header at `path` and every header it includes declare; with no `ctx`,
// only reads them into the cache.
static void read_header_tree(ParserContext *ctx, const char *path, const CSearchDirList *dirs,
                             StringList *visited, int depth)
{
    if (depth > C_HEADERS_MAX_INCLUDE_DEPTH || string_list_contains(visited, path))
    {
        return;
    }
    string_list_add(visited, path);
    const CachedHeader *header = c_header_cache_header(path, dirs);
    if (!header)
    {
        return;
    }
    for (int i = 0; ctx && i < header->declaration_count; i++)
    {
        const CachedDeclaration *declaration = &header->declarations[i];
        remember_c_declaration(ctx, declaration->kind, declaration->name, declaration->c_type);
    }
    for (int i = 0; i < header->include_count; i++)
    {
        read_header_tree(ctx, header->includes[i], dirs, visited, depth + 1);
    }
}

// What the headers read depend on: the zc binary (its reader), the C compiler and the
// include paths (`include_dirs`). NULL when it does not fit.
static const char *header_cache_key(const CompilerConfig *config,
                                    const CSearchDirList *include_dirs)
{
    char executable[MAX_PATH_LEN];
    long long modified = 0;
    long long size = 0;
    z_get_executable_file(executable, sizeof(executable));
    c_headers_file_stamp(executable, &modified, &size);
    char key[4096];
    int used = snprintf(key, sizeof(key), "zc=%s %lld %lld cc=%s include=", ZEN_VERSION, modified,
                        size, config->cc);
    for (int i = 0; i < include_dirs->count && used >= 0 && (size_t)used < sizeof(key); i++)
    {
        used +=
            snprintf(key + used, sizeof(key) - (size_t)used, "%s;", include_dirs->items[i].path);
    }
    if (used < 0 || (size_t)used >= sizeof(key))
    {
        return NULL;
    }
    return xstrdup(key);
}

// Reads the headers of the hosted preamble and those in `includes` (`include <x.h>`), as the C
// compiler will find them (the include paths, then the compiler's own directories), through the
// cache on disk. A header that is not found gives nothing. With `discard`, all of them are read
// again.
static int read_system_headers(ParserContext *ctx, const CompilerConfig *config,
                               const StringList *includes, int discard)
{
    CSearchDirList dirs = {0};
    for (size_t i = 0; i < config->include_paths.length; i++)
    {
        char absolute[MAX_PATH_LEN];
        z_get_absolute_path(config->include_paths.data[i], absolute, sizeof(absolute));
        c_search_dirs_add(&dirs, absolute[0] ? absolute : config->include_paths.data[i], "");
    }
    c_header_cache_open(header_cache_key(config, &dirs), discard);
    const StringList *compiler_dirs = c_header_cache_compiler_dirs();
    if (!compiler_dirs)
    {
        compiler_dirs = c_headers_compiler_dirs(config->cc, NULL);
        c_header_cache_set_compiler_dirs(compiler_dirs);
    }
    for (int i = 0; i < compiler_dirs->count; i++)
    {
        c_search_dirs_add(&dirs, compiler_dirs->items[i], "");
    }
    StringList names = {0};
    for (int i = 0; PREAMBLE_HEADERS[i]; i++)
    {
        string_list_add(&names, PREAMBLE_HEADERS[i]);
    }
    for (int i = 0; includes && i < includes->count; i++)
    {
        string_list_add(&names, includes->items[i]);
    }
    StringList visited = {0};
    for (int i = 0; i < names.count; i++)
    {
        char *path = c_headers_find(names.items[i], NULL, &dirs, NULL, 0);
        if (path)
        {
            read_header_tree(ctx, path, &dirs, &visited, 0);
            libc_free(path);
        }
    }
    string_list_free(&names);
    string_list_free(&visited);
    c_search_dirs_free(&dirs);
    return c_header_cache_save();
}

int refresh_c_header_cache(const CompilerConfig *config)
{
    return read_system_headers(NULL, config, NULL, 1);
}

// What the headers said of `name` among the declarations of `kind`, reading the system's
// headers the first time a name is not known.
static const char *c_declaration(ParserContext *ctx, CSymbolKind kind, const char *name)
{
    const char *c_type = known_c_declaration(ctx, kind, name);
    if (!c_type && !ctx->c_system_headers_read)
    {
        ctx->c_system_headers_read = 1;
        read_system_headers(ctx, ctx->config, &ctx->c_system_includes, 0);
        c_type = known_c_declaration(ctx, kind, name);
    }
    return c_type;
}

Type *c_function_return_type(ParserContext *ctx, const char *name)
{
    // The MISRA and freestanding preambles include other headers.
    if (ctx->config->misra_mode || ctx->config->is_freestanding)
    {
        return NULL;
    }
    return c_type_from_text(ctx, c_declaration(ctx, C_SYMBOL_FUNCTION, name));
}

int c_headers_declare_function(ParserContext *ctx, const char *name)
{
    return c_declaration(ctx, C_SYMBOL_FUNCTION, name) != NULL;
}

Type *c_variable_type(ParserContext *ctx, const char *name)
{
    if (ctx->config->misra_mode || ctx->config->is_freestanding)
    {
        return NULL;
    }
    // A macro may stand for the variable (`#define stdin __stdinp` in macOS's <stdio.h>).
    for (int depth = 0; name && depth < MAX_TYPEDEF_DEPTH; depth++)
    {
        const char *c_type = c_declaration(ctx, C_SYMBOL_VARIABLE, name);
        if (c_type)
        {
            return c_type_from_text(ctx, c_type);
        }
        const char *alias = known_c_declaration(ctx, C_SYMBOL_CONSTANT, name);
        name = (alias && strcmp(alias, name) != 0) ? alias : NULL;
    }
    return NULL;
}
