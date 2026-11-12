// SPDX-License-Identifier: MIT
#include "lsp_project.h"
#include "lsp_config.h"
#include "json_rpc.h"
#include "lsp_main.h"
#include "../utils/string_list.h"
#include "../utils/utils.h"
#include "../constants.h"
#include "../platform/os.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

LSPProject *g_project = NULL;

/**
 * @brief Latest text of a document the client has opened.
 *
 * Lives in the libc heap, not in the arena, so it survives lsp_project_rebuild().
 */
typedef struct TrackedDocument
{
    char *uri;
    char *source;
    struct TrackedDocument *next;
} TrackedDocument;

static TrackedDocument *g_tracked_documents = NULL;
static size_t g_built_total = 0;
static int g_workspace_indexed = 0;
// The compiler the server works with, owned by zc-lsp's main(). Projects point to it.
static ZenCompiler *g_server_compiler = NULL;

// Files parsed by the current workspace indexing, for its summary.
static int g_indexed_file_count = 0;

static void scan_dir(const char *dir_path, StringList *files);
static void scan_file(const char *path);
static void load_ignore_rules(const char *root_path);

/**
 * @brief A pattern of the project's root .gitignore, which the workspace scan skips.
 */
typedef struct
{
    char *pattern;
    int directory_only; ///< Written with a trailing `/`.
    int anchored;       ///< Relative to the root rather than a name at any depth.
    const char *source; ///< Where the rule comes from, for the log.
} IgnoreRule;

// Rules of the scan in progress (only valid while lsp_project_index_workspace() runs).
static IgnoreRule *g_ignore_rules = NULL;
static int g_ignore_rule_count = 0;
static size_t g_scan_root_length = 0;

void lsp_set_compiler(ZenCompiler *compiler)
{
    g_server_compiler = compiler;
}

ZenCompiler *lsp_compiler(void)
{
    return g_server_compiler;
}

void lsp_project_init(const char *root_path)
{
    if (g_project)
    {
        return;
    }

    g_project = xcalloc(1, sizeof(LSPProject));
    g_project->root_path = xstrdup(root_path);
    // The index finds the types a file names among the ones it imports.
    lsp_index_set_visible_type(lsp_project_visible_type);
    fprintf(stderr, "zls: project root: %s\n", root_path);
    g_project->use_gitignore = 1;

    // Create a persistent global context
    g_project->ctx = xcalloc(1, sizeof(ParserContext));
    g_project->ctx->compiler = g_server_compiler;
    g_project->ctx->config = &g_server_compiler->config;
    g_project->ctx->is_fault_tolerant = 1;
    module_state_init(&g_project->ctx->imports);
    g_project->ctx->cg.hoist_out = tmpfile(); // Support hoisting in LSP
    if (!g_project->ctx->cg.hoist_out)
    {
        fprintf(stderr, "zls: Warning: Failed to create hoist_out temporary file. Hoisting will be "
                        "disabled.\n");
    }

    // Register this persistent context as the parser/token/diagnostic context.
    // Without config the lexer/parser deref ctx->config (e.g. misra_mode) and
    // segfault on the first didOpen, and without the diagnostic context a parse
    // error would fall through to exit(1) and kill the whole server.
    token_set_parser_ctx(g_project->ctx);
    diag_set_parser_ctx(g_project->ctx);

    CompilerConfig *cfg = &g_project->ctx->compiler->config;

    // Add the workspace root and its std/ to the include paths: imports resolve from the
    // workspace, and a workspace with its own std/ (the compiler's repository) uses it. They
    // are searched before cfg->root_path, which keeps the compiler's own root, set by
    // z_compiler_setup next to the executable: the standard library is found from there for a
    // workspace without one.
    zvec_push_Str(&cfg->include_paths, xstrdup(root_path));

    char std_path[MAX_PATH_LEN];
    snprintf(std_path, sizeof(std_path), "%s/std", root_path);
    zvec_push_Str(&cfg->include_paths, xstrdup(std_path));

    lsp_config_load(root_path, cfg, g_project);
}

