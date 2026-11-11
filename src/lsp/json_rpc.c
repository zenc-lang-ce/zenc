// SPDX-License-Identifier: MIT
#include "json_rpc.h"
#include "../utils/cJSON.h"
#include "lsp_project.h"
#include "lsp_formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Prototype

// The client shows work done progress (`window.workDoneProgress`), read at initialize.
static int g_work_done_progress = 0;
// Each progress gets its own token, which is also the id of the request that creates it.
static int g_progress_count = 0;
static char g_progress_token[32];
// The last percentage sent, so that a report goes out only when it changes.
static int g_progress_percentage = -1;

void lsp_send_message(cJSON *message)
{
    char *str = cJSON_PrintUnformatted(message);
    fprintf(stdout, "Content-Length: %zu\r\n\r\n%s", strlen(str), str);
    fflush(stdout);
    zfree(str);
    cJSON_Delete(message);
}

static void send_progress(cJSON *value)
{
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "jsonrpc", "2.0");
    cJSON_AddStringToObject(message, "method", "$/progress");
    cJSON *params = cJSON_AddObjectToObject(message, "params");
    cJSON_AddStringToObject(params, "token", g_progress_token);
    cJSON_AddItemToObject(params, "value", value);
    lsp_send_message(message);
}

static cJSON *progress_value(const char *kind, const char *text, int percentage)
{
    cJSON *value = cJSON_CreateObject();
    cJSON_AddStringToObject(value, "kind", kind);
    if (text)
    {
        cJSON_AddStringToObject(value, "message", text);
    }
    if (percentage >= 0)
    {
        cJSON_AddNumberToObject(value, "percentage", percentage);
    }
    return value;
}

void lsp_progress_begin(const char *title, int total)
{
    if (!g_work_done_progress)
    {
        return;
    }
    // The client answers the request that creates the progress; the answer has no method,
    // so lsp_handle_message ignores it.
    snprintf(g_progress_token, sizeof(g_progress_token), "zc-progress-%d", ++g_progress_count);
    cJSON *create = cJSON_CreateObject();
    cJSON_AddStringToObject(create, "jsonrpc", "2.0");
    cJSON_AddStringToObject(create, "id", g_progress_token);
    cJSON_AddStringToObject(create, "method", "window/workDoneProgress/create");
    cJSON_AddStringToObject(cJSON_AddObjectToObject(create, "params"), "token", g_progress_token);
    lsp_send_message(create);

    char text[64];
    snprintf(text, sizeof(text), "0/%d files", total);
    cJSON *value = progress_value("begin", text, 0);
    cJSON_AddStringToObject(value, "title", title);
    send_progress(value);
    g_progress_percentage = 0;
}

void lsp_progress_report(int done, int total)
{
    if (!g_work_done_progress || total <= 0)
    {
        return;
    }
    // At most one report per percent: a large workspace would flood the client otherwise.
    int percentage = (int)((long long)done * 100 / total);
    if (percentage == g_progress_percentage)
    {
        return;
    }
    g_progress_percentage = percentage;
    char text[64];
    snprintf(text, sizeof(text), "%d/%d files", done, total);
    send_progress(progress_value("report", text, percentage));
}

void lsp_progress_end(const char *text)
{
    if (!g_work_done_progress)
    {
        return;
    }
    send_progress(progress_value("end", text, -1));
}

// Decode percent-encoded URI in-place (e.g. "file%20name" -> "file name")
static void uri_decode(char *s)
{
    if (!s)
    {
        return;
    }
    char *r = s;
    while (*s)
    {
        if (*s == '%' && s[1] && s[2])
        {
            char hi = (char)toupper((unsigned char)s[1]);
            char lo = (char)toupper((unsigned char)s[2]);
            if ((hi >= '0' && hi <= '9') || (hi >= 'A' && hi <= 'F'))
            {
                int v = (hi >= 'A' ? hi - 'A' + 10 : hi - '0') * 16;
                v += (lo >= 'A' ? lo - 'A' + 10 : lo - '0');
                *r++ = (char)v;
                s += 3;
                continue;
            }
        }
        *r++ = *s++;
    }
    *r = 0;
}

// Helper to extract textDocument params
static void get_params(cJSON *root, char **uri, int *line, int *col)
{
    cJSON *params = cJSON_GetObjectItem(root, "params");

    if (!params)
    {
        return;
    }

    cJSON *doc = cJSON_GetObjectItem(params, "textDocument");
    if (doc)
    {
        cJSON *u = cJSON_GetObjectItem(doc, "uri");
        if (u && u->valuestring)
        {
            *uri = xstrdup(u->valuestring);
            uri_decode(*uri);
        }
    }

    cJSON *pos = cJSON_GetObjectItem(params, "position");
    if (pos)
    {
        cJSON *l = cJSON_GetObjectItem(pos, "line");
        cJSON *c = cJSON_GetObjectItem(pos, "character");
        if (l)
        {
            *line = l->valueint;
        }
        if (c)
        {
            *col = c->valueint;
        }
    }
}

