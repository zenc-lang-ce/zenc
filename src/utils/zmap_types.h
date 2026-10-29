// SPDX-License-Identifier: MIT
#ifndef ZMAP_TYPES_H
#ifndef ZC_ALLOW_INTERNAL
#error "utils/zmap_types.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define ZMAP_TYPES_H

/**
 * @brief The compiler's hash map types.
 *
 * zmap.h generates every map type from one list, once, so they are all declared here, where
 * any part of the compiler can include them. A new map type is added to REGISTER_ZMAP_TYPES.
 */

#include "../arena.h"
#include <stdint.h>
#include <string.h>

// The maps allocate from the compiler's arena, which frees nothing.
#define ZMAP_MALLOC(sz) xmalloc(sz)
#define ZMAP_CALLOC(n, sz) xcalloc(n, sz)
#define ZMAP_FREE(p) ((void)(p))

static inline uint32_t zmap_hash_cstr(const char *k, uint32_t seed)
{
    // FNV-1a inline (avoids dependency on ZMAP_HASH_STR being defined yet)
    uint32_t hash = 2166136261u ^ seed;
    const uint8_t *data = (const uint8_t *)k;
    while (*data)
    {
        hash ^= *data++;
        hash *= 16777619;
    }
    return hash;
}
static inline int zmap_cmp_cstr(const char *a, const char *b)
{
    return strcmp(a, b);
}

struct Module;
struct SelectiveImport;
struct ImportedPlugin;
struct ZenSymbol;

#define REGISTER_ZMAP_TYPES(X)                                                                     \
    X(const char *, struct Module *, ModMap)                                                       \
    X(const char *, struct SelectiveImport *, SelMap)                                              \
    X(const char *, const char *, FileSet)                                                         \
    X(const char *, struct ImportedPlugin *, PluginMap)                                            \
    X(const struct ZenSymbol *, struct ZenSymbol *, DeclSet)                                       \
    X(const char *, struct ZenSymbol *, SymbolByName)

#include "zmap.h"

#endif
