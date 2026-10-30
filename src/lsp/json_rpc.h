// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "lsp/json_rpc.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef JSON_RPC_H
#define JSON_RPC_H

/**
 * @brief Handle a raw JSON-RPC request string.
 *
 * Parses the request, routes it to the appropriate handler (initialize, textDocument/didChange,
 * etc.), and sends back the response to stdout.
 *
 * @param json_str Null-terminated JSON request string.
 */
void handle_request(const char *json_str);

/// Set to 0 before calling handlers that modify persistent project state (didOpen, didChange).
/// When 1 (default), the caller (lsp_main.c) may restore the arena after the request.
extern int g_lsp_request_is_readonly;

/// The client accepts `LocationLink` results for textDocument/definition
/// (`textDocument.definition.linkSupport`), read at initialize.
extern int g_lsp_definition_link_support;

struct cJSON;

/// A response to a request: `{"jsonrpc":"2.0","id":...}`, with the id copied as the request
/// sent it (a number or a string).
struct cJSON *lsp_response_new(const struct cJSON *id);

/// Work done progress shown by the client (`$/progress`), for long work outside a request such
/// as indexing the workspace. Nothing is sent when the client does not support it. A report is
/// sent only when the percentage changes.
void lsp_progress_begin(const char *title, int total);
void lsp_progress_report(int done, int total);
void lsp_progress_end(const char *text);

#endif
