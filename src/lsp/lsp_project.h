// SPDX-License-Identifier: MIT
#ifndef LSP_PROJECT_H
#ifndef ZC_ALLOW_INTERNAL
#error "lsp/lsp_project.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define LSP_PROJECT_H

#include "parser.h"
#include "lsp_index.h"
#include "lsp_protocol.h"
struct cJSON;

/**
 * @brief Represents a tracked file in the LSP project.
 */
typedef struct ProjectFile
{
    char *path;      ///< Absolute file path: what identifies the file.
    char *uri;       ///< Its file:// URI, percent-encoded, as responses give it (lsp_path_to_uri).
    char *source;    ///< Cached source content (in-memory).
    ASTNode *ast;    ///< Cached AST for semantic analysis.
    LSPIndex *index; ///< File-specific symbol index.
    /// 1 for a module only imported from outside the workspace (the standard library): go to
    /// definition and hover reach it, references, rename and workspace symbols leave it out.
    int is_external;
    /// The generic impls (`impl Vec<T>`) and functions (`fn id<T>`) its last parse declared:
    /// templates, not nodes of `ast`.
    ASTNode **generic_impls;
    int generic_impl_count;
    ASTNode **generic_functions;
    int generic_function_count;
    /// The type names its last parse read in it that name no type it sees, newest first: no type
    /// of the program, or one that only files it does not import declare. A C header may still
    /// declare them.
    TypeUsage *unknown_types;
    /// The files its text imports, resolved as the parser resolves them, in the order written.
    char **imports;
    int import_count;
    struct ProjectFile *next;
} ProjectFile;

/**
 * @brief Global state for the Language Server Project.
 */
typedef struct
{
    ParserContext *ctx;
    ProjectFile *files;        ///< List of tracked open files.
    char *root_path;           ///< Project root directory.
    zvec_Str exclude_patterns; ///< `exclude` of zenc.server.json, in .gitignore syntax.
    int use_gitignore;         ///< `use_gitignore` of zenc.server.json (default 1).
} LSPProject;

// Global project instance
extern LSPProject *g_project;

// The compiler the server works with: set once by lsp_main(), read by projects and
// the C header support.
void lsp_set_compiler(ZenCompiler *compiler);
ZenCompiler *lsp_compiler(void);

// Initialize the project with a root directory
void lsp_project_init(const char *root_path);

// Perform full project indexing
void lsp_project_index_workspace(void);

// The path of a file:// URI as a client writes it: without the scheme, percent-decoded, and
// without the `/` before a Windows drive (`file:///C:/x` is `C:/x`). Any other URI is copied as it
// is. In the arena.
char *lsp_uri_to_path(const char *uri);

// The file:// URI of a path, percent-encoded, as a client expects it. In the arena.
char *lsp_path_to_uri(const char *path);

// The first declaration of `pf` at the top level of its file, the rest linked by `next`; NULL when
// it has no AST. Declarations a parse made outside the AST, such as generic impls, are not there.
ASTNode *lsp_project_top_level(const ProjectFile *pf);

// Find a file in the project by its URI, however the client encodes it.
ProjectFile *lsp_project_get_file(const char *uri);

// Find a file in the project by its path, as tokens name it (Token.file); NULL if not there.
ProjectFile *lsp_project_get_file_by_path(const char *path);

// The declaration of the type `name` (a struct, an enum, a trait or an alias, by the name the
// parser gives it) that the file `path` sees: its own, or one of a file it imports, directly or
// through another import. The server parses the whole workspace with one context, so a type of
// that name in a file `path` does not import is not this one. NULL when no file it sees declares
// it.
ASTNode *lsp_project_visible_type(const char *path, const char *name);

// Writes to `out` what an `import` in the file `importer` writes to import the file `target`: the
// path from `std/` for a file of the standard library (`std/net/http.zc`), the path relative to
// `importer` otherwise (`../shapes.zc`). Each is checked as the parser resolves an import. Returns
// 0 when it leads to `target` neither way.
int lsp_project_import_text(const char *importer, const char *target, char *out, size_t size);

// Update a file (re-parse and re-index)
void lsp_project_update_file(const char *uri, const char *src);

// Remember the latest text of an open document (outside the arena) so a rebuild can
// re-apply it.
void lsp_project_track_document(const char *uri, const char *src);

// Forget a document the client closed: a rebuild reads it from disk again.
void lsp_project_untrack_document(const char *uri);

// Arena usage right after the last full build (workspace indexing or rebuild).
size_t lsp_project_built_total(void);

// Rewind the arena to `base`, restore the compiler state saved together with it and
// rebuild the project from disk plus the tracked documents. Every pointer into the
// arena taken after `base` is invalid afterwards.
void lsp_project_rebuild(ZarenaMark base, const ZenCompiler *base_compiler);

// The definition of the symbol that `r`, a range of `pf`, names: `r` itself if it is a definition.
// NULL when the file that declares it has no definition there (a declaration not in the index,
// such as one the parser generates). The file goes to *def_file, if given.
LSPRange *lsp_project_definition(ProjectFile *pf, LSPRange *r, ProjectFile **def_file);

typedef struct ReferenceResult
{
    char *uri;
    LSPRange *range;
    struct ReferenceResult *next;
} ReferenceResult;

// The ranges of the workspace's files that name the symbol declared at `definition` (see
// lsp_range_names): its declaration and its uses.
ReferenceResult *lsp_project_find_references(Token definition);

#endif
