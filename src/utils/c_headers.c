// SPDX-License-Identifier: MIT
#include "c_headers.h"
#include "utils.h"
#include "zalloc.h"
#include "../platform/os.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define COMPILER_OUTPUT_SIZE 65536

const char *c_headers_skip_blanks(const char *p)
{
    while (*p == ' ' || *p == '\t')
    {
        p++;
    }
    return p;
}

// Copies the text up to `close` (on the same line) into `out`, and returns the position
// after `close`, or NULL when it is missing.
const char *c_headers_read_until(const char *p, char close, char *out, size_t out_size)
{
    size_t length = 0;
    while (p[length] && p[length] != close && p[length] != '\n')
    {
        length++;
    }
    if (p[length] != close || length == 0 || length >= out_size)
    {
        return NULL;
    }
    memcpy(out, p, length);
    out[length] = '\0';
    return p + length + 1;
}

char *c_headers_copy_text(const char *text, size_t length)
{
    char *copy = libc_malloc(length + 1);
    if (copy)
    {
        memcpy(copy, text, length);
        copy[length] = '\0';
    }
    return copy;
}

// Copies `text[0..length)` into `out` without surrounding whitespace.
void c_headers_copy_trimmed(const char *text, size_t length, char *out, size_t out_size)
{
    while (length > 0 && isspace((unsigned char)*text))
    {
        text++;
        length--;
    }
    while (length > 0 && isspace((unsigned char)text[length - 1]))
    {
        length--;
    }
    if (length >= out_size)
    {
        length = out_size - 1;
    }
    memcpy(out, text, length);
    out[length] = '\0';
}

int c_headers_is_identifier_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