// A response to the request `id`: `{"jsonrpc":"2.0","id":...}`, with the id copied as the request
// sent it (a number or a string).
static cJSON *lsp_response_new(const cJSON *id)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddItemToObject(response, "id", cJSON_Duplicate(id, 1));
    return response;
}

// The result of initialize: what the server can do. Reads what the client can do and opens the
// project at its root.
static cJSON *initialize(const cJSON *params)
{
    cJSON *capabilities = cJSON_GetObjectItem(params, "capabilities");
    cJSON *definition =
        cJSON_GetObjectItem(cJSON_GetObjectItem(capabilities, "textDocument"), "definition");
    g_lsp_definition_link_support = cJSON_IsTrue(cJSON_GetObjectItem(definition, "linkSupport"));
    g_work_done_progress = cJSON_IsTrue(
        cJSON_GetObjectItem(cJSON_GetObjectItem(capabilities, "window"), "workDoneProgress"));
    char *root = NULL;
    if (params)
    {
        cJSON *rp = cJSON_GetObjectItem(params, "rootPath");
        if (rp && rp->valuestring)
        {
            root = xstrdup(rp->valuestring);
            uri_decode(root);
        }
        else
        {
            cJSON *ru = cJSON_GetObjectItem(params, "rootUri");
            if (ru && ru->valuestring)
            {
                root = xstrdup(ru->valuestring);
            }
        }

        // Try workspaceFolders if rootPath/rootUri weren't set
        if (!root)
        {
            cJSON *wfs = cJSON_GetObjectItem(params, "workspaceFolders");
            if (wfs && cJSON_GetArraySize(wfs) > 0)
            {
                cJSON *wf = cJSON_GetArrayItem(wfs, 0);
                cJSON *wf_uri = cJSON_GetObjectItem(wf, "uri");
                if (wf_uri && wf_uri->valuestring)
                {
                    root = xstrdup(wf_uri->valuestring);
                }
            }
        }
    }

    if (root && strncmp(root, "file://", 7) == 0)
    {
        char *clean = xstrdup(root + 7);
        zfree(root);
        root = clean;
        uri_decode(root);
    }

    lsp_project_init(root ? root : ".");
    if (root)
    {
        zfree(root);
    }

    const char *result_text =
        "{"
        "\"serverInfo\":{\"name\":\"ZenC LS\",\"version\": \"1.0.0\"},"
        "\"capabilities\":{\"textDocumentSync\":{\"openClose\":true,\"change\":1},"
        "\"definitionProvider\":true,\"declarationProvider\":true,\"hoverProvider\":true,"
        "\"referencesProvider\":true,\"documentSymbolProvider\":true,"
        "\"workspaceSymbolProvider\":true,"
        "\"renameProvider\":true,\"codeActionProvider\":true,"
        "\"documentFormattingProvider\":true,"
        "\"signatureHelpProvider\":{\"triggerCharacters\":[\"(\"]},"
        "\"completionProvider\":{"
        "\"triggerCharacters\":[\".\"]},"
        "\"semanticTokensProvider\":{\"full\":true}"
        "}}";

    cJSON *result = cJSON_Parse(result_text);
    cJSON *semantic_tokens =
        cJSON_GetObjectItem(cJSON_GetObjectItem(result, "capabilities"), "semanticTokensProvider");
    cJSON_AddItemToObject(semantic_tokens, "legend", lsp_semantic_tokens_legend());
    return result;
}

// The result of textDocument/formatting: one edit that replaces the whole document with its
// formatted text. NULL when the server does not know the document or cannot format it.
static cJSON *formatting_edits(const char *uri)
{
    ProjectFile *pf = lsp_project_get_file(uri);
    char *formatted = pf && pf->source ? lsp_format_source(pf->source) : NULL;
    if (!formatted)
    {
        return NULL;
    }
    cJSON *result = cJSON_CreateArray();
    cJSON *edit = cJSON_CreateObject();

    // Replace whole document
    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", 0);
    cJSON_AddNumberToObject(start, "character", 0);

    // Count lines in source
    int lines = 0;
    for (const char *p = pf->source; *p; p++)
    {
        if (*p == '\n')
        {
            lines++;
        }
    }

    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", lines > 0 ? lines : 1);
    cJSON_AddNumberToObject(end, "character", 0);

    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddItemToObject(range, "end", end);
    cJSON_AddItemToObject(edit, "range", range);
    cJSON_AddStringToObject(edit, "newText", formatted);

    cJSON_AddItemToArray(result, edit);
    libc_free(formatted);
    return result;
}