void lsp_project_index_workspace(void)
{
    if (!g_project || !g_project->root_path)
    {
        return;
    }

    // Scan workspace
    load_ignore_rules(g_project->root_path);
    g_scan_root_length = strlen(g_project->root_path);
    double start = z_get_monotonic_time();
    g_indexed_file_count = 0;
    // The files are collected first, so that the progress shown by the client has a total.
    StringList files = {0};
    scan_dir(g_project->root_path, &files);
    lsp_progress_begin("Indexing Zen C workspace", files.count);
    for (int i = 0; i < files.count; i++)
    {
        scan_file(files.items[i]);
        lsp_progress_report(i + 1, files.count);
        if (lsp_client_gone())
        {
            fprintf(stderr, "zls: the editor exited, stopping\n");
            exit(0);
        }
    }
    string_list_free(&files);
    char summary[64];
    snprintf(summary, sizeof(summary), "Indexed %d files", g_indexed_file_count);
    lsp_progress_end(summary);
    fprintf(stderr, "zls: indexed %d files in %.0f ms\n", g_indexed_file_count,
            (z_get_monotonic_time() - start) * 1000.0);
    g_ignore_rules = NULL;
    g_ignore_rule_count = 0;

    g_workspace_indexed = 1;
    g_built_total = g_arena.total_alloc;
}

static char *libc_strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *copy = libc_malloc(len);
    if (copy)
    {
        memcpy(copy, s, len);
    }
    return copy;
}

void lsp_project_track_document(const char *uri, const char *src)
{
    char *source = libc_strdup(src);
    if (!source)
    {
        return;
    }

    TrackedDocument *document = g_tracked_documents;
    while (document && strcmp(document->uri, uri) != 0)
    {
        document = document->next;
    }

    if (!document)
    {
        document = libc_malloc(sizeof(TrackedDocument));
        char *uri_copy = libc_strdup(uri);
        if (!document || !uri_copy)
        {
            libc_free(document);
            libc_free(uri_copy);
            libc_free(source);
            return;
        }
        document->uri = uri_copy;
        document->source = NULL;
        document->next = g_tracked_documents;
        g_tracked_documents = document;
    }

    libc_free(document->source);
    document->source = source;
}

void lsp_project_untrack_document(const char *uri)
{
    for (TrackedDocument **link = &g_tracked_documents; *link; link = &(*link)->next)
    {
        TrackedDocument *document = *link;
        if (strcmp(document->uri, uri) == 0)
        {
            *link = document->next;
            libc_free(document->uri);
            libc_free(document->source);
            libc_free(document);
            return;
        }
    }
}

size_t lsp_project_built_total(void)
{
    return g_built_total;
}

void lsp_project_rebuild(ZarenaMark base, const ZenCompiler *base_compiler)
{
    if (!g_project)
    {
        return;
    }

    // Everything needed to rebuild must be copied out of the arena before rewinding it.
    char *root_path = libc_strdup(g_project->root_path ? g_project->root_path : ".");
    if (!root_path)
    {
        return;
    }
    if (g_project->ctx && g_project->ctx->cg.hoist_out)
    {
        fclose(g_project->ctx->cg.hoist_out);
    }

    zarena_restore(&g_arena, base);
#if ZARENA_ASAN_REDZONE
    // Under ASan, a stale pointer into the rewound region becomes a reported error
    // instead of silently reading whatever the rebuild writes there.
    for (zarena_block *block = base.block; block; block = block->next)
    {
        size_t start = block == base.block ? base.used : 0;
        __asan_poison_memory_region(block->data + start, block->capacity - start);
    }
#endif

    // The compiler state saved with the mark only points below it. Globals that may
    // point above it are reset.
    *g_server_compiler = *base_compiler;
    g_project = NULL;
    curr_func_ret = NULL;
    clear_registered_traits();

    lsp_project_init(root_path);
    libc_free(root_path);
    if (g_workspace_indexed)
    {
        lsp_project_index_workspace();
    }
    for (TrackedDocument *document = g_tracked_documents; document; document = document->next)
    {
        lsp_project_update_file(document->uri, document->source);
    }

    g_built_total = g_arena.total_alloc;
    fprintf(stderr, "zls: project rebuilt, arena in use: %zu KB\n", g_built_total / 1024);
}

// `*` and `?` match within a path component, as in .gitignore.
static int glob_match(const char *pattern, const char *text)
{
    while (*pattern)
    {
        if (*pattern == '*')
        {
            pattern++;
            for (;;)
            {
                if (glob_match(pattern, text))
                {
                    return 1;
                }
                if (!*text || *text == '/')
                {
                    return 0;
                }
                text++;
            }
        }
        if (!*text || (*pattern == '?' ? *text == '/' : *pattern != *text))
        {
            return 0;
        }
        pattern++;
        text++;
    }
    return !*text;
}

