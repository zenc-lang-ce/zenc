// SPDX-License-Identifier: MIT
#include "lsp_c_headers.h"
#include "../utils/c_headers.h"
#include "lsp_project.h"
#include "../utils/cJSON.h"
#include "../utils/string_list.h"
#include "../utils/utils.h"
#include "../constants.h"
#include "../platform/os.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// libc heap, not the arena: lookups run in read-only requests, which rewind the arena.

// Fields read from one C struct or union for completion.
#define MAX_C_FIELDS 128
#define MAX_HOVER_LINES 24
#define MAX_DOC_LENGTH 2048

typedef struct PkgConfigEntry
{
    char *spec;
    StringList dirs;
    struct PkgConfigEntry *next;
} PkgConfigEntry;

static PkgConfigEntry *g_pkg_config_entries = NULL;

// Include directories reported by `pkg-config --cflags-only-I`, cached per argument list.
static const StringList *pkg_config_dirs(const char *spec)
{
    for (PkgConfigEntry *entry = g_pkg_config_entries; entry; entry = entry->next)
    {
        if (strcmp(entry->spec, spec) == 0)
        {
            return &entry->dirs;
        }
    }

    PkgConfigEntry *entry = libc_malloc(sizeof(PkgConfigEntry));
    char *spec_copy = c_headers_copy_text(spec, strlen(spec));
    if (!entry || !spec_copy)
    {
        libc_free(entry);
        libc_free(spec_copy);
        return NULL;
    }
    memset(entry, 0, sizeof(PkgConfigEntry));
    entry->spec = spec_copy;
    entry->next = g_pkg_config_entries;
    g_pkg_config_entries = entry;

    if (!is_safe_pkg_config_spec(spec))
    {
        fprintf(stderr, "zls: pkg-config '%s' ignored: invalid characters\n", spec);
        return &entry->dirs;
    }

    char arguments[1024];
    snprintf(arguments, sizeof(arguments), "%s", spec);
    char program[] = "pkg-config";
    char only_include_flags[] = "--cflags-only-I";
    char *argv[40];
    int argc = 0;
    argv[argc++] = program;
    argv[argc++] = only_include_flags;
    for (char *word = strtok(arguments, " "); word && argc < 38; word = strtok(NULL, " "))
    {
        argv[argc++] = word;
    }
    argv[argc] = NULL;

    char output[4096];
    if (z_run_command_capture_ex(argv, output, sizeof(output), 0) == 0)
    {
        for (char *word = strtok(output, " \t\r\n"); word; word = strtok(NULL, " \t\r\n"))
        {
            if (strncmp(word, "-I", 2) == 0)
            {
                string_list_add(&entry->dirs, word + 2);
            }
        }
    }
    fprintf(stderr, "zls: pkg-config %s: %d include directories\n", spec, entry->dirs.count);
    return &entry->dirs;
}

/* --- Search directories --- */

#define MAX_MODULE_DEPTH 8

// Name of a file without its directories, for messages.
static const char *file_label(const char *path)
{
    const char *separator = z_path_last_sep(path);
    return separator ? separator + 1 : path;
}

// Directories named by `//> include:` and `//> pkg-config:` directives in `source`.
// Relative include paths are taken from the project root, which is where `zc build`
// normally runs.
static void collect_directive_dirs(const char *source, const char *file_path, CSearchDirList *dirs)
{
    char file_dir[MAX_PATH_LEN];
    c_headers_directory_of(file_path, file_dir, sizeof(file_dir));
    const char *base_dir = (g_project && g_project->root_path) ? g_project->root_path : file_dir;

    const char *line_start = source;
    while (*line_start)
    {
        const char *line_end = strchr(line_start, '\n');
        size_t length = line_end ? (size_t)(line_end - line_start) : strlen(line_start);
        const char *p = line_start;
        while (p < line_start + length && isspace((unsigned char)*p))
        {
            p++;
        }

        if ((size_t)(line_start + length - p) >= 3 && strncmp(p, "//>", 3) == 0)
        {
            char raw[2048];
            c_headers_copy_trimmed(p + 3, (size_t)(line_start + length - (p + 3)), raw,
                                   sizeof(raw));
            char directive[2048];
            if (resolve_build_directive(raw, directive, sizeof(directive)))
            {
                char origin[MAX_PATH_LEN];
                snprintf(origin, sizeof(origin), "`//> %s` in %s", directive,
                         file_label(file_path));
                if (strncmp(directive, "include:", 8) == 0)
                {
                    for (char *word = strtok(directive + 8, " \t"); word;
                         word = strtok(NULL, " \t"))
                    {
                        char path[MAX_PATH_LEN];
                        c_headers_join_path(base_dir, word, path, sizeof(path));
                        c_search_dirs_add(dirs, path, origin);
                    }
                }
                else if (strncmp(directive, "pkg-config:", 11) == 0)
                {
                    char spec[1024];
                    c_headers_copy_trimmed(directive + 11, strlen(directive + 11), spec,
                                           sizeof(spec));
                    const StringList *found = pkg_config_dirs(spec);
                    for (int i = 0; found && i < found->count; i++)
                    {
                        c_search_dirs_add(dirs, found->items[i], origin);
                    }
                }
            }
        }

        if (!line_end)
        {
            break;
        }
        line_start = line_end + 1;
    }
}

// Where the C compiler will look for headers when building `file_path`: its directives,
// the configured include paths and the compiler's own directories, in the order the
// compiler searches them.
static void collect_search_dirs(const char *source, const char *file_path, CSearchDirList *dirs)
{
    const CompilerConfig *cfg = &lsp_compiler()->config;
    collect_directive_dirs(source, file_path, dirs);
    for (size_t i = 0; i < cfg->include_paths.length; i++)
    {
        c_search_dirs_add(dirs, cfg->include_paths.data[i], "the configured include paths");
    }
    static int logged = 0;
    int status = 0;
    const StringList *compiler_dirs = c_headers_compiler_dirs(cfg->cc, &status);
    if (!logged)
    {
        logged = 1;
        fprintf(stderr, "zls: C compiler '%s' (exit %d) searches %d include directories\n", cfg->cc,
                status, compiler_dirs->count);
        for (int i = 0; i < compiler_dirs->count; i++)
        {
            fprintf(stderr, "zls:   %s\n", compiler_dirs->items[i]);
        }
    }
    char origin[MAX_PATH_LEN];
    snprintf(origin, sizeof(origin), "the default directories of the C compiler (`%s`)", cfg->cc);
    for (int i = 0; i < compiler_dirs->count; i++)
    {
        c_search_dirs_add(dirs, compiler_dirs->items[i], origin);
    }
}

/* --- Imports of a file --- */

typedef struct
{
    char name[MAX_PATH_LEN];
    char alias[MAX_VAR_NAME_LEN]; ///< Empty when imported without `as` or with `as *`.
    int is_system;                ///< `include <x.h>`.
    int is_export;                ///< `export import`.
    int line;                     ///< 0-based line of the import.
    int alias_column;             ///< Column of the alias, -1 without alias.
    int name_column;              ///< Column of the header name.
} HeaderImport;

typedef struct
{
    char name[MAX_PATH_LEN];
    int is_export;
    int line;
    int name_column;
} ModuleImport;

