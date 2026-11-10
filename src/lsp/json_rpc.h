// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "lsp/json_rpc.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef JSON_RPC_H
#define JSON_RPC_H

/**
 * @brief Handles a JSON-RPC message from the client, a request or a notification.
 *
 * Parses the message and routes it to its handler (initialize, textDocument/didChange, etc.). A
 * request, which has an id, gets exactly one response, sent from here: the result its handler
 * returns, `null` when it returns none, or an error for a method the server does not implement.
 *
 * @param json_str Null-terminated JSON message string.
 */
void lsp_handle_message(const char *json_str);

/// Set to 0 before calling handlers that modify persistent project state (didOpen, didChange).
/// When 1 (default), the caller (lsp_main.c) may restore the arena after the request.
extern int g_lsp_request_is_readonly;

/// The client accepts `LocationLink` results for textDocument/definition
/// (`textDocument.definition.linkSupport`), read at initialize.
extern int g_lsp_definition_link_support;

struct cJSON;

/// Writes a message to the client and deletes it: the only writer of the server's output. A
/// response is sent by lsp_handle_message; the rest of the server sends notifications.
void lsp_send_message(struct cJSON *message);

/// Work done progress shown by the client (`$/progress`), for long work outside a request such
/// as indexing the workspace. Nothing is sent when the client does not support it. A report is
/// sent only when the percentage changes.
void lsp_progress_begin(const char *title, int total);
void lsp_progress_report(int done, int total);
void lsp_progress_end(const char *text);

#endif