// Turns one line of .gitignore syntax into a rule; `line` is modified and kept by the rule.
// Supports the common subset: blank lines and `#` comments, names that match at any depth
// (`build/`, `*.dSYM/`), paths anchored to the root (`/repo`, `docs/plans/`), a trailing `/` for
// directories only, `*`, `?` and a leading `**/`. Negations (`!`) and other uses of `**` are
// ignored. Returns 0 when the line holds no usable pattern.
static int parse_ignore_rule(char *line, const char *source, IgnoreRule *rule)
{
    size_t length = strlen(line);
    while (length > 0 && isspace((unsigned char)line[length - 1]))
    {
        line[--length] = '\0';
    }

    char *pattern = line;
    *rule = (IgnoreRule){0};
    rule->source = source;
    if (length > 0 && pattern[length - 1] == '/')
    {
        rule->directory_only = 1;
        pattern[--length] = '\0';
    }
    if (strncmp(pattern, "**/", 3) == 0)
    {
        pattern += 3;
    }
    else if (pattern[0] == '/')
    {
        rule->anchored = 1;
        pattern++;
    }
    else if (strchr(pattern, '/'))
    {
        rule->anchored = 1;
    }

    if (!pattern[0] || pattern[0] == '#' || pattern[0] == '!' || strstr(pattern, "**"))
    {
        return 0;
    }
    rule->pattern = pattern;
    return 1;
}

// Rules of the scan: the root .gitignore (unless `use_gitignore` is false) and the `exclude`
// patterns of zenc.server.json.
static void load_ignore_rules(const char *root_path)
{
    g_ignore_rules = NULL;
    g_ignore_rule_count = 0;

    char *text = NULL;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/.gitignore", root_path);
    struct stat st;
    if (g_project->use_gitignore && stat(path, &st) == 0 && S_ISREG(st.st_mode))
    {
        text = load_file(path, NULL, g_project->ctx->config);
    }

    size_t capacity = 1 + g_project->exclude_patterns.length;
    for (const char *c = text; c && *c; c++)
    {
        capacity += *c == '\n';
    }
    g_ignore_rules = xcalloc(capacity, sizeof(IgnoreRule));

    char *line = text;
    while (line && *line)
    {
        char *next = strchr(line, '\n');
        if (next)
        {
            *next++ = '\0';
        }
        if (parse_ignore_rule(line, ".gitignore", &g_ignore_rules[g_ignore_rule_count]))
        {
            g_ignore_rule_count++;
        }
        line = next;
    }
    if (text)
    {
        fprintf(stderr, "zls: .gitignore: %d patterns\n", g_ignore_rule_count);
    }

    for (size_t i = 0; i < g_project->exclude_patterns.length; i++)
    {
        char *pattern = xstrdup(g_project->exclude_patterns.data[i]);
        if (parse_ignore_rule(pattern, "zenc.server.json exclude",
                              &g_ignore_rules[g_ignore_rule_count]))
        {
            g_ignore_rule_count++;
        }
    }
}

// The rule that ignores `path` (`name` is its last component), or NULL.
static const IgnoreRule *is_ignored(const char *path, const char *name, int is_directory)
{
    const char *relative = strlen(path) > g_scan_root_length ? path + g_scan_root_length + 1 : name;
    for (int i = 0; i < g_ignore_rule_count; i++)
    {
        const IgnoreRule *rule = &g_ignore_rules[i];
        if (rule->directory_only && !is_directory)
        {
            continue;
        }
        if (glob_match(rule->pattern, rule->anchored ? relative : name))
        {
            return rule;
        }
    }
    return NULL;
}

static void scan_file(const char *path)
{
    // Deduplicate indexing. A file an import indexed first (index_imported_modules) is indexed
    // again here, as a file of the workspace.
    ProjectFile *known = lsp_project_get_file_by_path(path);
    if (known && !known->is_external)
    {
        return;
    }

    char *src = load_file(path, g_project->ctx->current_filename, g_project->ctx->config);
    if (!src)
    {
        return;
    }

    lsp_project_update_file(lsp_path_to_uri(path), src);
    g_indexed_file_count++;

    // Free source after update
    zfree(src);
}