typedef struct
{
    HeaderImport *headers;
    int header_count;
    int header_capacity;
    ModuleImport *modules;
    int module_count;
    int module_capacity;
} FileImports;

static HeaderImport *add_header_import(FileImports *imports)
{
    if (imports->header_count == imports->header_capacity)
    {
        int capacity = imports->header_capacity ? imports->header_capacity * 2 : 4;
        HeaderImport *headers =
            libc_realloc(imports->headers, (size_t)capacity * sizeof(HeaderImport));
        if (!headers)
        {
            return NULL;
        }
        imports->headers = headers;
        imports->header_capacity = capacity;
    }
    HeaderImport *header = &imports->headers[imports->header_count];
    memset(header, 0, sizeof(HeaderImport));
    header->alias_column = -1;
    return header;
}

static ModuleImport *add_module_import(FileImports *imports)
{
    if (imports->module_count == imports->module_capacity)
    {
        int capacity = imports->module_capacity ? imports->module_capacity * 2 : 4;
        ModuleImport *modules =
            libc_realloc(imports->modules, (size_t)capacity * sizeof(ModuleImport));
        if (!modules)
        {
            return NULL;
        }
        imports->modules = modules;
        imports->module_capacity = capacity;
    }
    ModuleImport *module = &imports->modules[imports->module_count];
    memset(module, 0, sizeof(ModuleImport));
    return module;
}

static void free_imports(FileImports *imports)
{
    libc_free(imports->headers);
    libc_free(imports->modules);
    memset(imports, 0, sizeof(FileImports));
}

static int starts_with_word(const char *p, const char *word)
{
    size_t length = strlen(word);
    return strncmp(p, word, length) == 0 && (p[length] == ' ' || p[length] == '\t');
}

static int has_extension(const char *name, const char *extension)
{
    size_t length = strlen(name);
    size_t extension_length = strlen(extension);
    return length > extension_length && strcmp(name + length - extension_length, extension) == 0;
}

// The `import "x.h" [as alias]`, `include <x.h>`/`include "x.h"` and `import "x.zc"` lines
// of a file, with their positions.
static void collect_imports(const char *source, FileImports *imports)
{
    int line = 0;
    const char *line_start = source;
    while (*line_start)
    {
        const char *p = c_headers_skip_blanks(line_start);
        int is_export = starts_with_word(p, "export");
        if (is_export)
        {
            p = c_headers_skip_blanks(p + 6);
        }

        char name[MAX_PATH_LEN];
        if (starts_with_word(p, "import"))
        {
            p = c_headers_skip_blanks(p + 6);
            const char *name_start = p + 1;
            const char *after =
                *p == '"' ? c_headers_read_until(name_start, '"', name, sizeof(name)) : NULL;
            if (after && has_extension(name, ".h"))
            {
                HeaderImport *header = add_header_import(imports);
                if (header)
                {
                    snprintf(header->name, sizeof(header->name), "%s", name);
                    header->is_export = is_export;
                    header->line = line;
                    header->name_column = (int)(name_start - line_start);
                    const char *q = c_headers_skip_blanks(after);
                    if (starts_with_word(q, "as"))
                    {
                        q = c_headers_skip_blanks(q + 2);
                        size_t length = 0;
                        while (c_headers_is_identifier_char(q[length]))
                        {
                            length++;
                        }
                        if (length > 0 && length < sizeof(header->alias))
                        {
                            memcpy(header->alias, q, length);
                            header->alias[length] = '\0';
                            header->alias_column = (int)(q - line_start);
                        }
                    }
                    imports->header_count++;
                }
            }
            else if (after && has_extension(name, ".zc"))
            {
                ModuleImport *module = add_module_import(imports);
                if (module)
                {
                    snprintf(module->name, sizeof(module->name), "%s", name);
                    module->is_export = is_export;
                    module->line = line;
                    module->name_column = (int)(name_start - line_start);
                    imports->module_count++;
                }
            }
        }
        else if (!is_export && starts_with_word(p, "include"))
        {
            p = c_headers_skip_blanks(p + 7);
            if (*p == '<' || *p == '"')
            {
                char close = *p == '<' ? '>' : '"';
                if (c_headers_read_until(p + 1, close, name, sizeof(name)))
                {
                    HeaderImport *header = add_header_import(imports);
                    if (header)
                    {
                        snprintf(header->name, sizeof(header->name), "%s", name);
                        header->is_system = close == '>';
                        header->line = line;
                        header->name_column = (int)(p + 1 - line_start);
                        imports->header_count++;
                    }
                }
            }
        }

        const char *line_end = strchr(line_start, '\n');
        if (!line_end)
        {
            break;
        }
        line_start = line_end + 1;
        line++;
    }
}

// The text of `line` in `source`, trimmed, into `out`.
static void source_line(const char *source, int line, char *out, size_t out_size)
{
    const char *p = source;
    for (int i = 0; i < line && p; i++)
    {
        p = strchr(p, '\n');
        if (p)
        {
            p++;
        }
    }
    if (!p)
    {
        out[0] = '\0';
        return;
    }
    const char *end = strchr(p, '\n');
    c_headers_copy_trimmed(p, end ? (size_t)(end - p) : strlen(p), out, out_size);
}

/* --- Modules imported by a file --- */

// Resolves `import "x.zc"` next to the importing file, then from the project root.
static char *find_module_file(const char *name, const char *from_dir)
{
    char path[MAX_PATH_LEN];
    if (z_is_abs_path(name))
    {
        return c_headers_is_regular_file(name) ? c_headers_copy_text(name, strlen(name)) : NULL;
    }
    c_headers_join_path(from_dir, name, path, sizeof(path));
    if (c_headers_is_regular_file(path))
    {
        return c_headers_copy_text(path, strlen(path));
    }
    if (g_project && g_project->root_path)
    {
        c_headers_join_path(g_project->root_path, name, path, sizeof(path));
        if (c_headers_is_regular_file(path))
        {
            return c_headers_copy_text(path, strlen(path));
        }
    }
    return NULL;
}

// Text of a module: the client's version when it is open, otherwise the file on disk.
static char *module_text(const char *path)
{
    ProjectFile *file = lsp_project_get_file_by_path(path);
    if (file && file->source)
    {
        return c_headers_copy_text(file->source, strlen(file->source));
    }
    return c_headers_read_file(path);
}

typedef struct
{
    const ModuleImport *top; ///< The document's own import that leads to the module.
    const char *module_path;
    const char *module_source;
    const HeaderImport *header;
} ModuleHeader;

typedef int (*ModuleHeaderVisitor)(const ModuleHeader *entry, void *data);