void lsp_handle_message(const char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    if (!json)
    {
        return;
    }

    // A message without a method is the client's response to a request of the server
    // (window/workDoneProgress/create): nothing to answer.
    cJSON *method_item = cJSON_GetObjectItem(json, "method");
    if (!method_item || !method_item->valuestring)
    {
        cJSON_Delete(json);
        return;
    }
    char *method = method_item->valuestring;
    // A request has an id, a number or a string, which its response copies; a notification has
    // none and gets no response.
    cJSON *id_item = cJSON_GetObjectItem(json, "id");
    cJSON *params = cJSON_GetObjectItem(json, "params");

    char *uri = NULL;
    int line = 0, col = 0;
    get_params(json, &uri, &line, &col);

    cJSON *result = NULL;
    int implemented = 1;
    if (strcmp(method, "initialize") == 0)
    {
        g_lsp_request_is_readonly = 0;
        result = initialize(params);
    }
    else if (strcmp(method, "initialized") == 0)
    {
        g_lsp_request_is_readonly = 0;
        lsp_project_index_workspace();
    }
    else if (strcmp(method, "textDocument/didOpen") == 0 ||
             strcmp(method, "textDocument/didChange") == 0)
    {
        g_lsp_request_is_readonly = 0;
        cJSON *text = cJSON_GetObjectItem(cJSON_GetObjectItem(params, "textDocument"), "text");
        // For didChange, text is inside contentChanges
        if (!text && strcmp(method, "textDocument/didChange") == 0)
        {
            cJSON *changes = cJSON_GetObjectItem(params, "contentChanges");
            text = cJSON_GetObjectItem(cJSON_GetArrayItem(changes, 0), "text");
        }
        if (uri && cJSON_IsString(text))
        {
            lsp_check_file(uri, text->valuestring);
            lsp_project_track_document(uri, text->valuestring);
        }
    }
    else if (strcmp(method, "textDocument/didClose") == 0)
    {
        g_lsp_request_is_readonly = 0;
        cJSON *closed = cJSON_GetObjectItem(cJSON_GetObjectItem(params, "textDocument"), "uri");
        if (cJSON_IsString(closed))
        {
            lsp_close_file(closed->valuestring);
        }
    }
    // Declarations and definitions are not told apart: go to declaration answers like go to
    // definition.
    else if (strcmp(method, "textDocument/definition") == 0 ||
             strcmp(method, "textDocument/declaration") == 0)
    {
        result = uri ? lsp_goto_definition(uri, line, col) : NULL;
    }
    else if (strcmp(method, "textDocument/hover") == 0)
    {
        result = uri ? lsp_hover(uri, line, col) : NULL;
    }
    else if (strcmp(method, "textDocument/completion") == 0)
    {
        result = uri ? lsp_completion(uri, line, col) : NULL;
    }
    else if (strcmp(method, "textDocument/documentSymbol") == 0)
    {
        result = uri ? lsp_document_symbol(uri) : NULL;
    }
    else if (strcmp(method, "workspace/symbol") == 0)
    {
        cJSON *query = cJSON_GetObjectItem(params, "query");
        result = lsp_workspace_symbol(cJSON_IsString(query) ? query->valuestring : "");
    }
    else if (strcmp(method, "textDocument/references") == 0)
    {
        result = uri ? lsp_references(uri, line, col) : NULL;
    }
    else if (strcmp(method, "textDocument/signatureHelp") == 0)
    {
        result = uri ? lsp_signature_help(uri, line, col) : NULL;
    }
    else if (strcmp(method, "textDocument/codeAction") == 0)
    {
        cJSON *diagnostics =
            cJSON_GetObjectItem(cJSON_GetObjectItem(params, "context"), "diagnostics");
        result = uri && diagnostics ? lsp_code_action(uri, diagnostics) : NULL;
    }
    else if (strcmp(method, "textDocument/semanticTokens/full") == 0)
    {
        result = uri ? lsp_semantic_tokens_full(uri) : NULL;
    }
    else if (strcmp(method, "textDocument/rename") == 0)
    {
        cJSON *new_name = cJSON_GetObjectItem(params, "newName");
        result = uri && cJSON_IsString(new_name) ? lsp_rename(uri, line, col, new_name->valuestring)
                                                 : NULL;
    }
    else if (strcmp(method, "textDocument/formatting") == 0)
    {
        result = uri ? formatting_edits(uri) : NULL;
    }
    else if (strcmp(method, "shutdown") == 0)
    {
        // Answered with `null`.
    }
    else if (strcmp(method, "exit") == 0)
    {
        // For notification, no ID. Just exit.
        exit(0);
    }
    else
    {
        implemented = 0;
    }

    if (id_item)
    {
        cJSON *response = lsp_response_new(id_item);
        if (implemented)
        {
            cJSON_AddItemToObject(response, "result", result ? result : cJSON_CreateNull());
        }
        else
        {
            // A request the server does not implement still needs an answer, or the client
            // waits for it forever. Unknown notifications (no id) are ignored.
            cJSON *error = cJSON_CreateObject();
            cJSON_AddNumberToObject(error, "code", -32601); // MethodNotFound
            char message[256];
            snprintf(message, sizeof(message), "Method not found: %s", method);
            cJSON_AddStringToObject(error, "message", message);
            cJSON_AddItemToObject(response, "error", error);
        }
        lsp_send_message(response);
    }
    else
    {
        cJSON_Delete(result);
    }

    zfree(uri);
    cJSON_Delete(json);
}