// Collects the .zc files under dir_path that the scan does not skip.
static void scan_dir(const char *dir_path, StringList *files)
{
    DIR *d = opendir(dir_path);
    if (!d)
    {
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_name[0] == '.')
        {
            continue;
        }

        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", dir_path, dir->d_name);

        struct stat st;
        if (stat(path, &st) == 0)
        {
            const IgnoreRule *ignored = is_ignored(path, dir->d_name, S_ISDIR(st.st_mode));
            if (ignored)
            {
                if (S_ISDIR(st.st_mode))
                {
                    fprintf(stderr, "zls: skipping %s (%s)\n", path, ignored->source);
                }
                continue;
            }
            const char *ext = strrchr(dir->d_name, '.');
            if (S_ISDIR(st.st_mode))
            {
                scan_dir(path, files);
            }
            else if (S_ISREG(st.st_mode) && ext && strcmp(ext, ".zc") == 0)
            {
                string_list_add(files, path);
            }
        }
    }
    closedir(d);
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    c = (char)toupper((unsigned char)c);
    return c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

char *lsp_uri_to_path(const char *uri)
{
    if (strncmp(uri, "file://", 7) != 0)
    {
        return xstrdup(uri);
    }
    const char *s = uri + 7;
    char *path = xmalloc(strlen(s) + 1);
    size_t used = 0;
    for (; *s; s++)
    {
        int high = s[0] == '%' ? hex_digit(s[1]) : -1;
        int low = high >= 0 ? hex_digit(s[2]) : -1;
        if (low >= 0)
        {
            path[used++] = (char)(high * 16 + low);
            s += 2;
        }
        else
        {
            path[used++] = *s;
        }
    }
    path[used] = 0;
    // `file:///C:/x`: that `/` belongs to the URI, not to the path.
    if (path[0] == '/' && isalpha((unsigned char)path[1]) && path[2] == ':')
    {
        memmove(path, path + 1, used);
    }
    return path;
}

char *lsp_path_to_uri(const char *path)
{
    size_t length = strlen(path);
    size_t capacity = length * 3 + sizeof("file:///");
    char *uri = xmalloc(capacity);
    size_t used = (size_t)snprintf(uri, capacity, "file://");
    if (isalpha((unsigned char)path[0]) && path[1] == ':')
    {
        uri[used++] = '/';
    }
    for (size_t i = 0; i < length; i++)
    {
        unsigned char c = (unsigned char)path[i];
        if (c == '\\')
        {
            uri[used++] = '/';
        }
        else if (isalnum(c) || strchr("/-._~:", c))
        {
            uri[used++] = (char)c;
        }
        else
        {
            static const char hex[] = "0123456789ABCDEF";
            uri[used++] = '%';
            uri[used++] = hex[c >> 4];
            uri[used++] = hex[c & 0x0F];
        }
    }
    uri[used] = 0;
    return uri;
}

ASTNode *lsp_project_top_level(const ProjectFile *pf)
{
    ASTNode *node = pf ? pf->ast : NULL;
    return node && node->kind == NODE_ROOT ? node->root.children : node;
}

ProjectFile *lsp_project_get_file(const char *uri)
{
    return uri ? lsp_project_get_file_by_path(lsp_uri_to_path(uri)) : NULL;
}

ProjectFile *lsp_project_get_file_by_path(const char *path)
{
    if (!g_project || !path)
    {
        return NULL;
    }
    for (ProjectFile *curr = g_project->files; curr; curr = curr->next)
    {
        if (curr->path && strcmp(curr->path, path) == 0)
        {
            return curr;
        }
    }
    return NULL;
}

static ProjectFile *add_project_file(const char *path)
{
    ProjectFile *f = xcalloc(1, sizeof(ProjectFile));
    f->path = xstrdup(path);
    f->uri = lsp_path_to_uri(path);

    f->next = g_project->files;
    g_project->files = f;
    return f;
}

// What the parser context had before a parse: the lists it adds to, newest first, so what the
// parse added is what comes before these.
typedef struct
{
    TypeUsage *usages;
    GenericParameterUse *generic_parameter_uses;
    GenericImplTemplate *impls;
    GenericFuncTemplate *functions;
    ASTNode *implicit_imports; ///< The std modules the parser imports by itself (std/test.zc).
} ParseMarks;

static ParseMarks parse_marks(void)
{
    ParseMarks marks = {g_project->ctx->pending_type_validations,
                        g_project->ctx->generic_parameter_uses, g_project->ctx->impl_templates,
                        g_project->ctx->func_templates, g_project->ctx->implicit_std_imports};
    return marks;
}

// Bumped whenever a file's imports are read again: the visible files kept for a file are stale.
static unsigned g_imports_generation = 0;

