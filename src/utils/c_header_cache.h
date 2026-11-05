// SPDX-License-Identifier: MIT
#ifndef C_HEADER_CACHE_H
#ifndef ZC_ALLOW_INTERNAL
#error "utils/c_header_cache.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define C_HEADER_CACHE_H

/**
 * @file c_header_cache.h
 * @brief What the compiler reads from the system's C headers, kept on disk between
 * compilations: the functions, variables and typedefs each header declares, with the C text of
 * their types, the macros that only stand for another name, the headers it includes, and the C
 * compiler's include directories. One file per key (the
 * zc binary, the C compiler, the include paths) in z_get_cache_dir(). A header is read again when
 * its modification time or size change. Compiler only: memory comes from the arena.
 */

#include "c_headers.h"
#include "string_list.h"

typedef struct
{
    CSymbolKind kind; ///< C_SYMBOL_FUNCTION, C_SYMBOL_VARIABLE, C_SYMBOL_TYPE or C_SYMBOL_CONSTANT.
    char *name;
    char *c_type; ///< Its type as C text; for a macro, the name it stands for.
} CachedDeclaration;

typedef struct CachedHeader
{
    char *path;
    long long modified;
    long long size;
    CachedDeclaration *declarations; ///< What it declares with a known type or name.
    int declaration_count;
    int declaration_capacity;
    char **includes; ///< The headers it includes that were found, as paths.
    int include_count;
    int include_capacity;
    int checked; ///< Compared with the file on disk by this process.
    struct CachedHeader *next;
} CachedHeader;

/// Loads the cache file of `key`, once per process. With `discard`, the file is ignored and
/// replaced when the cache is saved.
void c_header_cache_open(const char *key, int discard);

/// The C compiler's include directories, when the cache has them; NULL otherwise.
const StringList *c_header_cache_compiler_dirs(void);
void c_header_cache_set_compiler_dirs(const StringList *dirs);

/// The header at `path`: from the cache while it has not changed on disk, otherwise read
/// and scanned, with its includes looked up in `dirs`. NULL when it cannot be read.
const CachedHeader *c_header_cache_header(const char *path, const CSearchDirList *dirs);

/// The cache file, once opened; NULL when there is no cache directory.
const char *c_header_cache_file(void);

/// Writes the cache file, when something changed since it was loaded: how many headers it
/// holds, 0 when nothing was written, -1 when it could not be written.
int c_header_cache_save(void);

#endif