// Visits the C headers imported by the modules that `source` imports, depth first. With
// `only_reexports`, only what they re-export: `export import "x.h"`, through modules they
// `export import` themselves. Stops when `visit` returns non-zero, and returns that value.
static int walk_module_headers(const char *source, const char *file_path, const ModuleImport *top,
                               int only_reexports, int depth, StringList *visited,
                               ModuleHeaderVisitor visit, void *data)
{
    if (depth > MAX_MODULE_DEPTH)
    {
        return 0;
    }
    FileImports imports = {0};
    collect_imports(source, &imports);
    char file_dir[MAX_PATH_LEN];
    c_headers_directory_of(file_path, file_dir, sizeof(file_dir));

    int stop = 0;
    for (int i = 0; i < imports.module_count && !stop; i++)
    {
        const ModuleImport *module = &imports.modules[i];
        if (only_reexports && depth > 0 && !module->is_export)
        {
            continue;
        }
        char *path = find_module_file(module->name, file_dir);
        if (!path || string_list_contains(visited, path))
        {
            libc_free(path);
            continue;
        }
        string_list_add(visited, path);

        char *text = module_text(path);
        if (text)
        {
            const ModuleImport *entry_top = top ? top : module;
            FileImports module_imports = {0};
            collect_imports(text, &module_imports);
            for (int h = 0; h < module_imports.header_count && !stop; h++)
            {
                if (only_reexports && !module_imports.headers[h].is_export)
                {
                    continue;
                }
                ModuleHeader entry = {entry_top, path, text, &module_imports.headers[h]};
                stop = visit(&entry, data);
            }
            free_imports(&module_imports);
            if (!stop)
            {
                stop = walk_module_headers(text, path, entry_top, only_reexports, depth + 1,
                                           visited, visit, data);
            }
            libc_free(text);
        }
        libc_free(path);
    }
    free_imports(&imports);
    return stop;
}

/* --- Symbol under the cursor --- */

typedef struct
{
    char name[MAX_VAR_NAME_LEN];
    char qualifier[MAX_VAR_NAME_LEN]; ///< `alias` of `alias::name`, or empty.
    int is_qualifier;                 ///< The cursor is on `alias` itself.
    int column;                       ///< Column where the identifier starts.
    const char *line_start;
} CursorSymbol;

static int symbol_at(const char *source, int line, int col, CursorSymbol *cursor)
{
    const char *p = source;
    for (int i = 0; i < line && p; i++)
    {
        p = strchr(p, '\n');
        if (p)
        {
            p++;
        }
    }
    if (!p || col < 0)
    {
        return 0;
    }
    const char *line_end = strchr(p, '\n');
    int length = line_end ? (int)(line_end - p) : (int)strlen(p);
    if (col > length)
    {
        col = length;
    }

    int start = col;
    int end = col;
    while (start > 0 && c_headers_is_identifier_char(p[start - 1]))
    {
        start--;
    }
    while (end < length && c_headers_is_identifier_char(p[end]))
    {
        end++;
    }
    if (start == end || isdigit((unsigned char)p[start]) ||
        (size_t)(end - start) >= sizeof(cursor->name))
    {
        return 0;
    }
    memcpy(cursor->name, p + start, (size_t)(end - start));
    cursor->name[end - start] = '\0';
    cursor->column = start;
    cursor->line_start = p;
    cursor->is_qualifier = end + 1 < length && p[end] == ':' && p[end + 1] == ':';

    cursor->qualifier[0] = '\0';
    if (start >= 2 && p[start - 1] == ':' && p[start - 2] == ':')
    {
        int qualifier_end = start - 2;
        int qualifier_start = qualifier_end;
        while (qualifier_start > 0 && c_headers_is_identifier_char(p[qualifier_start - 1]))
        {
            qualifier_start--;
        }
        size_t qualifier_length = (size_t)(qualifier_end - qualifier_start);
        if (qualifier_length > 0 && qualifier_length < sizeof(cursor->qualifier))
        {
            memcpy(cursor->qualifier, p + qualifier_start, qualifier_length);
            cursor->qualifier[qualifier_length] = '\0';
        }
    }
    return 1;
}

static void fill_origin(CHeaderOrigin *origin, const HeaderImport *header, const char *file_path,
                        const char *file_source, int is_re_export, const CSearchDirList *dirs)
{
    snprintf(origin->header, sizeof(origin->header), "%s", header->name);
    snprintf(origin->alias, sizeof(origin->alias), "%s", header->alias);
    snprintf(origin->declared_in, sizeof(origin->declared_in), "%s", file_path);
    source_line(file_source, header->line, origin->statement, sizeof(origin->statement));
    origin->line = header->line;
    origin->alias_column = header->alias_column;
    origin->alias_length = (int)strlen(header->alias);
    origin->name_column = header->name_column;
    origin->is_re_export = is_re_export;

    char file_dir[MAX_PATH_LEN];
    c_headers_directory_of(file_path, file_dir, sizeof(file_dir));
    origin->found_by[0] = '\0';
    char *path = c_headers_find(header->name, header->is_system ? NULL : file_dir, dirs,
                                origin->found_by, sizeof(origin->found_by));
    snprintf(origin->path, sizeof(origin->path), "%s", path ? path : "");
    libc_free(path);
}

// Where to look for a header that a module imports: where the document looks, plus the
// module's own directives. `zc build` ignores those (see lsp_c_headers_add_diagnostics()),
// but the module may well be compiled on its own or with them copied over.
static void module_search_dirs(const CSearchDirList *document_dirs, const ModuleHeader *entry,
                               CSearchDirList *dirs)
{
    c_search_dirs_append(dirs, document_dirs);
    collect_directive_dirs(entry->module_source, entry->module_path, dirs);
}

typedef struct
{
    const char *qualifier;
    const char *name;
    const CSearchDirList *document_dirs;
    StringList *visited_headers;
    const CHeaderSymbol **results;
    int max_results;
    CHeaderOrigin *via;
    int found;
} ReexportSearch;

static int search_reexport(const ModuleHeader *entry, void *data)
{
    ReexportSearch *search = data;
    if (search->qualifier[0] && strcmp(entry->header->alias, search->qualifier) != 0)
    {
        return 0;
    }
    CSearchDirList dirs = {0};
    module_search_dirs(search->document_dirs, entry, &dirs);
    CHeaderOrigin origin;
    fill_origin(&origin, entry->header, entry->module_path, entry->module_source, 1, &dirs);
    if (origin.path[0])
    {
        search->found =
            c_headers_search_tree(origin.path, search->name, &dirs, search->visited_headers,
                                  search->results, search->max_results, 0);
    }
    c_search_dirs_free(&dirs);
    if (search->found > 0 && search->via)
    {
        *search->via = origin;
    }
    // A qualified name stops at its alias, whether the header declares it or not.
    return search->found > 0 || search->qualifier[0];
}

// The declarations of `name` in the headers a document imports, only those imported as
// `qualifier` when it is not empty, and then in the ones its imported modules re-export.
static int find_named(const char *document_path, const char *source, const char *qualifier,
                      const char *name, const CHeaderSymbol **results, int max_results,
                      CHeaderOrigin *via)
{
    FileImports imports = {0};
    collect_imports(source, &imports);
    CSearchDirList dirs = {0};
    collect_search_dirs(source, document_path, &dirs);
    char document_dir[MAX_PATH_LEN];
    c_headers_directory_of(document_path, document_dir, sizeof(document_dir));

    StringList visited = {0};
    int found = 0;
    for (int i = 0; i < imports.header_count && found == 0; i++)
    {
        const HeaderImport *header = &imports.headers[i];
        if (qualifier[0] && strcmp(header->alias, qualifier) != 0)
        {
            continue;
        }
        char *path =
            c_headers_find(header->name, header->is_system ? NULL : document_dir, &dirs, NULL, 0);
        if (!path)
        {
            fprintf(stderr, "zls: C header '%s' not found\n", header->name);
            continue;
        }
        found = c_headers_search_tree(path, name, &dirs, &visited, results, max_results, 0);
        libc_free(path);
    }

    if (found == 0)
    {
        // Headers that imported modules re-export (`export import "x.h" as alias`).
        ReexportSearch search = {qualifier, name, &dirs, &visited, results, max_results, via, 0};
        StringList visited_modules = {0};
        walk_module_headers(source, document_path, NULL, 1, 0, &visited_modules, search_reexport,
                            &search);
        string_list_free(&visited_modules);
        found = search.found;
    }

    string_list_free(&visited);
    c_search_dirs_free(&dirs);
    free_imports(&imports);
    return found;
}