int c_headers_is_regular_file(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

// Directory part of `path`, or an empty string when it has none.
void c_headers_directory_of(const char *path, char *out, size_t out_size)
{
    snprintf(out, out_size, "%s", path);
    char *separator = z_path_last_sep(out);
    if (separator)
    {
        *separator = '\0';
    }
    else
    {
        out[0] = '\0';
    }
}

void c_headers_join_path(const char *directory, const char *name, char *out, size_t out_size)
{
    if (!directory[0] || z_is_abs_path(name))
    {
        snprintf(out, out_size, "%s", name);
    }
    else
    {
        snprintf(out, out_size, "%s/%s", directory, name);
    }
}

/* --- Where to look for headers --- */

static StringList g_compiler_dirs;
static int g_compiler_dirs_probed = 0;

const StringList *c_headers_compiler_dirs(const char *cc, int *status)
{
    if (g_compiler_dirs_probed)
    {
        return &g_compiler_dirs;
    }
    g_compiler_dirs_probed = 1;

    // The configured compiler may carry its own arguments (e.g. "zig cc").
    char compiler[MAX_PATH_LEN];
    snprintf(compiler, sizeof(compiler), "%s", (cc && cc[0]) ? cc : "cc");
    char *argv[24];
    int argc = 0;
    for (char *word = strtok(compiler, " \t"); word && argc < 16; word = strtok(NULL, " \t"))
    {
        argv[argc++] = word;
    }
    if (argc == 0)
    {
        return &g_compiler_dirs;
    }
#if ZC_OS_WINDOWS
    char null_device[] = "NUL";
#else
    char null_device[] = "/dev/null";
#endif
    char preprocess_only[] = "-E";
    char language_flag[] = "-x";
    char language[] = "c";
    char verbose[] = "-v";
    argv[argc++] = preprocess_only;
    argv[argc++] = language_flag;
    argv[argc++] = language;
    argv[argc++] = verbose;
    argv[argc++] = null_device;
    argv[argc] = NULL;

    char *output = libc_malloc(COMPILER_OUTPUT_SIZE);
    if (!output)
    {
        return &g_compiler_dirs;
    }
    int exit_status = z_run_command_capture_ex(argv, output, COMPILER_OUTPUT_SIZE, 1);
    if (status)
    {
        *status = exit_status;
    }

    const char *marker = strstr(output, "#include <...> search starts here:");
    const char *line_start = marker ? strchr(marker, '\n') : NULL;
    while (line_start)
    {
        line_start++;
        const char *line_end = strchr(line_start, '\n');
        size_t length = line_end ? (size_t)(line_end - line_start) : strlen(line_start);
        char entry[MAX_PATH_LEN];
        c_headers_copy_trimmed(line_start, length, entry, sizeof(entry));
        if (strncmp(entry, "End of search list.", 19) == 0)
        {
            break;
        }
        if (!strstr(entry, "(framework directory)"))
        {
            string_list_add(&g_compiler_dirs, entry);
        }
        line_start = line_end;
    }

    libc_free(output);
    return &g_compiler_dirs;
}

void c_search_dirs_add(CSearchDirList *list, const char *path, const char *origin)
{
    if (!path[0])
    {
        return;
    }
    for (int i = 0; i < list->count; i++)
    {
        if (strcmp(list->items[i].path, path) == 0)
        {
            return;
        }
    }
    if (list->count == list->capacity)
    {
        int capacity = list->capacity ? list->capacity * 2 : 8;
        CSearchDir *items = libc_realloc(list->items, (size_t)capacity * sizeof(CSearchDir));
        if (!items)
        {
            return;
        }
        list->items = items;
        list->capacity = capacity;
    }
    char *path_copy = c_headers_copy_text(path, strlen(path));
    char *origin_copy = c_headers_copy_text(origin, strlen(origin));
    if (!path_copy || !origin_copy)
    {
        libc_free(path_copy);
        libc_free(origin_copy);
        return;
    }
    list->items[list->count].path = path_copy;
    list->items[list->count].origin = origin_copy;
    list->count++;
}

void c_search_dirs_append(CSearchDirList *list, const CSearchDirList *other)
{
    for (int i = 0; i < other->count; i++)
    {
        c_search_dirs_add(list, other->items[i].path, other->items[i].origin);
    }
}

void c_search_dirs_free(CSearchDirList *list)
{
    for (int i = 0; i < list->count; i++)
    {
        libc_free(list->items[i].path);
        libc_free(list->items[i].origin);
    }
    libc_free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

// Resolves a header like the C preprocessor: `local_dir` first for `"x.h"` (the directory
// of the file that includes it, NULL for `<x.h>`), then the search directories. Returns a
// libc string or NULL, and what found it in `found_by` when given.
char *c_headers_find(const char *name, const char *local_dir, const CSearchDirList *dirs,
                     char *found_by, size_t found_by_size)
{
    char path[MAX_PATH_LEN];
    const char *origin = NULL;
    if (z_is_abs_path(name))
    {
        if (c_headers_is_regular_file(name))
        {
            snprintf(path, sizeof(path), "%s", name);
            origin = "its absolute path";
        }
    }
    else
    {
        if (local_dir && local_dir[0])
        {
            c_headers_join_path(local_dir, name, path, sizeof(path));
            if (c_headers_is_regular_file(path))
            {
                origin = "the directory of the file that imports it";
            }
        }
        for (int i = 0; !origin && i < dirs->count; i++)
        {
            c_headers_join_path(dirs->items[i].path, name, path, sizeof(path));
            if (c_headers_is_regular_file(path))
            {
                origin = dirs->items[i].origin;
            }
        }
    }
    if (!origin)
    {
        return NULL;
    }
    if (found_by)
    {
        snprintf(found_by, found_by_size, "%s", origin);
    }
    return c_headers_copy_text(path, strlen(path));
}

/* --- Header cache --- */

static CHeaderFile *g_headers = NULL;

static int line_of(const CHeaderFile *header, int offset)
{
    int low = 0;
    int high = header->line_count - 1;
    while (low < high)
    {
        int middle = (low + high + 1) / 2;
        if (header->line_offsets[middle] <= offset)
        {
            low = middle;
        }
        else
        {
            high = middle - 1;
        }
    }
    return low;
}

static CHeaderEntry *add_header_symbol(CHeaderFile *header, const char *name, int name_length,
                                       int offset, int start_line, int end_line, int is_forward,
                                       int kind)
{
    if (header->symbol_count == header->symbol_capacity)
    {
        int capacity = header->symbol_capacity ? header->symbol_capacity * 2 : 64;
        CHeaderEntry *symbols =
            libc_realloc(header->symbols, (size_t)capacity * sizeof(CHeaderEntry));
        if (!symbols)
        {
            return NULL;
        }
        header->symbols = symbols;
        header->symbol_capacity = capacity;
    }
    char *copy = c_headers_copy_text(name, (size_t)name_length);
    if (!copy)
    {
        return NULL;
    }
    CHeaderEntry *entry = &header->symbols[header->symbol_count++];
    entry->name = copy;
    entry->is_forward = is_forward;
    entry->kind = kind;
    entry->symbol.path = header->path;
    entry->symbol.line = line_of(header, offset);
    entry->symbol.column = offset - header->line_offsets[entry->symbol.line];
    entry->symbol.length = name_length;
    entry->symbol.start_line = start_line;
    entry->symbol.end_line = end_line;
    entry->tag = C_TAG_NONE;
    entry->c_type = NULL;
    return entry;
}

static void add_include(CHeaderFile *header, const char *name, int is_system)
{
    if (header->include_count == header->include_capacity)
    {
        int capacity = header->include_capacity ? header->include_capacity * 2 : 8;
        CHeaderInclude *includes =
            libc_realloc(header->includes, (size_t)capacity * sizeof(CHeaderInclude));
        if (!includes)
        {
            return;
        }
        header->includes = includes;
        header->include_capacity = capacity;
    }
    char *copy = c_headers_copy_text(name, strlen(name));
    if (copy)
    {
        header->includes[header->include_count].name = copy;
        header->includes[header->include_count].is_system = is_system;
        header->include_count++;
    }
}

/* --- Declaration scanner ---
 *
 * Not a C preprocessor: it reads the header line by line, skipping comments, and splits
 * the code into top-level statements (up to `;`, or up to the closing brace of a function
 * body). Every `#if` branch is read, so a name can be declared more than once. */

typedef struct
{
    char *chars;
    int *offsets; ///< Offset in the header text of each character.
    int length;
    int capacity;
} Statement;

static void statement_push(Statement *statement, char c, int offset)
{
    if (c == ' ' && (statement->length == 0 || statement->chars[statement->length - 1] == ' '))
    {
        return;
    }
    if (statement->length == statement->capacity)
    {
        int capacity = statement->capacity ? statement->capacity * 2 : 256;
        char *chars = libc_realloc(statement->chars, (size_t)capacity);
        if (!chars)
        {
            return;
        }
        statement->chars = chars;
        int *offsets = libc_realloc(statement->offsets, (size_t)capacity * sizeof(int));
        if (!offsets)
        {
            return;
        }
        statement->offsets = offsets;
        statement->capacity = capacity;
    }
    statement->chars[statement->length] = c;
    statement->offsets[statement->length] = offset;
    statement->length++;
}

typedef struct
{
    int start; ///< Index in the statement.
    int length;
    char kind; ///< 'w' word, 'n' number, 's' string literal, or the punctuation character.
    int brace_depth;
    int paren_depth;
} CToken;

static int tokenize(const Statement *statement, CToken *tokens)
{
    int count = 0;
    int brace_depth = 0;
    int paren_depth = 0;
    int i = 0;
    while (i < statement->length)
    {
        char c = statement->chars[i];
        if (c == ' ')
        {
            i++;
            continue;
        }

        CToken *token = &tokens[count++];
        token->start = i;
        token->brace_depth = brace_depth;
        token->paren_depth = paren_depth;
        if (isalpha((unsigned char)c) || c == '_')
        {
            int end = i;
            while (end < statement->length && c_headers_is_identifier_char(statement->chars[end]))
            {
                end++;
            }
            token->kind = 'w';
            token->length = end - i;
            i = end;
        }
        else if (isdigit((unsigned char)c))
        {
            int end = i;
            while (end < statement->length &&
                   (c_headers_is_identifier_char(statement->chars[end]) ||
                    statement->chars[end] == '.'))
            {
                end++;
            }
            token->kind = 'n';
            token->length = end - i;
            i = end;
        }
        else if (c == '"' || c == '\'')
        {
            int end = i + 1;
            while (end < statement->length && statement->chars[end] != c)
            {
                if (statement->chars[end] == '\\')
                {
                    end++;
                }
                end++;
            }
            token->kind = 's';
            token->length = (end < statement->length ? end + 1 : end) - i;
            i += token->length;
        }
        else
        {
            token->kind = c;
            token->length = 1;
            i++;
            if (c == '{')
            {
                brace_depth++;
            }
            else if (c == '}' && brace_depth > 0)
            {
                brace_depth--;
                token->brace_depth = brace_depth;
            }
            else if (c == '(')
            {
                paren_depth++;
            }
            else if (c == ')' && paren_depth > 0)
            {
                paren_depth--;
                token->paren_depth = paren_depth;
            }
        }
    }
    return count;
}

static int token_is(const Statement *statement, const CToken *token, const char *word)
{
    size_t length = strlen(word);
    return token->kind == 'w' && (size_t)token->length == length &&
           strncmp(statement->chars + token->start, word, length) == 0;
}

static int token_in(const Statement *statement, const CToken *token, const char *const *words)
{
    for (int i = 0; words[i]; i++)
    {
        if (token_is(statement, token, words[i]))
        {
            return 1;
        }
    }
    return 0;
}

static const char *const C_KEYWORDS[] = {
    "auto",          "break",     "case",       "char",
    "const",         "continue",  "default",    "do",
    "double",        "else",      "enum",       "extern",
    "float",         "for",       "goto",       "if",
    "inline",        "int",       "long",       "register",
    "restrict",      "return",    "short",      "signed",
    "sizeof",        "static",    "struct",     "switch",
    "typedef",       "union",     "unsigned",   "void",
    "volatile",      "while",     "_Bool",      "_Complex",
    "_Atomic",       "bool",      "__restrict", "__inline",
    "__extension__", "_Nullable", "_Nonnull",   "_Null_unspecified",
    "__nullable",    "__nonnull", NULL};

// Words followed by parentheses that are not the declared name.
static const char *const ATTRIBUTE_WORDS[] = {
    "__attribute__", "__attribute", "__declspec", "__asm__", "__asm",          "asm",
    "_Alignas",      "alignas",     "__typeof__", "typeof",  "_Static_assert", "static_assert",
    "_Pragma",       "__pragma",    "sizeof",     NULL};

static int is_name_token(const Statement *statement, const CToken *token)
{
    return token->kind == 'w' && !token_in(statement, token, C_KEYWORDS) &&
           !token_in(statement, token, ATTRIBUTE_WORDS);
}

static CHeaderEntry *record(CHeaderFile *header, const Statement *statement, const CToken *token,
                            int start_line, int end_line, int is_forward, int kind)
{
    return add_header_symbol(header, statement->chars + token->start, token->length,
                             statement->offsets[token->start], start_line, end_line, is_forward,
                             kind);
}

// The tag keyword a declaration starts with (`struct`, `union` or `enum` at `token`).
static CTagKind tag_kind(const Statement *statement, const CToken *token)
{
    if (token_is(statement, token, "struct"))
    {
        return C_TAG_STRUCT;
    }
    if (token_is(statement, token, "union"))
    {
        return C_TAG_UNION;
    }
    return token_is(statement, token, "enum") ? C_TAG_ENUM : C_TAG_NONE;
}

// First token of `kind` at the statement's top level (outside braces and parentheses).
static int find_top_level(const CToken *tokens, int count, int from, char kind)
{
    for (int i = from; i < count; i++)
    {
        if (tokens[i].kind == kind && tokens[i].brace_depth == 0 && tokens[i].paren_depth == 0)
        {
            return i;
        }
    }
    return -1;
}

// `enum { A, B = 2, C }`: every name right after the opening brace or a comma.
static void record_enumerators(CHeaderFile *header, const Statement *statement,
                               const CToken *tokens, int count, int body)
{
    for (int i = body + 1; i < count; i++)
    {
        if (tokens[i].kind == '}' && tokens[i].brace_depth == 0)
        {
            return;
        }
        if (tokens[i].kind == 'w' && tokens[i].brace_depth == 1 && tokens[i].paren_depth == 0 &&
            (i == body + 1 || tokens[i - 1].kind == ','))
        {
            int line = line_of(header, statement->offsets[tokens[i].start]);
            record(header, statement, &tokens[i], line, line, 0, C_SYMBOL_ENUM_MEMBER);
        }
    }
}

// A function pointer `(*name)`: the name after the opening parenthesis at `open`.
static int function_pointer_name(const Statement *statement, const CToken *tokens, int count,
                                 int open)
{
    int i = open + 1;
    if (i >= count || (tokens[i].kind != '*' && tokens[i].kind != '^'))
    {
        return -1;
    }
    while (i < count && (tokens[i].kind == '*' || tokens[i].kind == '^' ||
                         token_in(statement, &tokens[i], C_KEYWORDS)))
    {
        i++;
    }
    return (i < count && tokens[i].kind == 'w') ? i : -1;
}

static char *declaration_type_text(const Statement *statement, const CToken *tokens, int from,
                                   int to);

static void process_typedef(CHeaderFile *header, const Statement *statement, const CToken *tokens,
                            int count, int start_line, int end_line)
{
    if (count < 2)
    {
        return;
    }
    int body = find_top_level(tokens, count, 0, '{');
    if (body >= 0)
    {
        // typedef struct Tag { ... } Name, *Pointer;
        int is_enum = token_is(statement, &tokens[1], "enum");
        if (body >= 3 && tokens[body - 1].kind == 'w' &&
            (is_enum || token_is(statement, &tokens[1], "struct") ||
             token_is(statement, &tokens[1], "union")))
        {
            CHeaderEntry *tag = record(header, statement, &tokens[body - 1], start_line, end_line,
                                       0, is_enum ? C_SYMBOL_ENUM : C_SYMBOL_STRUCT);
            if (tag)
            {
                tag->tag = tag_kind(statement, &tokens[1]);
            }
        }
        if (is_enum)
        {
            record_enumerators(header, statement, tokens, count, body);
        }
        int close = find_top_level(tokens, count, body + 1, '}');
        for (int i = close + 1; close >= 0 && i < count; i++)
        {
            if (tokens[i].paren_depth == 0 && is_name_token(statement, &tokens[i]))
            {
                record(header, statement, &tokens[i], start_line, end_line, 0,
                       is_enum ? C_SYMBOL_ENUM : C_SYMBOL_STRUCT);
            }
        }
        return;
    }

    // typedef void (*Name)(int);
    for (int i = 1; i < count; i++)
    {
        if (tokens[i].kind == '(' && tokens[i].paren_depth == 0)
        {
            int name = function_pointer_name(statement, tokens, count, i);
            if (name >= 0)
            {
                record(header, statement, &tokens[name], start_line, end_line, 0, C_SYMBOL_TYPE);
                return;
            }
        }
    }

    // typedef int Name(int);   typedef unsigned int Name;   typedef int Name[4];
    int last_name = -1;
    int is_plain = 1; // Neither a function type nor an array.
    for (int i = 1; i < count; i++)
    {
        if (tokens[i].paren_depth > 0)
        {
            continue;
        }
        if (tokens[i].kind == '(' || tokens[i].kind == '[' || tokens[i].kind == ';')
        {
            is_plain = tokens[i].kind == ';';
            break;
        }
        if (is_name_token(statement, &tokens[i]))
        {
            last_name = i;
        }
    }
    if (last_name >= 0)
    {
        CHeaderEntry *entry =
            record(header, statement, &tokens[last_name], start_line, end_line, 0, C_SYMBOL_TYPE);
        if (entry && is_plain)
        {
            entry->c_type = declaration_type_text(statement, tokens, 1, last_name);
        }
    }
}

// Words of a declaration that say nothing about its type: storage, inlining, nullability.
static const char *const NON_TYPE_WORDS[] = {
    "extern",        "static",        "inline",   "__inline",
    "__inline__",    "_Noreturn",     "noreturn", "register",
    "_Thread_local", "__extension__", "restrict", "__restrict",
    "__restrict__",  "_Nullable",     "_Nonnull", "_Null_unspecified",
    "__nullable",    "__nonnull",     NULL};

// Reserved-looking words that are C types, not the macros system headers use for attributes.
static const char *const RESERVED_TYPE_WORDS[] = {"_Bool",    "_Complex",   "__int128",
                                                  "__signed", "__signed__", NULL};

// Words that go with a type without naming one.
static const char *const TYPE_QUALIFIER_WORDS[] = {"const", "volatile", "struct",
                                                   "union", "enum",     NULL};

// A reserved name at tokens[at] is the declaration's type, not a macro (`__darwin_time_t` in
// `typedef __darwin_time_t time_t`, `struct __sFILE *`), when no type came before it and only
// pointers and qualifiers follow it up to tokens[to].
static int is_reserved_type_name(const Statement *statement, const CToken *tokens, int at, int to,
                                 int has_type)
{
    if (has_type)
    {
        return 0;
    }
    for (int i = at + 1; i < to; i++)
    {
        if (tokens[i].kind != '*' &&
            !(tokens[i].kind == 'w' && (token_in(statement, &tokens[i], TYPE_QUALIFIER_WORDS) ||
                                        token_in(statement, &tokens[i], NON_TYPE_WORDS))))
        {
            return 0;
        }
    }
    return 1;
}

// The type a declaration gives its name, written from tokens[from..to) as C spells it
// (`const char *`), without storage words, attributes (`__attribute__((...))`, a macro called
// with arguments), the reserved macros of system headers (`__BEGIN_DECLS`) or anything before
// the end of a previous declaration. A libc string, or NULL when nothing is left.
static char *declaration_type_text(const Statement *statement, const CToken *tokens, int from,
                                   int to)
{
    char text[512];
    size_t used = 0;
    char previous = 0;
    int has_type = 0; // A word other than a qualifier was written.
    for (int i = from; i < to; i++)
    {
        const CToken *token = &tokens[i];
        // Only the last declaration before the name counts: a statement may run on from an
        // inline function's body (`char *f(char *s) { ... } char *g(...)`).
        if (token->kind == '}' || token->kind == ';')
        {
            used = 0;
            previous = 0;
            has_type = 0;
            continue;
        }
        if (token->brace_depth > 0)
        {
            continue;
        }
        if (token->kind == 'w' && i + 1 < to && tokens[i + 1].kind == '(')
        {
            // An attribute or a macro with arguments: skip it with its parentheses.
            int depth = tokens[i + 1].paren_depth;
            i += 2;
            while (i < to && !(tokens[i].kind == ')' && tokens[i].paren_depth == depth))
            {
                i++;
            }
            continue;
        }
        if (token->kind == 'w' &&
            (token_in(statement, token, NON_TYPE_WORDS) ||
             (c_headers_is_reserved_identifier(statement->chars + token->start) &&
              !token_in(statement, token, RESERVED_TYPE_WORDS) &&
              !is_reserved_type_name(statement, tokens, i, to, has_type))))
        {
            continue;
        }
        if (token->kind != 'w' && token->kind != '*')
        {
            continue;
        }
        int space = used > 0 && !(previous == '*' && token->kind == '*');
        if (used + (size_t)space + (size_t)token->length + 1 > sizeof(text))
        {
            return NULL;
        }
        if (space)
        {
            text[used++] = ' ';
        }
        memcpy(text + used, statement->chars + token->start, (size_t)token->length);
        used += (size_t)token->length;
        previous = token->kind;
        has_type =
            has_type || (token->kind == 'w' && !token_in(statement, token, TYPE_QUALIFIER_WORDS));
    }
    return used ? c_headers_copy_text(text, used) : NULL;
}

static void process_function_or_variable(CHeaderFile *header, const Statement *statement,
                                         const CToken *tokens, int count, int start_line,
                                         int end_line)
{
    for (int i = 0; i < count; i++)
    {
        if (tokens[i].kind != '(' || tokens[i].brace_depth != 0 || tokens[i].paren_depth != 0)
        {
            continue;
        }
        // void (*callback)(int);
        int pointer_name = function_pointer_name(statement, tokens, count, i);
        if (pointer_name >= 0)
        {
            record(header, statement, &tokens[pointer_name], start_line, end_line, 0,
                   C_SYMBOL_VARIABLE);
            return;
        }
        // RLAPI void InitWindow(int width, int height, const char *title);
        if (i > 0 && is_name_token(statement, &tokens[i - 1]))
        {
            // System headers annotate the return type with reserved macros called with
            // arguments (`void * __sized_by_or_null(__size) malloc(size_t __size)`): the name
            // is the first one before `(` that is not reserved, if there is one.
            int name = i - 1;
            if (c_headers_is_reserved_identifier(statement->chars + tokens[name].start))
            {
                for (int j = i + 1; j < count; j++)
                {
                    if (tokens[j].kind == '(' && tokens[j].brace_depth == 0 &&
                        tokens[j].paren_depth == 0 && is_name_token(statement, &tokens[j - 1]) &&
                        !c_headers_is_reserved_identifier(statement->chars + tokens[j - 1].start))
                    {
                        name = j - 1;
                        break;
                    }
                }
            }
            CHeaderEntry *entry = record(header, statement, &tokens[name], start_line, end_line, 0,
                                         C_SYMBOL_FUNCTION);
            if (entry)
            {
                entry->c_type = declaration_type_text(statement, tokens, 0, name);
            }
            return;
        }
        // `__attribute__((...))` and the like: the name comes later.
    }

    // Only `extern` variables are declarations worth recording.
    int is_extern = 0;
    int last_name = -1;
    for (int i = 0; i < count; i++)
    {
        if (tokens[i].brace_depth != 0 || tokens[i].paren_depth != 0)
        {
            continue;
        }
        if (token_is(statement, &tokens[i], "extern"))
        {
            is_extern = 1;
        }
        if (tokens[i].kind == '=' || tokens[i].kind == '[' || tokens[i].kind == ':' ||
            tokens[i].kind == ';')
        {
            break;
        }
        if (is_name_token(statement, &tokens[i]))
        {
            last_name = i;
        }
    }
    // A reserved macro after the name annotates it (`extern FILE *__stdinp
    // __swift_nonisolated_unsafe;`): the name is the one before it, when a type comes first.
    if (last_name >= 2 && is_name_token(statement, &tokens[last_name - 1]) &&
        c_headers_is_reserved_identifier(statement->chars + tokens[last_name].start) &&
        (tokens[last_name - 2].kind == '*' ||
         (tokens[last_name - 2].kind == 'w' &&
          !token_in(statement, &tokens[last_name - 2], NON_TYPE_WORDS))))
    {
        last_name--;
    }
    if (is_extern && last_name >= 0)
    {
        CHeaderEntry *entry = record(header, statement, &tokens[last_name], start_line, end_line, 0,
                                     C_SYMBOL_VARIABLE);
        if (entry)
        {
            entry->c_type = declaration_type_text(statement, tokens, 0, last_name);
        }
    }
}

// `__X` or `_X`: names reserved for the implementation, used by system header macros.
int c_headers_is_reserved_identifier(const char *name)
{
    return name[0] == '_' && (name[1] == '_' || isupper((unsigned char)name[1]));
}

// Skips what may precede the declaration itself: macro invocations without a semicolon,
// like `_LIBC_SINGLE_BY_DEFAULT()` (a declaration never starts with its name), and bare
// macros like `__BEGIN_DECLS` in front of `typedef`, `struct`, `union` or `enum`.
static int declaration_start(const Statement *statement, const CToken *tokens, int count)
{
    int start = 0;
    while (start + 1 < count && is_name_token(statement, &tokens[start]) &&
           tokens[start + 1].kind == '(')
    {
        int close = -1;
        for (int i = start + 2; i < count; i++)
        {
            if (tokens[i].kind == ')' && tokens[i].paren_depth == tokens[start + 1].paren_depth)
            {
                close = i;
                break;
            }
        }
        if (close < 0)
        {
            break;
        }
        start = close + 1;
    }

    for (int i = start; i < count && tokens[i].kind == 'w'; i++)
    {
        if (token_is(statement, &tokens[i], "typedef") ||
            token_is(statement, &tokens[i], "struct") || token_is(statement, &tokens[i], "union") ||
            token_is(statement, &tokens[i], "enum"))
        {
            return i;
        }
        if (!is_name_token(statement, &tokens[i]))
        {
            break;
        }
    }
    return start;
}

static void process_statement(CHeaderFile *header, const Statement *statement)
{
    if (statement->length == 0)
    {
        return;
    }
    CToken *tokens = libc_malloc((size_t)statement->length * sizeof(CToken));
    if (!tokens)
    {
        return;
    }
    int all_count = tokenize(statement, tokens);
    int first_token = declaration_start(statement, tokens, all_count);
    const CToken *declaration = tokens + first_token;
    int count = all_count - first_token;
    if (count > 0)
    {
        // A reserved-name macro alone on a line above the declaration (`__BEGIN_DECLS`) is
        // not shown as part of it.
        int shown = 0;
        while (shown + 1 < count && declaration[shown].kind == 'w' &&
               c_headers_is_reserved_identifier(statement->chars + declaration[shown].start) &&
               line_of(header, statement->offsets[declaration[shown].start]) <
                   line_of(header, statement->offsets[declaration[shown + 1].start]))
        {
            shown++;
        }
        int start_line = line_of(header, statement->offsets[declaration[shown].start]);
        int end_line = line_of(header, statement->offsets[statement->length - 1]);

        int first_paren = find_top_level(declaration, count, 0, '(');
        int body = find_top_level(declaration, count, 0, '{');
        if (token_is(statement, &declaration[0], "typedef"))
        {
            process_typedef(header, statement, declaration, count, start_line, end_line);
        }
        else if ((token_is(statement, &declaration[0], "struct") ||
                  token_is(statement, &declaration[0], "union") ||
                  token_is(statement, &declaration[0], "enum")) &&
                 (first_paren < 0 || (body >= 0 && body < first_paren)))
        {
            // struct Tag { ... };   enum { A, B };   struct Tag;
            if (count >= 2 && declaration[1].kind == 'w')
            {
                CHeaderEntry *tag = record(
                    header, statement, &declaration[1], start_line, end_line, body < 0,
                    token_is(statement, &declaration[0], "enum") ? C_SYMBOL_ENUM : C_SYMBOL_STRUCT);
                if (tag)
                {
                    tag->tag = tag_kind(statement, &declaration[0]);
                }
            }
            if (body >= 0 && token_is(statement, &declaration[0], "enum"))
            {
                record_enumerators(header, statement, declaration, count, body);
            }
        }
        else
        {
            process_function_or_variable(header, statement, declaration, count, start_line,
                                         end_line);
        }
    }
    libc_free(tokens);
}

// `extern "C" {`: a C++ linkage block, whose contents are top-level declarations.
static int is_extern_c_block(const Statement *statement)
{
    char compact[16];
    int length = 0;
    for (int i = 0; i < statement->length; i++)
    {
        if (statement->chars[i] == ' ')
        {
            continue;
        }
        if (length == (int)sizeof(compact) - 1)
        {
            return 0;
        }
        compact[length++] = statement->chars[i];
    }
    compact[length] = '\0';
    return strcmp(compact, "extern\"C\"{") == 0;
}

// Whether the body that just closed belongs to a function: `... )  { ... }`.
static int is_function_body(const Statement *statement)
{
    for (int i = 0; i < statement->length; i++)
    {
        if (statement->chars[i] == '{')
        {
            int j = i - 1;
            while (j >= 0 && statement->chars[j] == ' ')
            {
                j--;
            }
            return j >= 0 && statement->chars[j] == ')';
        }
    }
    return 0;
}

// The name a macro body from `body` to `end` stands for, when it is only that name, maybe in
// parentheses (`__stdinp` in `#define stdin __stdinp`, `stdin` in `#define stdin (stdin)`):
// a libc string, or NULL.
static char *macro_alias(const char *body, const char *end)
{
    const char *p = c_headers_skip_blanks(body);
    int parenthesized = p < end && *p == '(';
    if (parenthesized)
    {
        p = c_headers_skip_blanks(p + 1);
    }
    const char *name = p;
    while (p < end && c_headers_is_identifier_char(*p))
    {
        p++;
    }
    if (p == name || (*name >= '0' && *name <= '9'))
    {
        return NULL;
    }
    const char *rest = c_headers_skip_blanks(p);
    if (parenthesized)
    {
        if (rest >= end || *rest != ')')
        {
            return NULL;
        }
        rest = c_headers_skip_blanks(rest + 1);
    }
    // Only a comment may follow.
    if (rest < end && *rest != '\n' && strncmp(rest, "/*", 2) != 0 && strncmp(rest, "//", 2) != 0)
    {
        return NULL;
    }
    return c_headers_copy_text(name, (size_t)(p - name));
}

// Handles the preprocessor line starting at `offset` (`#define NAME`, `#include`) and
// returns the offset of the newline that ends it, continuation lines included.
static int scan_preprocessor_line(CHeaderFile *header, int offset)
{
    const char *text = header->text;
    int end = offset;
    while (text[end] && !(text[end] == '\n' && (end == 0 || text[end - 1] != '\\')))
    {
        end++;
    }

    const char *p = c_headers_skip_blanks(text + offset + 1);
    if (strncmp(p, "define", 6) == 0 && (p[6] == ' ' || p[6] == '\t'))
    {
        const char *name = c_headers_skip_blanks(p + 6);
        int length = 0;
        while (c_headers_is_identifier_char(name[length]))
        {
            length++;
        }
        if (length > 0)
        {
            int start_line = line_of(header, offset);
            // `#define NAME(args)` reads like a function; `#define NAME value` like a constant.
            CHeaderEntry *entry =
                add_header_symbol(header, name, length, (int)(name - text), start_line,
                                  line_of(header, end > offset ? end - 1 : offset), 0,
                                  name[length] == '(' ? C_SYMBOL_FUNCTION : C_SYMBOL_CONSTANT);
            if (entry && entry->kind == C_SYMBOL_CONSTANT)
            {
                entry->c_type = macro_alias(name + length, text + end);
            }
        }
    }
    else if (strncmp(p, "include", 7) == 0)
    {
        p = c_headers_skip_blanks(p + 7);
        if (*p == '<' || *p == '"')
        {
            char name[MAX_PATH_LEN];
            if (c_headers_read_until(p + 1, *p == '<' ? '>' : '"', name, sizeof(name)))
            {
                add_include(header, name, *p == '<');
            }
        }
    }
    return end;
}

static void scan_header(CHeaderFile *header)
{
    const char *text = header->text;
    Statement statement = {0};
    int brace_depth = 0;
    int paren_depth = 0;
    int at_line_start = 1;
    int i = 0;
    while (text[i])
    {
        char c = text[i];
        if (c == '\n')
        {
            at_line_start = 1;
            statement_push(&statement, ' ', i);
            i++;
            continue;
        }
        if (isspace((unsigned char)c))
        {
            statement_push(&statement, ' ', i);
            i++;
            continue;
        }
        if (at_line_start && c == '#')
        {
            i = scan_preprocessor_line(header, i);
            continue;
        }
        at_line_start = 0;

        if (c == '/' && text[i + 1] == '/')
        {
            while (text[i] && text[i] != '\n')
            {
                i++;
            }
            continue;
        }
        if (c == '/' && text[i + 1] == '*')
        {
            i += 2;
            while (text[i] && !(text[i] == '*' && text[i + 1] == '/'))
            {
                i++;
            }
            if (text[i])
            {
                i += 2;
            }
            statement_push(&statement, ' ', i);
            continue;
        }
        if (c == '"' || c == '\'')
        {
            statement_push(&statement, c, i);
            i++;
            while (text[i] && text[i] != c && text[i] != '\n')
            {
                if (text[i] == '\\' && text[i + 1])
                {
                    statement_push(&statement, text[i], i);
                    i++;
                }
                statement_push(&statement, text[i], i);
                i++;
            }
            if (text[i] == c)
            {
                statement_push(&statement, c, i);
                i++;
            }
            continue;
        }

        statement_push(&statement, c, i);
        if (c == '(')
        {
            paren_depth++;
        }
        else if (c == ')')
        {
            if (paren_depth > 0)
            {
                paren_depth--;
            }
        }
        else if (paren_depth == 0 && c == '{')
        {
            if (brace_depth == 0 && is_extern_c_block(&statement))
            {
                statement.length = 0;
            }
            else
            {
                brace_depth++;
            }
        }
        else if (paren_depth == 0 && c == '}')
        {
            if (brace_depth == 0)
            {
                // Closes an `extern "C"` block.
                statement.length = 0;
            }
            else
            {
                brace_depth--;
                if (brace_depth == 0 && is_function_body(&statement))
                {
                    process_statement(header, &statement);
                    statement.length = 0;
                }
            }
        }
        else if (paren_depth == 0 && brace_depth == 0 && c == ';')
        {
            process_statement(header, &statement);
            statement.length = 0;
        }
        i++;
    }
    libc_free(statement.chars);
    libc_free(statement.offsets);
}

char *c_headers_read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        return NULL;
    }
    char *text = NULL;
    if (fseek(file, 0, SEEK_END) == 0)
    {
        long size = ftell(file);
        if (size < 0)
        {
            fclose(file);
            return NULL;
        }
        if (fseek(file, 0, SEEK_SET) == 0)
        {
            text = libc_malloc((size_t)size + 1);
            if (text)
            {
                size_t read_size = fread(text, 1, (size_t)size, file);
                text[read_size] = '\0';
            }
        }
    }
    fclose(file);
    return text;
}

