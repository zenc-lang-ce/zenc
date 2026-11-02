// SPDX-License-Identifier: MIT
#include "c_header_cache.h"
#include "zalloc.h"
#include "../arena.h"
#include "../platform/os.h"
#include <stdio.h>
#include <string.h>

// The first line of a cache file. Change it when what the reader gives changes in a way the
// key does not show (the key already changes with the zc binary).
#define CACHE_FORMAT "zc c-header cache 3"

static CachedHeader *g_headers = NULL;
static CachedHeader *g_last_header = NULL;
static StringList g_compiler_dirs;
static int g_has_compiler_dirs = 0;
static char *g_cache_file = NULL;
static char *g_cache_key = NULL;
static int g_opened = 0;
static int g_changed = 0;

static unsigned long long hash_text(const char *text)
{
    unsigned long long hash = 14695981039346656037ULL;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++)
    {
        hash = (hash ^ *p) * 1099511628211ULL;
    }
    return hash;
}

static CachedHeader *find_header(const char *path)
{
    for (CachedHeader *header = g_headers; header; header = header->next)
    {
        if (strcmp(header->path, path) == 0)
        {
            return header;
        }
    }
    return NULL;
}

static CachedHeader *add_header(const char *path)
{
    CachedHeader *header = xcalloc(1, sizeof(CachedHeader));
    header->path = xstrdup(path);
    if (g_last_header)
    {
        g_last_header->next = header;
    }
    else
    {
        g_headers = header;
    }
    g_last_header = header;
    return header;
}

// The word a cache line starts with for each kind of declaration.
static const struct
{
    CSymbolKind kind;
    const char *word;
} DECLARATION_WORDS[] = {{C_SYMBOL_FUNCTION, "function"},
                         {C_SYMBOL_VARIABLE, "variable"},
                         {C_SYMBOL_TYPE, "type"},
                         {C_SYMBOL_CONSTANT, "macro"}};

#define DECLARATION_KINDS ((int)(sizeof(DECLARATION_WORDS) / sizeof(DECLARATION_WORDS[0])))

// Adds to `header` the declaration a `<word> <name> <type>` line gives, when it is one and
// there is room.
static void add_declaration_line(CachedHeader *header, char *line)
{
    for (int i = 0; i < DECLARATION_KINDS; i++)
    {
        size_t length = strlen(DECLARATION_WORDS[i].word);
        if (strncmp(line, DECLARATION_WORDS[i].word, length) != 0 || line[length] != ' ')
        {
            continue;
        }
        char *name = line + length + 1;
        char *space = strchr(name, ' ');
        if (space && header->declaration_count < header->declaration_capacity)
        {
            *space = '\0';
            CachedDeclaration *declaration = &header->declarations[header->declaration_count++];
            declaration->kind = DECLARATION_WORDS[i].kind;
            declaration->name = xstrdup(name);
            declaration->c_type = xstrdup(space + 1);
        }
        return;
    }
}

// `header <modified> <size> <declarations> <includes> <path>`, followed by its declarations
// (`function <name> <type>`, `variable`, `type`, `macro <name> <name>`) and `include <path>`
// lines.
static void load_cache_file(void)
{
    char *text = c_headers_read_file(g_cache_file);
    if (!text)
    {
        return;
    }
    CachedHeader *current = NULL;
    int line_number = 0;
    char *line = text;
    while (*line)
    {
        char *end = strchr(line, '\n');
        if (end)
        {
            *end = '\0';
        }
        if ((line_number == 0 && strcmp(line, CACHE_FORMAT) != 0) ||
            (line_number == 1 && (strncmp(line, "key ", 4) != 0 || strcmp(line + 4, g_cache_key))))
        {
            break;
        }
        long long modified = 0;
        long long size = 0;
        int declarations = 0;
        int includes = 0;
        int path_offset = 0;
        if (strncmp(line, "dir ", 4) == 0)
        {
            string_list_add(&g_compiler_dirs, line + 4);
            g_has_compiler_dirs = 1;
        }
        else if (sscanf(line, "header %lld %lld %d %d %n", &modified, &size, &declarations,
                        &includes, &path_offset) == 4 &&
                 path_offset > 0 && declarations >= 0 && includes >= 0)
        {
            current = add_header(line + path_offset);
            current->modified = modified;
            current->size = size;
            current->declarations = xcalloc((size_t)declarations + 1, sizeof(CachedDeclaration));
            current->includes = xcalloc((size_t)includes + 1, sizeof(char *));
            current->declaration_capacity = declarations;
            current->include_capacity = includes;
        }
        else if (current && strncmp(line, "include ", 8) == 0)
        {
            if (current->include_count < current->include_capacity)
            {
                current->includes[current->include_count++] = xstrdup(line + 8);
            }
        }
        else if (current)
        {
            add_declaration_line(current, line);
        }
        line_number++;
        if (!end)
        {
            break;
        }
        line = end + 1;
    }
    libc_free(text);
}