int lsp_c_headers_find_at(const char *document_path, const char *source, int line, int col,
                          const CHeaderSymbol **results, int max_results, CHeaderOrigin *via)
{
    if (via)
    {
        via->is_re_export = 0;
        via->declared_in[0] = '\0';
    }
    CursorSymbol cursor;
    if (!document_path || !source || max_results <= 0 || !symbol_at(source, line, col, &cursor) ||
        cursor.is_qualifier)
    {
        return 0;
    }
    return find_named(document_path, source, cursor.qualifier, cursor.name, results, max_results,
                      via);
}

int lsp_c_headers_is_extern_name(const char *source, int line, int col)
{
    CursorSymbol cursor;
    if (!source || !symbol_at(source, line, col, &cursor) || cursor.qualifier[0] ||
        cursor.is_qualifier)
    {
        return 0;
    }
    // `extern fn name`: walk back over `fn` and `extern`.
    const char *p = cursor.line_start + cursor.column;
    const char *const keywords[] = {"fn", "extern"};
    for (int k = 0; k < 2; k++)
    {
        while (p > cursor.line_start && (p[-1] == ' ' || p[-1] == '\t'))
        {
            p--;
        }
        size_t length = strlen(keywords[k]);
        if ((size_t)(p - cursor.line_start) < length ||
            strncmp(p - length, keywords[k], length) != 0)
        {
            return 0;
        }
        p -= length;
        if (p > cursor.line_start && c_headers_is_identifier_char(p[-1]))
        {
            return 0;
        }
    }
    return 1;
}

typedef struct
{
    const char *alias;
    const CSearchDirList *document_dirs;
    CHeaderOrigin *origin;
} AliasSearch;

static int find_reexported_alias(const ModuleHeader *entry, void *data)
{
    AliasSearch *search = data;
    if (strcmp(entry->header->alias, search->alias) != 0)
    {
        return 0;
    }
    CSearchDirList dirs = {0};
    module_search_dirs(search->document_dirs, entry, &dirs);
    fill_origin(search->origin, entry->header, entry->module_path, entry->module_source, 1, &dirs);
    c_search_dirs_free(&dirs);
    return 1;
}

CHeaderOriginKind lsp_c_headers_origin_at(const char *document_path, const char *source, int line,
                                          int col, CHeaderOrigin *origin)
{
    if (!document_path || !source)
    {
        return C_HEADER_NONE;
    }
    FileImports imports = {0};
    collect_imports(source, &imports);
    CSearchDirList dirs = {0};
    CHeaderOriginKind kind = C_HEADER_NONE;

    // On an import line of the document: its alias or its header name.
    for (int i = 0; i < imports.header_count && kind == C_HEADER_NONE; i++)
    {
        const HeaderImport *header = &imports.headers[i];
        if (header->line != line)
        {
            continue;
        }
        int alias_end = header->alias_column + (int)strlen(header->alias);
        int name_end = header->name_column + (int)strlen(header->name);
        if (header->alias_column >= 0 && col >= header->alias_column && col <= alias_end)
        {
            kind = C_HEADER_ALIAS_DECLARATION;
        }
        else if (col >= header->name_column && col <= name_end)
        {
            kind = C_HEADER_NAME;
        }
        if (kind != C_HEADER_NONE)
        {
            collect_search_dirs(source, document_path, &dirs);
            fill_origin(origin, header, document_path, source, 0, &dirs);
        }
    }

    // On `alias` of `alias::name`: imported here, or re-exported by an imported module.
    CursorSymbol cursor;
    if (kind == C_HEADER_NONE && symbol_at(source, line, col, &cursor) && cursor.is_qualifier)
    {
        collect_search_dirs(source, document_path, &dirs);
        for (int i = 0; i < imports.header_count && kind == C_HEADER_NONE; i++)
        {
            if (strcmp(imports.headers[i].alias, cursor.name) == 0)
            {
                fill_origin(origin, &imports.headers[i], document_path, source, 0, &dirs);
                kind = C_HEADER_ALIAS_USE;
            }
        }
        if (kind == C_HEADER_NONE)
        {
            AliasSearch search = {cursor.name, &dirs, origin};
            StringList visited = {0};
            if (walk_module_headers(source, document_path, NULL, 1, 0, &visited,
                                    find_reexported_alias, &search))
            {
                kind = C_HEADER_ALIAS_USE;
            }
            string_list_free(&visited);
        }
    }

    c_search_dirs_free(&dirs);
    free_imports(&imports);
    return kind;
}

/* --- Hover --- */

// The text of `line` in `header`, without its line break.
static const char *line_text(const CHeaderFile *header, int line, int *length)
{
    const char *start = header->text + header->line_offsets[line];
    const char *end = strchr(start, '\n');
    int size = end ? (int)(end - start) : (int)strlen(start);
    while (size > 0 && start[size - 1] == '\r')
    {
        size--;
    }
    *length = size;
    return start;
}

// Column where a comment starts on the line, outside string literals, or -1.
static int comment_column(const char *text, int length)
{
    char quote = 0;
    for (int i = 0; i < length; i++)
    {
        if (quote)
        {
            if (text[i] == '\\')
            {
                i++;
            }
            else if (text[i] == quote)
            {
                quote = 0;
            }
        }
        else if (text[i] == '"' || text[i] == '\'')
        {
            quote = text[i];
        }
        else if (text[i] == '/' && i + 1 < length && (text[i + 1] == '/' || text[i + 1] == '*'))
        {
            return i;
        }
    }
    return -1;
}

// Appends the text of a comment line without its markers (`//`, `/*`, `*/`, leading `*`).
static void append_comment_text(char *doc, size_t doc_size, const char *text, int length)
{
    char line[MAX_DOC_LENGTH];
    c_headers_copy_trimmed(text, (size_t)length, line, sizeof(line));
    char *p = line;
    if (strncmp(p, "/*", 2) == 0)
    {
        p += 2;
    }
    while (*p == '/' || *p == '*' || *p == '!' || *p == '<')
    {
        p++;
    }
    char *end = strstr(p, "*/");
    if (end)
    {
        *end = '\0';
    }
    char cleaned[MAX_DOC_LENGTH];
    c_headers_copy_trimmed(p, strlen(p), cleaned, sizeof(cleaned));
    if (!cleaned[0])
    {
        return;
    }
    size_t used = strlen(doc);
    snprintf(doc + used, doc_size - used, "%s%s", used ? "\n" : "", cleaned);
}

