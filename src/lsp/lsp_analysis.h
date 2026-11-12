// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "lsp/lsp_analysis.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef LSP_ANALYSIS_H
#define LSP_ANALYSIS_H

#include "diagnostics/diagnostics.h"

struct cJSON;

// The diagnostics of the open documents: collected while a document is checked and published.
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
// The code actions at (line, col): writing the methods that an impl of a trait lacks.
struct cJSON *lsp_code_actions(const char *uri, int line, int col);

#endif