void c_header_cache_open(const char *key, int discard)
{
    if (g_opened)
    {
        return;
    }
    g_opened = 1;
    const char *dir = z_get_cache_dir();
    if (!dir || !key)
    {
        return;
    }
    char path[MAX_PATH_LEN];
    int written = snprintf(path, sizeof(path), "%s/c-headers-%016llx.txt", dir, hash_text(key));
    if (written < 0 || (size_t)written >= sizeof(path))
    {
        return;
    }
    g_cache_file = xstrdup(path);
    g_cache_key = xstrdup(key);
    if (discard)
    {
        g_changed = 1;
        return;
    }
    load_cache_file();
}

const char *c_header_cache_file(void)
{
    return g_cache_file;
}

const StringList *c_header_cache_compiler_dirs(void)
{
    return g_has_compiler_dirs ? &g_compiler_dirs : NULL;
}

void c_header_cache_set_compiler_dirs(const StringList *dirs)
{
    // Without directories (no C compiler) there is nothing worth keeping.
    if (dirs->count == 0)
    {
        return;
    }
    for (int i = 0; i < dirs->count; i++)
    {
        string_list_add(&g_compiler_dirs, dirs->items[i]);
    }
    g_has_compiler_dirs = 1;
    g_changed = 1;
}

static CachedHeader *scan_header(const char *path, const CSearchDirList *dirs)
{
    CHeaderFile *file = c_headers_get(path);
    if (!file)
    {
        return NULL;
    }
    CachedHeader *header = find_header(path);
    if (!header)
    {
        header = add_header(path);
    }
    header->modified = file->modified;
    header->size = file->size;
    header->declarations = xcalloc((size_t)file->symbol_count + 1, sizeof(CachedDeclaration));
    header->declaration_count = 0;
    header->declaration_capacity = file->symbol_count;
    for (int i = 0; i < file->symbol_count; i++)
    {
        const CHeaderEntry *entry = &file->symbols[i];
        if (!entry->c_type ||
            (entry->kind != C_SYMBOL_FUNCTION && entry->kind != C_SYMBOL_VARIABLE &&
             entry->kind != C_SYMBOL_TYPE && entry->kind != C_SYMBOL_CONSTANT))
        {
            continue;
        }
        CachedDeclaration *declaration = &header->declarations[header->declaration_count++];
        declaration->kind = (CSymbolKind)entry->kind;
        declaration->name = xstrdup(entry->name);
        declaration->c_type = xstrdup(entry->c_type);
    }
    header->includes = xcalloc((size_t)file->include_count + 1, sizeof(char *));
    header->include_count = 0;
    header->include_capacity = file->include_count;
    char header_dir[MAX_PATH_LEN];
    c_headers_directory_of(path, header_dir, sizeof(header_dir));
    for (int i = 0; i < file->include_count; i++)
    {
        char *child = c_headers_find(
            file->includes[i].name, file->includes[i].is_system ? NULL : header_dir, dirs, NULL, 0);
        if (child)
        {
            header->includes[header->include_count++] = xstrdup(child);
            libc_free(child);
        }
    }
    header->checked = 1;
    g_changed = 1;
    return header;
}

const CachedHeader *c_header_cache_header(const char *path, const CSearchDirList *dirs)
{
    CachedHeader *header = find_header(path);
    if (header && !header->checked)
    {
        long long modified = 0;
        long long size = 0;
        header->checked = c_headers_file_stamp(path, &modified, &size) &&
                          modified == header->modified && size == header->size;
    }
    if (header && header->checked)
    {
        return header;
    }
    return scan_header(path, dirs);
}

int c_header_cache_save(void)
{
    if (!g_changed || !g_cache_file)
    {
        return 0;
    }
    g_changed = 0;
    if (z_make_directories(z_get_cache_dir()) != 0)
    {
        return -1;
    }
    // Written aside and renamed, so a compilation running at the same time never reads half
    // a file.
    char temporary[MAX_PATH_LEN];
    int length = snprintf(temporary, sizeof(temporary), "%s.%d.tmp", g_cache_file, z_get_pid());
    if (length < 0 || (size_t)length >= sizeof(temporary))
    {
        return -1;
    }
    FILE *out = fopen(temporary, "w");
    if (!out)
    {
        return -1;
    }
    int count = 0;
    fprintf(out, "%s\nkey %s\n", CACHE_FORMAT, g_cache_key);
    for (int i = 0; i < g_compiler_dirs.count; i++)
    {
        fprintf(out, "dir %s\n", g_compiler_dirs.items[i]);
    }
    for (const CachedHeader *header = g_headers; header; header = header->next)
    {
        fprintf(out, "header %lld %lld %d %d %s\n", header->modified, header->size,
                header->declaration_count, header->include_count, header->path);
        for (int i = 0; i < header->declaration_count; i++)
        {
            const CachedDeclaration *declaration = &header->declarations[i];
            for (int j = 0; j < DECLARATION_KINDS; j++)
            {
                if (DECLARATION_WORDS[j].kind == declaration->kind)
                {
                    fprintf(out, "%s %s %s\n", DECLARATION_WORDS[j].word, declaration->name,
                            declaration->c_type);
                }
            }
        }
        for (int i = 0; i < header->include_count; i++)
        {
            fprintf(out, "include %s\n", header->includes[i]);
        }
        count++;
    }
    if (fclose(out) != 0 || z_rename_replace(temporary, g_cache_file) != 0)
    {
        remove(temporary);
        return -1;
    }
    return count;
}