static void clear_header(CHeaderFile *header)
{
    for (int i = 0; i < header->symbol_count; i++)
    {
        libc_free(header->symbols[i].name);
        libc_free(header->symbols[i].c_type);
    }
    for (int i = 0; i < header->include_count; i++)
    {
        libc_free(header->includes[i].name);
    }
    libc_free(header->symbols);
    libc_free(header->includes);
    libc_free(header->text);
    libc_free(header->line_offsets);
    header->symbols = NULL;
    header->includes = NULL;
    header->text = NULL;
    header->line_offsets = NULL;
    header->symbol_count = 0;
    header->symbol_capacity = 0;
    header->include_count = 0;
    header->include_capacity = 0;
    header->line_count = 0;
}

CHeaderFile *c_headers_cached(const char *path)
{
    for (CHeaderFile *header = g_headers; header; header = header->next)
    {
        if (strcmp(header->path, path) == 0)
        {
            return header;
        }
    }
    return NULL;
}

int c_headers_file_stamp(const char *path, long long *modified, long long *size)
{
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
    {
        return 0;
    }
    *modified = (long long)st.st_mtime;
    *size = (long long)st.st_size;
    return 1;
}

// The scanned header at `path`, scanned again when it has changed on disk.
CHeaderFile *c_headers_get(const char *path)
{
    long long modified = 0;
    long long size = 0;
    if (!c_headers_file_stamp(path, &modified, &size))
    {
        return NULL;
    }

    CHeaderFile *header = c_headers_cached(path);
    if (header && header->modified == modified && header->size == size)
    {
        return header;
    }
    if (header)
    {
        clear_header(header);
    }
    else
    {
        header = libc_malloc(sizeof(CHeaderFile));
        char *path_copy = c_headers_copy_text(path, strlen(path));
        if (!header || !path_copy)
        {
            libc_free(header);
            libc_free(path_copy);
            return NULL;
        }
        memset(header, 0, sizeof(CHeaderFile));
        header->path = path_copy;
        header->next = g_headers;
        g_headers = header;
    }
    header->modified = modified;
    header->size = size;

    header->text = c_headers_read_file(path);
    if (!header->text)
    {
        return header;
    }
    int lines = 1;
    for (const char *p = header->text; *p; p++)
    {
        lines += *p == '\n';
    }
    header->line_offsets = libc_malloc((size_t)lines * sizeof(int));
    if (!header->line_offsets)
    {
        return header;
    }
    header->line_offsets[0] = 0;
    header->line_count = 1;
    for (int i = 0; header->text[i] && header->line_count < lines; i++)
    {
        if (header->text[i] == '\n')
        {
            header->line_offsets[header->line_count++] = i + 1;
        }
    }
    scan_header(header);
    return header;
}