// Reads the files `pf` imports from its text (`import "a.zc"`, `import { f } from "a.zc"`) and
// resolves them as the parser does. Its AST does not list them all: a file another one imported
// before leaves no node where it is imported again.
static void read_imports(ProjectFile *pf)
{
    g_imports_generation++;
    pf->imports = NULL;
    pf->import_count = 0;
    if (!pf->source || !pf->path)
    {
        return;
    }
    int capacity = 0;
    Lexer l;
    lexer_init(&l, pf->source, g_project->ctx->config, pf->path);
    for (Token t = lexer_next(&l); t.kind != TOK_EOF; t = lexer_next(&l))
    {
        if (t.kind != TOK_IDENT || t.len != 6 || strncmp(t.start, "import", 6) != 0)
        {
            continue;
        }
        Token file = lexer_next(&l);
        while (file.kind != TOK_STRING && file.kind != TOK_SEMICOLON && file.kind != TOK_EOF)
        {
            file = lexer_next(&l);
        }
        if (file.kind != TOK_STRING || file.len < 2)
        {
            continue;
        }
        char *written = xmalloc(file.len - 1);
        memcpy(written, file.start + 1, file.len - 2);
        written[file.len - 2] = 0;
        char *resolved = z_resolve_path(written, pf->path, g_project->ctx->config);
        if (!resolved)
        {
            continue;
        }
        if (pf->import_count == capacity)
        {
            capacity = capacity ? capacity * 2 : 8;
            pf->imports = xrealloc(pf->imports, (size_t)capacity * sizeof(char *));
        }
        pf->imports[pf->import_count++] = resolved;
    }
}

#define VISIBLE_FILES_MAX 256
#define VISIBLE_FILES_MAX_DEPTH 32 // As deep as index_imported_modules follows imports

// The files a file sees: itself first, then what it imports, directly or through another import.
typedef struct
{
    const char *paths[VISIBLE_FILES_MAX];
    ProjectFile *files[VISIBLE_FILES_MAX]; // NULL for a file the server has not read
    int count;
} VisibleFiles;

