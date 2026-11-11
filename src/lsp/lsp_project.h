// SPDX-License-Identifier: MIT
#ifndef LSP_PROJECT_H
#ifndef ZC_ALLOW_INTERNAL
#error "lsp/lsp_project.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define LSP_PROJECT_H

#include "parser.h"
#include "lsp_index.h"
struct cJSON;

/**
 * @brief Represents a tracked file in the LSP project.
 */
typedef struct ProjectFile
{
    char *path;      ///< Absolute file path.
    char *uri;       ///< file:// URI.
    char *source;    ///< Cached source content (in-memory).
    ASTNode *ast;    ///< Cached AST for semantic analysis.
    LSPIndex *index; ///< File-specific symbol index.
    /// 1 for a module only imported from outside the workspace (the standard library): go to
    /// definition and hover reach it, references, rename and workspace symbols leave it out.
    int is_external;
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

// Find a file in the project
ProjectFile *lsp_project_get_file(const char *uri);

// Find a file in the project by its path, as tokens name it (Token.file); NULL if not there.
ProjectFile *lsp_project_get_file_by_path(const char *path);

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

// Semantic Tokens: the result of textDocument/semanticTokens/full.
struct cJSON *lsp_semantic_tokens_full(const char *uri);

/** Severity of a published diagnostic, as the LSP specification numbers it. */
typedef enum
{
    LSP_SEVERITY_ERROR = 1,
    LSP_SEVERITY_WARNING = 2,
    LSP_SEVERITY_INFORMATION = 3,
    LSP_SEVERITY_HINT = 4
} LspDiagnosticSeverity;

// LSP analysis functions (declared here for cross-file visibility)
void lsp_diagnose_consumer(void *data, const ZenDiagnostic *d);
void lsp_check_file(const char *uri, const char *src);
void lsp_close_file(const char *uri);

// The handlers of the requests: each returns the `result` of its response, which
// lsp_handle_message sends. NULL answers `null`.
struct cJSON *lsp_goto_definition(const char *uri, int line, int col);
struct cJSON *lsp_hover(const char *uri, int line, int col);
struct cJSON *lsp_completion(const char *uri, int line, int col);
struct cJSON *lsp_document_symbol(const char *uri);
struct cJSON *lsp_workspace_symbol(const char *query);
struct cJSON *lsp_references(const char *uri, int line, int col);
struct cJSON *lsp_signature_help(const char *uri, int line, int col);
struct cJSON *lsp_rename(const char *uri, int line, int col, const char *new_name);
struct cJSON *lsp_code_action(const char *uri, struct cJSON *diagnostics);

#endif