// The comment block right above `line`, or nothing.
static void leading_comment(const CHeaderFile *header, int line, char *doc, size_t doc_size)
{
    int first = line;
    int length;
    for (int k = line - 1; k >= 0 && line - k <= 40; k--)
    {
        const char *text = line_text(header, k, &length);
        char trimmed[MAX_DOC_LENGTH];
        c_headers_copy_trimmed(text, (size_t)length, trimmed, sizeof(trimmed));
        size_t trimmed_length = strlen(trimmed);
        int is_comment = strncmp(trimmed, "//", 2) == 0 || strncmp(trimmed, "/*", 2) == 0 ||
                         trimmed[0] == '*' ||
                         (trimmed_length >= 2 && strcmp(trimmed + trimmed_length - 2, "*/") == 0);
        if (!is_comment)
        {
            break;
        }
        first = k;
    }
    for (int k = first; k < line; k++)
    {
        const char *text = line_text(header, k, &length);
        append_comment_text(doc, doc_size, text, length);
    }
}

// Appends `text` to the growing libc buffer `*out`.
static void append(char **out, size_t *used, size_t *capacity, const char *text, size_t length)
{
    if (!*out)
    {
        return;
    }
    if (*used + length + 1 > *capacity)
    {
        size_t new_capacity = (*used + length + 1) * 2;
        char *grown = libc_realloc(*out, new_capacity);
        if (!grown)
        {
            return;
        }
        *out = grown;
        *capacity = new_capacity;
    }
    memcpy(*out + *used, text, length);
    *used += length;
    (*out)[*used] = '\0';
}

char *lsp_c_headers_hover(const CHeaderSymbol *symbol, const CHeaderOrigin *via)
{
    CHeaderFile *header = symbol ? c_headers_cached(symbol->path) : NULL;
    if (!header || !header->text || symbol->end_line >= header->line_count)
    {
        return NULL;
    }

    int first = symbol->start_line;
    int last = symbol->end_line;
    int truncated = last - first >= MAX_HOVER_LINES;
    if (truncated)
    {
        last = first + MAX_HOVER_LINES - 1;
    }

    // A one-line declaration usually carries its documentation as a trailing comment.
    char doc[MAX_DOC_LENGTH];
    doc[0] = '\0';
    int last_length;
    const char *last_text = line_text(header, last, &last_length);
    if (first == last)
    {
        int comment = comment_column(last_text, last_length);
        if (comment >= 0)
        {
            append_comment_text(doc, sizeof(doc), last_text + comment, last_length - comment);
            last_length = comment;
        }
    }
    if (!doc[0])
    {
        leading_comment(header, first, doc, sizeof(doc));
    }

    // Indentation shared by every line of the declaration.
    int indent = -1;
    for (int k = first; k <= last; k++)
    {
        int length;
        const char *text = line_text(header, k, &length);
        int spaces = 0;
        while (spaces < length && (text[spaces] == ' ' || text[spaces] == '\t'))
        {
            spaces++;
        }
        if (spaces < length && (indent < 0 || spaces < indent))
        {
            indent = spaces;
        }
    }
    if (indent < 0)
    {
        indent = 0;
    }

    size_t used = 0;
    size_t capacity = 512;
    char *out = libc_malloc(capacity);
    if (!out)
    {
        return NULL;
    }
    out[0] = '\0';
    append(&out, &used, &capacity, "```c\n", 5);
    for (int k = first; k <= last; k++)
    {
        int length;
        const char *text = line_text(header, k, &length);
        if (k == last)
        {
            length = last_length;
        }
        while (length > 0 && isspace((unsigned char)text[length - 1]))
        {
            length--;
        }
        int skip = indent < length ? indent : length;
        append(&out, &used, &capacity, text + skip, (size_t)(length - skip));
        append(&out, &used, &capacity, "\n", 1);
    }
    if (truncated)
    {
        append(&out, &used, &capacity, "...\n", 4);
    }
    append(&out, &used, &capacity, "```\n", 4);
    if (doc[0])
    {
        append(&out, &used, &capacity, doc, strlen(doc));
        append(&out, &used, &capacity, "\n\n", 2);
    }
    char location[MAX_PATH_LEN + 32];
    snprintf(location, sizeof(location), "`%s:%d`", symbol->path, symbol->line + 1);
    append(&out, &used, &capacity, location, strlen(location));
    if (via && via->is_re_export)
    {
        char reexport[MAX_PATH_LEN + 600];
        snprintf(reexport, sizeof(reexport), "\n\nRe-exported by `%s`:\n```zc\n%s\n```",
                 file_label(via->declared_in), via->statement);
        append(&out, &used, &capacity, reexport, strlen(reexport));
    }
    return out;
}

char *lsp_c_headers_origin_hover(const CHeaderOrigin *origin, CHeaderOriginKind kind)
{
    char text[MAX_PATH_LEN * 3];
    size_t used = 0;
    if (kind == C_HEADER_NAME)
    {
        str_appendf(text, sizeof(text), &used, "C header `%s`", origin->header);
    }
    else
    {
        str_appendf(text, sizeof(text), &used, "`%s`: alias of the C header `%s`", origin->alias,
                    origin->header);
    }
    if (origin->is_re_export)
    {
        str_appendf(text, sizeof(text), &used, "\n\nRe-exported by `%s`:\n```zc\n%s\n```",
                    file_label(origin->declared_in), origin->statement);
    }
    else if (kind == C_HEADER_ALIAS_USE)
    {
        str_appendf(text, sizeof(text), &used, "\n\nImported at line %d:\n```zc\n%s\n```",
                    origin->line + 1, origin->statement);
    }
    if (origin->path[0])
    {
        str_appendf(text, sizeof(text), &used, "\n\n`%s`\n\nFound through %s.", origin->path,
                    origin->found_by);
    }
    else
    {
        str_appendf(text, sizeof(text), &used,
                    "\n\nNot found where the C compiler will look for it.");
    }
    return c_headers_copy_text(text, strlen(text));
}

/* --- Completion --- */

#define MAX_COMPLETION_ITEMS 2000

// `typed` matches `name` when its characters appear in it in order, ignoring case, as the
// client filters.
static int matches_typed(const char *name, const char *typed)
{
    for (; *typed; typed++)
    {
        int wanted = tolower((unsigned char)*typed);
        while (*name && tolower((unsigned char)*name) != wanted)
        {
            name++;
        }
        if (!*name)
        {
            return 0;
        }
        name++;
    }
    return 1;
}

// The line that declares the name, without its comment.
static void symbol_detail(const CHeaderFile *header, const CHeaderSymbol *symbol, char *detail,
                          size_t detail_size)
{
    int length;
    const char *text = line_text(header, symbol->line, &length);
    int comment = comment_column(text, length);
    c_headers_copy_trimmed(text, (size_t)(comment >= 0 ? comment : length), detail, detail_size);
}

// The comment on the declaration's last line, or the comment block above it.
static void symbol_documentation(const CHeaderFile *header, const CHeaderSymbol *symbol, char *doc,
                                 size_t doc_size)
{
    doc[0] = '\0';
    int length;
    const char *text = line_text(header, symbol->end_line, &length);
    int comment = comment_column(text, length);
    if (comment >= 0)
    {
        append_comment_text(doc, doc_size, text + comment, length - comment);
    }
    if (!doc[0])
    {
        leading_comment(header, symbol->start_line, doc, doc_size);
    }
}