// Declarations of `name` in `header`; forward declarations only when nothing else exists.
// A name declared twice by the same declaration (`typedef struct Color {...} Color;`)
// counts once.
static int collect_matches(CHeaderFile *header, const char *name, const CHeaderSymbol **results,
                           int max_results)
{
    int count = 0;
    for (int forward = 0; forward < 2 && count == 0; forward++)
    {
        for (int i = 0; i < header->symbol_count && count < max_results; i++)
        {
            const CHeaderEntry *candidate = &header->symbols[i];
            if (candidate->is_forward != forward || strcmp(candidate->name, name) != 0)
            {
                continue;
            }
            int same_declaration = 0;
            for (int j = 0; j < count; j++)
            {
                if (results[j]->start_line == candidate->symbol.start_line)
                {
                    same_declaration = 1;
                }
            }
            if (!same_declaration)
            {
                results[count++] = &candidate->symbol;
            }
        }
    }
    return count;
}

// Searches `path` and then, depth first, the headers it includes. Returns the matches of the
// first header that declares `name`.
int c_headers_search_tree(const char *path, const char *name, const CSearchDirList *dirs,
                          StringList *visited, const CHeaderSymbol **results, int max_results,
                          int depth)
{
    if (depth > C_HEADERS_MAX_INCLUDE_DEPTH || string_list_contains(visited, path))
    {
        return 0;
    }
    string_list_add(visited, path);

    CHeaderFile *header = c_headers_get(path);
    if (!header)
    {
        return 0;
    }
    int count = collect_matches(header, name, results, max_results);
    if (count > 0)
    {
        return count;
    }

    char header_dir[MAX_PATH_LEN];
    c_headers_directory_of(path, header_dir, sizeof(header_dir));
    for (int i = 0; i < header->include_count; i++)
    {
        char *child =
            c_headers_find(header->includes[i].name,
                           header->includes[i].is_system ? NULL : header_dir, dirs, NULL, 0);
        if (!child)
        {
            continue;
        }
        count = c_headers_search_tree(child, name, dirs, visited, results, max_results, depth + 1);
        libc_free(child);
        if (count > 0)
        {
            return count;
        }
    }
    return 0;
}