static int is_visible(const VisibleFiles *visible, const char *path)
{
    for (int i = 0; path && i < visible->count; i++)
    {
        if (strcmp(visible->paths[i], path) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static void collect_visible_files(VisibleFiles *visible, const char *path, int depth)
{
    if (is_visible(visible, path) || visible->count == VISIBLE_FILES_MAX ||
        depth > VISIBLE_FILES_MAX_DEPTH)
    {
        return;
    }
    ProjectFile *pf = lsp_project_get_file_by_path(path);
    visible->paths[visible->count] = path;
    visible->files[visible->count++] = pf;
    for (int i = 0; pf && i < pf->import_count; i++)
    {
        collect_visible_files(visible, pf->imports[i], depth + 1);
    }
}

// The files `path` sees. They are kept while the same file asks again (its index asks once per type
// name it reads) and no file's imports change.
static const VisibleFiles *visible_files(const char *path)
{
    static VisibleFiles visible;
    static char kept_path[MAX_PATH_LEN];
    static unsigned kept_generation;
    if (visible.count && kept_generation == g_imports_generation && strcmp(kept_path, path) == 0)
    {
        return &visible;
    }
    visible.count = 0;
    collect_visible_files(&visible, path, 0);
    snprintf(kept_path, sizeof(kept_path), "%s", path);
    kept_generation = g_imports_generation;
    return &visible;
}

void lsp_project_visit_documents_seeing(const char *uri,
                                        void (*visit)(const char *uri, const char *source))
{
    ProjectFile *seen = g_project ? lsp_project_get_file(uri) : NULL;
    if (!seen)
    {
        return;
    }
    // Imports are resolved as the parser resolves them, following symbolic links (/tmp is
    // /private/tmp on macOS); the path of an open document is the one of its URI.
    const char *resolved = z_resolve_path(seen->path, NULL, g_project->ctx->config);
    for (TrackedDocument *document = g_tracked_documents; document; document = document->next)
    {
        ProjectFile *pf = lsp_project_get_file(document->uri);
        if (!pf || pf == seen)
        {
            continue;
        }
        const VisibleFiles *visible = visible_files(pf->path);
        if (is_visible(visible, seen->path) || is_visible(visible, resolved))
        {
            visit(document->uri, document->source);
        }
    }
}

// The name a top-level node declares a type with, as the parser registers it; NULL for no type.
static const char *declared_type_name(ASTNode *node)
{
    switch (node->kind)
    {
    case NODE_STRUCT:
        return node->strct.name;
    case NODE_ENUM:
        return node->enm.name;
    case NODE_TRAIT:
        return node->trait.name;
    case NODE_TYPE_ALIAS:
        return node->type_alias.alias;
    default:
        return NULL;
    }
}

// Whether `text`, written in an `import` of the file `importer`, leads to the file `target` (a
// resolved path), as the parser resolves it.
static int import_leads_to(const char *text, const char *importer, const char *target)
{
    char *resolved = z_resolve_path(text, importer, g_project->ctx->config);
    return resolved && strcmp(resolved, target) == 0;
}

int lsp_project_import_text(const char *importer, const char *target, char *out, size_t size)
{
    if (!g_project || !g_project->ctx || !importer || !target || size == 0)
    {
        return 0;
    }
    CompilerConfig *cfg = g_project->ctx->config;
    char *from = z_resolve_path(importer, NULL, cfg);
    char *to = z_resolve_path(target, NULL, cfg);
    if (!from || !to)
    {
        return 0;
    }
    const char *std_dir = z_find_std_dir(to);
    if (std_dir && import_leads_to(std_dir + 1, importer, to))
    {
        snprintf(out, size, "%s", std_dir + 1);
        return 1;
    }
    // The directories both share, then a `../` for each one left of `importer`'s.
    size_t common = 0;
    for (size_t i = 0; from[i] && from[i] == to[i]; i++)
    {
        if (z_is_path_sep(from[i]))
        {
            common = i + 1;
        }
    }
    size_t used = 0;
    out[0] = 0;
    for (const char *p = from + common; *p; p++)
    {
        if (z_is_path_sep(*p))
        {
            str_appendf(out, size, &used, "../");
        }
    }
    str_appendf(out, size, &used, "%s", to + common);
    return import_leads_to(out, importer, to);
}

ASTNode *lsp_project_visible_type(const char *path, const char *name)
{
    if (!g_project || !g_project->ctx || !path || !name)
    {
        return NULL;
    }
    const VisibleFiles *visible = visible_files(path);

    // The context registered one declaration per name and kind: when it is in a file `path` sees,
    // that is the one. It also finds the types of a file not parsed on its own yet (an import the
    // parse is still reading), which has no AST.
    ParserContext *ctx = g_project->ctx;
    GenericTemplate *template = find_generic_template(ctx, name);
    ASTNode *registered[] = {find_struct_def(ctx, name), template ? template->struct_node : NULL,
                             find_trait_def(ctx, name)};
    int any_registered = find_type_alias_node(ctx, name) != NULL;
    for (size_t r = 0; r < sizeof(registered) / sizeof(registered[0]); r++)
    {
        if (registered[r] && is_visible(visible, registered[r]->token.file))
        {
            return registered[r];
        }
        any_registered |= registered[r] != NULL;
    }
    // A name the program does not declare (a C header's) is in no file's AST.
    if (!any_registered)
    {
        return NULL;
    }
    // Another file declares a type of that name too, and the context kept that one.
    for (int i = 0; i < visible->count; i++)
    {
        ProjectFile *pf = visible->files[i];
        for (ASTNode *node = pf ? lsp_project_top_level(pf) : NULL; node; node = node->next)
        {
            const char *declared = declared_type_name(node);
            if (declared && strcmp(declared, name) == 0)
            {
                return node;
            }
        }
    }
    return NULL;
}

// The index of `pf`, a file the last parse read from `root`: its nodes, the generic impls the parse
// registered for it and the type names it read there. Its generic impls and functions are kept
// too, since they are no nodes of its AST.
static void index_parsed_file(ProjectFile *pf, ASTNode *root, ParseMarks before)
{
    // Before the index: the types its names go to are those of the files it imports.
    read_imports(pf);
    pf->index = lsp_index_new();
    lsp_build_index(g_project->ctx, pf->index, root, pf->source);

    // A generic impl (`impl Vec<T>`) is a template, not a node of the file's AST: its
    // methods are what `Vec<int>::new` goes to.
    pf->generic_impl_count = 0;
    for (GenericImplTemplate *it = g_project->ctx->impl_templates; it && it != before.impls;
         it = it->next)
    {
        pf->generic_impl_count++;
    }
    pf->generic_impls = xcalloc((size_t)pf->generic_impl_count + 1, sizeof(ASTNode *));
    pf->generic_impl_count = 0;
    for (GenericImplTemplate *it = g_project->ctx->impl_templates; it && it != before.impls;
         it = it->next)
    {
        ASTNode *impl = it->impl_node;
        if (impl && impl->token.file && strcmp(impl->token.file, pf->path) == 0)
        {
            pf->generic_impls[pf->generic_impl_count++] = impl;
            lsp_build_index(g_project->ctx, pf->index, impl, pf->source);
        }
    }

    pf->generic_function_count = 0;
    for (GenericFuncTemplate *it = g_project->ctx->func_templates; it && it != before.functions;
         it = it->next)
    {
        pf->generic_function_count++;
    }
    pf->generic_functions = xcalloc((size_t)pf->generic_function_count + 1, sizeof(ASTNode *));
    pf->generic_function_count = 0;
    for (GenericFuncTemplate *it = g_project->ctx->func_templates; it && it != before.functions;
         it = it->next)
    {
        ASTNode *fn = it->func_node;
        if (fn && fn->token.file && strcmp(fn->token.file, pf->path) == 0)
        {
            pf->generic_functions[pf->generic_function_count++] = fn;
            lsp_build_index(g_project->ctx, pf->index, fn, pf->source);
        }
    }

    for (TypeUsage *use = g_project->ctx->pending_type_validations; use && use != before.usages;
         use = use->next)
    {
        // The names the file imports were read too, in their own files.
        if (use->location.file && strcmp(use->location.file, pf->path) == 0)
        {
            lsp_index_add_type_ref(pf->index, g_project->ctx, use->location, use->name);
        }
    }

    for (GenericParameterUse *use = g_project->ctx->generic_parameter_uses;
         use && use != before.generic_parameter_uses; use = use->next)
    {
        if (!use->at.file || strcmp(use->at.file, pf->path) != 0)
        {
            continue;
        }
        if (use->at.start == use->declaration.start)
        {
            char hover[MAX_SHORT_MSG_LEN];
            snprintf(hover, sizeof(hover), "type parameter %.*s", (int)use->at.len, use->at.start);
            lsp_index_add_def(pf->index, use->at, DECLARATION_TYPE_PARAMETER, hover, NULL);
        }
        else
        {
            lsp_index_add_ref(pf->index, use->at, use->declaration, NULL);
        }
    }
}

// Whether a file of the project declares a type named `name`.
static int project_declares_type(const char *name)
{
    for (ProjectFile *pf = g_project->files; pf; pf = pf->next)
    {
        for (ASTNode *node = lsp_project_top_level(pf); node; node = node->next)
        {
            const char *declared = declared_type_name(node);
            if (declared && strcmp(declared, name) == 0)
            {
                return 1;
            }
        }
    }
    return 0;
}

// The type names the last parse, whose marks are `before`, read in `pf` and that name no type
// that `pf` sees: no type of the program, or one that only files `pf` does not import declare.
// The server parses the whole workspace with one context, which knows those too.
static TypeUsage *unknown_types_of(ProjectFile *pf, ParseMarks before)
{
    TypeUsage *unknown = NULL;
    TypeUsage **tail = &unknown;
    for (TypeUsage *use = g_project->ctx->pending_type_validations; use && use != before.usages;
         use = use->next)
    {
        if (use->location.file && strcmp(use->location.file, pf->path) == 0 &&
            !lsp_project_visible_type(pf->path, use->name) &&
            (!is_known_type_name(g_project->ctx, use->name) || project_declares_type(use->name)))
        {
            TypeUsage *copy = xmalloc(sizeof(TypeUsage));
            *copy = *use;
            copy->next = NULL;
            *tail = copy;
            tail = &copy->next;
        }
    }
    return unknown;
}

// The modules the last parse read that are no file of the project, such as the standard library
// outside the workspace: their definitions are what `Vec` and `Vec<int>::new` go to. The
// server's context parses a module once, so each is indexed from the import that read it.
static void index_imported_modules(ASTNode *nodes, ParseMarks before, int depth);

// The modules that the parse whose marks are `before` imported without an import of the program
// (the parser loads std/test.zc for a `test` block): they are no nodes of any AST.
static void index_implicit_imports(ParseMarks before)
{
    for (ASTNode *n = g_project->ctx->implicit_std_imports; n && n != before.implicit_imports;
         n = n->next)
    {
        ASTNode *next = n->next;
        n->next = NULL; // One import, not the ones before it.
        index_imported_modules(n, before, 0);
        n->next = next;
    }
}

static void index_imported_modules(ASTNode *nodes, ParseMarks before, int depth)
{
    for (ASTNode *n = nodes; n && depth < 32; n = n->next)
    {
        const char *path = n->kind == NODE_IMPORT ? n->import_stmt.path : NULL;
        if (!path || !n->import_stmt.module_root || lsp_project_get_file_by_path(path))
        {
            continue;
        }
        ProjectFile *module = add_project_file(path);
        module->is_external = 1;
        module->ast = n->import_stmt.module_root;
        // The text it was parsed from, so code the parser generates in it stays out of the index.
        module->source = load_file(path, NULL, g_project->ctx->config);
        index_parsed_file(module, n->import_stmt.module_root, before);
        index_imported_modules(n->import_stmt.module_root, before, depth + 1);
    }
}

void lsp_project_update_file(const char *uri, const char *src)
{
    if (!g_project)
    {
        return;
    }

    ProjectFile *pf = lsp_project_get_file(uri);
    if (!pf)
    {
        pf = add_project_file(lsp_uri_to_path(uri));
    }
    // Opened or in the workspace: a file of the project, even if an import read it first.
    pf->is_external = 0;

    // Use the plain path for internal compiler state.
    // This allows z_resolve_path and is_file_imported to work correctly.
    const char *saved_filename = g_project->ctx->current_filename;
    g_project->ctx->current_filename = pf->path;

    if (pf->index)
    {
        lsp_index_free(pf->index);
        pf->index = NULL;
    }

    if (pf->source)
    {
        zfree(pf->source);
    }
    pf->source = xstrdup(src);

    // The tokens point into the text they were read from: the caller's copy may be gone by
    // the next request, the file's own stays as long as what is parsed from it.
    Lexer l;
    lexer_init(&l, pf->source, g_project->ctx->config, g_project->ctx->current_filename);

    // Reset parser context globals only for fresh manual updates.
    // During workspace indexing, we want to accumulate definitions.
    // Initialize built-ins if it's the first time
    if (!g_project->ctx->global_scope)
    {
        register_builtins(g_project->ctx);
    }

    g_project->ctx->had_error = 0;

    if (!is_file_imported(g_project->ctx, pf->path))
    {
        mark_file_imported(g_project->ctx, pf->path);
    }

    // Each parse counts its own diagnostics; otherwise the counts grow with every edit.
    g_project->ctx->compiler->error_count = 0;
    g_project->ctx->compiler->warning_count = 0;

    // The parser lists every type name it reads (newest first), to validate them when it is done.
    // The server indexes the names this parse read, keeps those of this file that name no type
    // (the caller may warn about them), and drops the list with the parse.
    ParseMarks before = parse_marks();
    ASTNode *root = parse_program(g_project->ctx, &l);
    pf->unknown_types = NULL;
    if (root)
    {
        pf->ast = root;
        index_parsed_file(pf, root, before);
        pf->unknown_types = unknown_types_of(pf, before);
        index_imported_modules(root->kind == NODE_ROOT ? root->root.children : root, before, 0);
        index_implicit_imports(before);
    }
    else
    {
        pf->ast = NULL;
        pf->generic_impl_count = 0;
        pf->generic_function_count = 0;
    }
    g_project->ctx->pending_type_validations = before.usages;
    g_project->ctx->generic_parameter_uses = before.generic_parameter_uses;

    g_project->ctx->current_filename = saved_filename;
}

LSPRange *lsp_project_definition(ProjectFile *pf, LSPRange *r, ProjectFile **def_file)
{
    ProjectFile *owner = pf;
    const char *file = r->symbol.definition.file;
    if (r->kind == RANGE_REFERENCE && file && pf->path && strcmp(file, pf->path) != 0)
    {
        owner = lsp_project_get_file_by_path(file);
    }
    if (def_file)
    {
        *def_file = owner;
    }
    if (r->kind == RANGE_DEFINITION)
    {
        return r;
    }
    if (!owner || !owner->index)
    {
        return NULL;
    }
    LSPRange *def =
        lsp_find_at(owner->index, r->symbol.definition.line - 1, r->symbol.definition.col - 1);
    return (def && def->kind == RANGE_DEFINITION) ? def : NULL;
}

ReferenceResult *lsp_project_find_references(Token definition)
{
    if (!g_project)
    {
        return NULL;
    }
    ReferenceResult *head = NULL;
    ReferenceResult *tail = NULL;

    for (ProjectFile *pf = g_project->files; pf; pf = pf->next)
    {
        // A module outside the workspace is not the program's: rename must not edit it.
        if (!pf->index || pf->is_external)
        {
            continue;
        }
        for (LSPRange *r = pf->index->head; r; r = r->next)
        {
            if (!lsp_range_names(r, definition))
            {
                continue;
            }
            ReferenceResult *found = calloc(1, sizeof(ReferenceResult));
            found->uri = pf->uri;
            found->range = r;
            if (!head)
            {
                head = found;
            }
            else
            {
                tail->next = found;
            }
            tail = found;
        }
    }
    return head;
}