// The completion kind of a C declaration of that kind.
static LspCompletionItemKind completion_kind(CSymbolKind kind)
{
    switch (kind)
    {
    case C_SYMBOL_FUNCTION:
        return LSP_COMPLETION_FUNCTION;
    case C_SYMBOL_VARIABLE:
        return LSP_COMPLETION_VARIABLE;
    case C_SYMBOL_ENUM:
        return LSP_COMPLETION_ENUM;
    case C_SYMBOL_ENUM_MEMBER:
        return LSP_COMPLETION_ENUM_MEMBER;
    case C_SYMBOL_CONSTANT:
        return LSP_COMPLETION_CONSTANT;
    case C_SYMBOL_STRUCT:
        return LSP_COMPLETION_STRUCT;
    case C_SYMBOL_TYPE:
        return LSP_COMPLETION_TYPE_PARAMETER;
    default:
        return LSP_COMPLETION_VARIABLE;
    }
}

// Completion items for the declarations of `path` matching `typed`, then those of the headers
// it includes with quotes: `#include "x.h"` belongs to the same library, while `<x.h>` brings
// in the system or other libraries.
static void add_completion_items(const char *path, const char *typed, const CSearchDirList *dirs,
                                 StringList *visited, StringList *names, cJSON *items, int depth)
{
    if (depth > C_HEADERS_MAX_INCLUDE_DEPTH || string_list_contains(visited, path))
    {
        return;
    }
    string_list_add(visited, path);
    CHeaderFile *header = c_headers_get(path);
    if (!header || !header->text)
    {
        return;
    }

    for (int i = 0; i < header->symbol_count && names->count < MAX_COMPLETION_ITEMS; i++)
    {
        const CHeaderEntry *entry = &header->symbols[i];
        // Reserved names (`__declspec`, `_X`) belong to the implementation, not the library.
        if (c_headers_is_reserved_identifier(entry->name) || !matches_typed(entry->name, typed) ||
            string_list_contains(names, entry->name))
        {
            continue;
        }
        string_list_add(names, entry->name);
        char detail[256];
        symbol_detail(header, &entry->symbol, detail, sizeof(detail));
        char doc[MAX_DOC_LENGTH];
        symbol_documentation(header, &entry->symbol, doc, sizeof(doc));

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", entry->name);
        cJSON_AddNumberToObject(item, "kind", completion_kind(entry->kind));
        cJSON_AddStringToObject(item, "detail", detail);
        if (doc[0])
        {
            cJSON_AddStringToObject(item, "documentation", doc);
        }
        cJSON_AddItemToArray(items, item);
    }

    char header_dir[MAX_PATH_LEN];
    c_headers_directory_of(path, header_dir, sizeof(header_dir));
    for (int i = 0; i < header->include_count; i++)
    {
        if (header->includes[i].is_system)
        {
            continue;
        }
        char *child = c_headers_find(header->includes[i].name, header_dir, dirs, NULL, 0);
        if (child)
        {
            add_completion_items(child, typed, dirs, visited, names, items, depth + 1);
            libc_free(child);
        }
    }
}

struct cJSON *lsp_c_headers_completion(const char *document_path, const char *source, int line,
                                       int col)
{
    if (!document_path || !source || col < 0)
    {
        return NULL;
    }
    const char *p = source;
    for (int i = 0; i < line && p; i++)
    {
        p = strchr(p, '\n');
        if (p)
        {
            p++;
        }
    }
    if (!p)
    {
        return NULL;
    }
    const char *line_end = strchr(p, '\n');
    int length = line_end ? (int)(line_end - p) : (int)strlen(p);
    if (col > length)
    {
        col = length;
    }

    // `alias::typed|`
    int typed_start = col;
    while (typed_start > 0 && c_headers_is_identifier_char(p[typed_start - 1]))
    {
        typed_start--;
    }
    if (typed_start < 3 || p[typed_start - 1] != ':' || p[typed_start - 2] != ':')
    {
        return NULL;
    }
    int alias_end = typed_start - 2;
    int alias_start = alias_end;
    while (alias_start > 0 && c_headers_is_identifier_char(p[alias_start - 1]))
    {
        alias_start--;
    }
    char typed[MAX_VAR_NAME_LEN];
    size_t typed_length = (size_t)(col - typed_start);
    if (alias_start == alias_end || typed_length >= sizeof(typed))
    {
        return NULL;
    }
    memcpy(typed, p + typed_start, typed_length);
    typed[typed_length] = '\0';

    CHeaderOrigin origin;
    if (lsp_c_headers_origin_at(document_path, source, line, alias_start, &origin) !=
            C_HEADER_ALIAS_USE ||
        !origin.path[0])
    {
        return NULL;
    }

    CSearchDirList dirs = {0};
    collect_search_dirs(source, document_path, &dirs);
    StringList visited = {0};
    StringList names = {0};
    cJSON *items = cJSON_CreateArray();
    add_completion_items(origin.path, typed, &dirs, &visited, &names, items, 0);
    string_list_free(&names);
    string_list_free(&visited);
    c_search_dirs_free(&dirs);
    return items;
}

/* --- Locations --- */

/* --- Struct fields --- */

typedef struct
{
    char name[MAX_VAR_NAME_LEN];
    char type[MAX_SHORT_MSG_LEN];
} CField;

typedef struct
{
    CField fields[MAX_C_FIELDS];
    int count;
} CFieldList;

// The text of a declaration with its comments and preprocessor lines turned into blanks.
// Release it with libc_free().
static char *declaration_text(const CHeaderSymbol *symbol)
{
    CHeaderFile *header = c_headers_get(symbol->path);
    if (!header || !header->text || symbol->end_line >= header->line_count)
    {
        return NULL;
    }
    int start = header->line_offsets[symbol->start_line];
    int end = symbol->end_line + 1 < header->line_count ? header->line_offsets[symbol->end_line + 1]
                                                        : (int)strlen(header->text);
    char *text = c_headers_copy_text(header->text + start, (size_t)(end - start));
    if (!text)
    {
        return NULL;
    }
    for (char *p = text; *p; p++)
    {
        if (p[0] == '/' && p[1] == '/')
        {
            while (*p && *p != '\n')
            {
                *p++ = ' ';
            }
        }
        else if (p[0] == '/' && p[1] == '*')
        {
            while (*p && !(p[0] == '*' && p[1] == '/'))
            {
                *p = *p == '\n' ? '\n' : ' ';
                p++;
            }
            if (*p)
            {
                *p++ = ' ';
                *p = ' ';
            }
        }
        if (!*p)
        {
            break;
        }
    }
    for (char *line = text; line;)
    {
        if (*c_headers_skip_blanks(line) == '#')
        {
            while (*line && *line != '\n')
            {
                *line++ = ' ';
            }
        }
        line = strchr(line, '\n');
        line = line ? line + 1 : NULL;
    }
    return text;
}

// The position of the bracket that closes the one at `open`, or -1.
static int matching_close(const char *text, int open, int length)
{
    int depth = 0;
    for (int i = open; i < length; i++)
    {
        if (text[i] == '{' || text[i] == '(' || text[i] == '[')
        {
            depth++;
        }
        else if ((text[i] == '}' || text[i] == ')' || text[i] == ']') && --depth == 0)
        {
            return i;
        }
    }
    return -1;
}

