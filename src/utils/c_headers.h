// SPDX-License-Identifier: MIT
#ifndef C_HEADERS_H
#ifndef ZC_ALLOW_INTERNAL
#error "utils/c_headers.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define C_HEADERS_H

/**
 * @file c_headers.h
 * @brief Reads C headers: the names they declare (functions, types, structs, fields), the
 * headers they include, and where the C compiler looks for them. Shared by the compiler and
 * the language server. Memory comes from the libc heap, not the arena: the language server
 * rewinds the arena between requests, and headers are cached across them.
 */

#include "../constants.h"
#include "string_list.h"
#include <stddef.h>

#define C_HEADERS_MAX_INCLUDE_DEPTH 16

/// Where a name is declared in a header.
typedef struct CHeaderSymbol
{
    const char *path; ///< Path of the header.
    int line;         ///< 0-based line of the name.
    int column;       ///< 0-based column of the name.
    int length;       ///< Length of the name.
    int start_line;   ///< First line of the whole declaration.
    int end_line;     ///< Last line of the whole declaration.
} CHeaderSymbol;

typedef struct
{
    char *path;
    char *origin; ///< What made it a search directory, for hover.
} CSearchDir;

typedef struct
{
    CSearchDir *items;
    int count;
    int capacity;
} CSearchDirList;

// LSP CompletionItemKind of each kind of C declaration.
typedef enum
{
    C_SYMBOL_FUNCTION = 3,
    C_SYMBOL_VARIABLE = 6,
    C_SYMBOL_ENUM = 13,
    C_SYMBOL_ENUM_MEMBER = 20,
    C_SYMBOL_CONSTANT = 21,
    C_SYMBOL_STRUCT = 22,
    C_SYMBOL_TYPE = 25
} CSymbolKind;

typedef struct
{
    CHeaderSymbol symbol;
    char *name;
    int is_forward; ///< `struct X;` without a body.
    int kind;       ///< LSP CompletionItemKind (see CSymbolKind).
} CHeaderEntry;

typedef struct
{
    char *name;
    int is_system;
} CHeaderInclude;

typedef struct CHeaderFile
{
    char *path;
    long long modified;
    long long size;
    char *text;
    int *line_offsets;
    int line_count;
    CHeaderEntry *symbols;
    int symbol_count;
    int symbol_capacity;
    CHeaderInclude *includes;
    int include_count;
    int include_capacity;
    struct CHeaderFile *next;
} CHeaderFile;

char *c_headers_copy_text(const char *text, size_t length);
void c_headers_copy_trimmed(const char *text, size_t length, char *out, size_t out_size);
int c_headers_is_identifier_char(char c);
int c_headers_is_regular_file(const char *path);
void c_headers_directory_of(const char *path, char *out, size_t out_size);
void c_headers_join_path(const char *directory, const char *name, char *out, size_t out_size);
char *c_headers_read_file(const char *path);
int c_headers_is_reserved_identifier(const char *name);
const char *c_headers_skip_blanks(const char *p);
const char *c_headers_read_until(const char *p, char close, char *out, size_t out_size);

void c_search_dirs_add(CSearchDirList *list, const char *path, const char *origin);
void c_search_dirs_append(CSearchDirList *list, const CSearchDirList *other);
void c_search_dirs_free(CSearchDirList *list);

/// The default `#include <...>` directories of the C compiler `cc` ("cc" when empty), asked
/// once per process; `status` gets the compiler's exit status when given.
const StringList *c_headers_compiler_dirs(const char *cc, int *status);

/// Resolves a header like the C preprocessor: `local_dir` first for `"x.h"` (NULL for
/// `<x.h>`), then `dirs`. Returns a libc string or NULL; `found_by` gets what found it.
char *c_headers_find(const char *name, const char *local_dir, const CSearchDirList *dirs,
                     char *found_by, size_t found_by_size);

/// The header at `path`, read and scanned, from the cache unless it changed on disk.
CHeaderFile *c_headers_get(const char *path);
/// The cached header at `path`, or NULL when it has not been read.
CHeaderFile *c_headers_cached(const char *path);

/// Searches `path` and then, depth first, the headers it includes, for the declarations of
/// `name`; returns how many of the first header that declares it were stored in `results`.
int c_headers_search_tree(const char *path, const char *name, const CSearchDirList *dirs,
                          StringList *visited, const CHeaderSymbol **results, int max_results,
                          int depth);

#endif