// Copies `text[0..length)` into `out` with each run of whitespace as one space, and none at
// the ends.
static void copy_collapsed(const char *text, int length, char *out, size_t out_size)
{
    size_t used = 0;
    int space = 0;
    for (int i = 0; i < length && used + 1 < out_size; i++)
    {
        if (isspace((unsigned char)text[i]))
        {
            space = used > 0;
            continue;
        }
        if (space && used + 2 < out_size)
        {
            out[used++] = ' ';
        }
        space = 0;
        out[used++] = text[i];
    }
    out[used] = '\0';
}

// Adds the field that `declarator` declares (`*name`, `name[4]`, `name : 3`) with the type
// written before it, `base`.
static void add_field(CFieldList *list, const char *base, const char *declarator, int length)
{
    int stars = 0;
    int name = -1;
    int name_length = 0;
    int suffix = length;
    for (int i = 0; i < length; i++)
    {
        if (declarator[i] == '*' && name < 0)
        {
            stars++;
        }
        else if (name < 0 && c_headers_is_identifier_char(declarator[i]) &&
                 !isdigit((unsigned char)declarator[i]))
        {
            name = i;
            while (i < length && c_headers_is_identifier_char(declarator[i]))
            {
                i++;
            }
            name_length = i - name;
            i--;
        }
        else if (name >= 0 && declarator[i] == '[')
        {
            suffix = i;
            break;
        }
        else if (name >= 0 && declarator[i] == ':')
        {
            // The width of a bit-field is not part of the type.
            suffix = length = i;
            break;
        }
    }
    if (name < 0 || name_length >= MAX_VAR_NAME_LEN || list->count >= MAX_C_FIELDS)
    {
        return;
    }
    CField *field = &list->fields[list->count++];
    memcpy(field->name, declarator + name, (size_t)name_length);
    field->name[name_length] = '\0';
    char type[MAX_SHORT_MSG_LEN];
    snprintf(type, sizeof(type), "%s%s%.*s%.*s", base, stars ? " " : "", stars, "**********",
             length - suffix, declarator + suffix);
    copy_collapsed(type, (int)strlen(type), field->type, sizeof(field->type));
}

// Adds a field for each declarator of `text`, separated by top-level commas.
static void add_declarators(CFieldList *list, const char *base, const char *text, int length)
{
    int start = 0;
    int depth = 0;
    for (int i = 0; i <= length; i++)
    {
        if (i == length || (text[i] == ',' && depth == 0))
        {
            add_field(list, base, text + start, i - start);
            start = i + 1;
        }
        else if (text[i] == '[' || text[i] == '(')
        {
            depth++;
        }
        else if (text[i] == ']' || text[i] == ')')
        {
            depth--;
        }
    }
}

static void parse_fields(const char *text, int length, CFieldList *list);

// One member of a struct or union, without its `;`.
static void parse_member(const char *text, int length, CFieldList *list)
{
    char base[MAX_SHORT_MSG_LEN];
    for (int i = 0; i < length; i++)
    {
        if (text[i] == '{')
        {
            // struct { ... } name;   or, with no name, a member whose fields are the outer's.
            int close = matching_close(text, i, length);
            if (close < 0)
            {
                return;
            }
            int named = 0;
            for (int j = close + 1; j < length; j++)
            {
                named |= c_headers_is_identifier_char(text[j]);
            }
            if (!named)
            {
                parse_fields(text + i + 1, close - i - 1, list);
                return;
            }
            copy_collapsed(text, i, base, sizeof(base));
            add_declarators(list, base, text + close + 1, length - close - 1);
            return;
        }
        if (text[i] == '(' && *c_headers_skip_blanks(text + i + 1) == '*')
        {
            // void (*callback)(int);   the whole member is the type.
            int count = list->count;
            add_field(list, "", text + i + 1, length - i - 1);
            if (list->count > count)
            {
                copy_collapsed(text, length, list->fields[count].type,
                               sizeof(list->fields[count].type));
            }
            return;
        }
    }

    // unsigned char r;   float x, y;   char *name, buffer[16];   int flag : 1;
    // The type ends where the first declarator starts: its name and the `*` before it.
    int name = -1;
    for (int i = 0; i < length && text[i] != ',' && text[i] != '[' && text[i] != ':'; i++)
    {
        if (c_headers_is_identifier_char(text[i]) &&
            (i == 0 || !c_headers_is_identifier_char(text[i - 1])))
        {
            name = i;
        }
    }
    int type_end = name;
    while (type_end > 0 &&
           (text[type_end - 1] == '*' || isspace((unsigned char)text[type_end - 1])))
    {
        type_end--;
    }
    if (type_end <= 0)
    {
        return;
    }
    copy_collapsed(text, type_end, base, sizeof(base));
    add_declarators(list, base, text + type_end, length - type_end);
}

// The members between the braces of a struct or union.
static void parse_fields(const char *text, int length, CFieldList *list)
{
    int start = 0;
    int depth = 0;
    for (int i = 0; i < length; i++)
    {
        if (text[i] == '{')
        {
            depth++;
        }
        else if (text[i] == '}')
        {
            depth--;
        }
        else if (text[i] == ';' && depth == 0)
        {
            parse_member(text + start, i - start, list);
            start = i + 1;
        }
    }
}

// The name a body-less typedef stands for: `Tag` in `typedef struct Tag Name;`, `Vector4` in
// `typedef Vector4 Quaternion;`. Empty when there is none.
static void typedef_target(const char *text, char *out, size_t out_size)
{
    out[0] = '\0';
    const char *p = strstr(text, "typedef");
    if (!p)
    {
        return;
    }
    p += 7;
    while (*p)
    {
        while (isspace((unsigned char)*p))
        {
            p++;
        }
        const char *word = p;
        while (c_headers_is_identifier_char(*p))
        {
            p++;
        }
        size_t length = (size_t)(p - word);
        if (length == 0)
        {
            return;
        }
        if ((length == 6 && strncmp(word, "struct", 6) == 0) ||
            (length == 5 && strncmp(word, "union", 5) == 0) ||
            (length == 5 && strncmp(word, "const", 5) == 0))
        {
            continue;
        }
        c_headers_copy_trimmed(word, length, out, out_size);
        return;
    }
}

// Reads into `list` the fields of the C struct or union `name` as the headers of a document
// declare it, following typedefs of another name to the declaration with the body.
static void struct_fields(const char *document_path, const char *source, const char *name,
                          CFieldList *list)
{
    list->count = 0;
    char wanted[MAX_VAR_NAME_LEN];
    snprintf(wanted, sizeof(wanted), "%s", name);
    for (int hop = 0; hop < 4 && list->count == 0 && wanted[0]; hop++)
    {
        const CHeaderSymbol *symbols[8];
        int found = find_named(document_path, source, "", wanted, symbols, 8, NULL);
        char target[MAX_VAR_NAME_LEN] = "";
        for (int i = 0; i < found && list->count == 0; i++)
        {
            char *text = declaration_text(symbols[i]);
            if (!text)
            {
                continue;
            }
            const char *body = strchr(text, '{');
            if (body && (strstr(text, "struct") || strstr(text, "union")))
            {
                int open = (int)(body - text);
                int close = matching_close(text, open, (int)strlen(text));
                if (close > open)
                {
                    parse_fields(body + 1, close - open - 1, list);
                }
            }
            else if (!body && !target[0])
            {
                typedef_target(text, target, sizeof(target));
            }
            libc_free(text);
        }
        if (strcmp(target, wanted) == 0)
        {
            break;
        }
        snprintf(wanted, sizeof(wanted), "%s", target);
    }
}

int lsp_c_headers_add_fields(const char *document_path, const char *source, const char *name,
                             struct cJSON *items)
{
    CFieldList *list = libc_malloc(sizeof(CFieldList));
    if (!list)
    {
        return 0;
    }
    struct_fields(document_path, source, name, list);
    int count = list->count;
    for (int i = 0; i < count; i++)
    {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", list->fields[i].name);
        cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_FIELD);
        cJSON_AddStringToObject(item, "detail", list->fields[i].type);
        cJSON_AddItemToArray(items, item);
    }
    libc_free(list);
    return count;
}

int lsp_c_headers_field_type(const char *document_path, const char *source, const char *name,
                             const char *field, char *type, size_t type_size)
{
    CFieldList *list = libc_malloc(sizeof(CFieldList));
    if (!list)
    {
        return 0;
    }
    struct_fields(document_path, source, name, list);
    int found = 0;
    for (int i = 0; i < list->count && !found; i++)
    {
        if (strcmp(list->fields[i].name, field) == 0)
        {
            snprintf(type, type_size, "%s", list->fields[i].type);
            found = 1;
        }
    }
    libc_free(list);
    return found;
}

static cJSON *make_range(int line, int column, int length)
{
    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", line);
    cJSON_AddNumberToObject(start, "character", column);
    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", line);
    cJSON_AddNumberToObject(end, "character", column + length);
    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddItemToObject(range, "end", end);
    return range;
}

static cJSON *make_location(const char *path, int line, int column, int length)
{
    cJSON *location = cJSON_CreateObject();
    cJSON_AddStringToObject(location, "uri", lsp_path_to_uri(path));
    cJSON_AddItemToObject(location, "range", make_range(line, column, length));
    return location;
}

struct cJSON *lsp_c_headers_locations(const CHeaderSymbol **symbols, int count)
{
    cJSON *locations = cJSON_CreateArray();
    for (int i = 0; i < count; i++)
    {
        cJSON *location = make_location(symbols[i]->path, symbols[i]->line, symbols[i]->column,
                                        symbols[i]->length);
        if (location)
        {
            cJSON_AddItemToArray(locations, location);
        }
    }
    return locations;
}

struct cJSON *lsp_c_headers_origin_range(const CHeaderOrigin *origin, CHeaderOriginKind kind)
{
    if (kind == C_HEADER_NAME)
    {
        return make_range(origin->line, origin->name_column, (int)strlen(origin->header));
    }
    if (kind == C_HEADER_ALIAS_DECLARATION)
    {
        return make_range(origin->line, origin->alias_column, origin->alias_length);
    }
    return NULL;
}

struct cJSON *lsp_c_headers_origin_location(const CHeaderOrigin *origin, CHeaderOriginKind kind,
                                            int link_support)
{
    if (kind == C_HEADER_ALIAS_USE)
    {
        return make_location(origin->declared_in, origin->line, origin->alias_column,
                             origin->alias_length);
    }
    if (!origin->path[0])
    {
        return NULL;
    }
    if (!link_support)
    {
        return make_location(origin->path, 0, 0, 0);
    }
    cJSON *link = cJSON_CreateObject();
    cJSON_AddItemToObject(link, "originSelectionRange", lsp_c_headers_origin_range(origin, kind));
    cJSON_AddStringToObject(link, "targetUri", lsp_path_to_uri(origin->path));
    cJSON_AddItemToObject(link, "targetRange", make_range(0, 0, 0));
    cJSON_AddItemToObject(link, "targetSelectionRange", make_range(0, 0, 0));
    cJSON *links = cJSON_CreateArray();
    cJSON_AddItemToArray(links, link);
    return links;
}

/* --- Diagnostics --- */

typedef struct
{
    const CSearchDirList *document_dirs;
    struct cJSON *diagnostics;
    StringList warned;
} DirectiveCheck;

// `zc build` only applies the `//>` directives of the file it compiles, but every header
// of every imported module ends up in the generated C. Warns about the headers that only a
// module's own directive makes findable.
static int check_module_header(const ModuleHeader *entry, void *data)
{
    DirectiveCheck *check = data;
    const HeaderImport *header = entry->header;
    if (string_list_contains(&check->warned, header->name))
    {
        return 0;
    }
    char module_dir[MAX_PATH_LEN];
    c_headers_directory_of(entry->module_path, module_dir, sizeof(module_dir));
    char *path = c_headers_find(header->name, header->is_system ? NULL : module_dir,
                                check->document_dirs, NULL, 0);
    if (path)
    {
        libc_free(path);
        return 0;
    }

    CSearchDirList module_dirs = {0};
    collect_directive_dirs(entry->module_source, entry->module_path, &module_dirs);
    char found_by[MAX_PATH_LEN];
    path = c_headers_find(header->name, NULL, &module_dirs, found_by, sizeof(found_by));
    c_search_dirs_free(&module_dirs);
    if (!path)
    {
        return 0;
    }
    libc_free(path);
    string_list_add(&check->warned, header->name);

    // Three paths may not fit: a message cut at the buffer size still points at the problem.
    char message[MAX_PATH_LEN * 2];
    int written = snprintf(message, sizeof(message),
                           "C header '%s' (imported by %s) is only found through %s, but zc "
                           "build only applies the directives of the file it compiles: add "
                           "that directive here.",
                           header->name, file_label(entry->module_path), found_by);
    if (written < 0)
    {
        return 0;
    }
    cJSON *diagnostic = cJSON_CreateObject();
    cJSON_AddItemToObject(
        diagnostic, "range",
        make_range(entry->top->line, entry->top->name_column, (int)strlen(entry->top->name)));
    cJSON_AddNumberToObject(diagnostic, "severity", LSP_SEVERITY_WARNING);
    cJSON_AddStringToObject(diagnostic, "message", message);
    cJSON_AddItemToArray(check->diagnostics, diagnostic);
    return 0;
}

void lsp_c_headers_add_diagnostics(const char *document_path, const char *source,
                                   struct cJSON *diagnostics)
{
    if (!document_path || !source || !diagnostics)
    {
        return;
    }
    FileImports imports = {0};
    collect_imports(source, &imports);
    int module_count = imports.module_count;
    free_imports(&imports);
    if (module_count == 0)
    {
        return;
    }

    CSearchDirList dirs = {0};
    collect_search_dirs(source, document_path, &dirs);
    DirectiveCheck check = {&dirs, diagnostics, {0}};
    StringList visited = {0};
    walk_module_headers(source, document_path, NULL, 0, 0, &visited, check_module_header, &check);
    string_list_free(&visited);
    string_list_free(&check.warned);
    c_search_dirs_free(&dirs);
}
