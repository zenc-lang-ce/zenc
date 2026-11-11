// SPDX-License-Identifier: MIT
#include "../utils/cJSON.h"
#include "../constants.h"
#include "../ast/primitives.h"
#include "../utils/utils.h"
#include "lsp_project.h" // Includes lsp_index.h, parser.h
#include "lsp_c_headers.h"
#include "lsp_locals.h"
#include "json_rpc.h"
#include "../plugins/plugin_manager.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct Diagnostic
{
    int line;
    int col;
    int len;
    int severity;
    int id;
    char *message;
    struct Diagnostic *next;
} Diagnostic;

typedef struct
{
    Diagnostic *head;
    Diagnostic *tail;
} DiagnosticList;

typedef enum
{
    CTX_GLOBAL,
    CTX_FUNCTION,
    CTX_LOOP,
    CTX_AFTER_IF,
    CTX_ASSIGNMENT
} LSPContext;

// Appends a diagnostic to `data`, a DiagnosticList.
static void diagnostic_list_add(void *data, Token t, int severity, const char *msg, int diag_id)
{
    DiagnosticList *list = (DiagnosticList *)data;
    Diagnostic *d = calloc(1, sizeof(Diagnostic));
    d->line = t.line > 0 ? t.line - 1 : 0;
    d->col = t.col > 0 ? t.col - 1 : 0;
    d->len = (int)(t.len);
    d->severity = severity;
    d->id = diag_id;
    d->message = msg ? xstrdup(msg) : xstrdup("Unknown error");
    d->next = NULL;

    if (!list->head)
    {
        list->head = d;
        list->tail = d;
    }
    else
    {
        list->tail->next = d;
        list->tail = d;
    }
}

// Consumer of the server's diagnostics. `data` is the DiagnosticList of the document
// being checked, or NULL when nobody collects them (e.g. while indexing).
void lsp_diagnose_consumer(void *data, const ZenDiagnostic *d)
{
    if (!data)
    {
        return;
    }
    char hints[MAX_PATH_LEN];
    str_join(hints, sizeof(hints), d->hints, "\n");
    char msg[MAX_ERROR_MSG_LEN + sizeof(hints) + 32]; // message, hints and the text around
    if (hints[0])
    {
        snprintf(msg, sizeof(msg), "%s (Suggestion: %s)", d->message, hints);
    }
    else
    {
        snprintf(msg, sizeof(msg), "%s", d->message);
    }
    diagnostic_list_add(data, d->token, d->is_error ? LSP_SEVERITY_ERROR : LSP_SEVERITY_WARNING,
                        msg, d->id);
}

void lsp_close_file(const char *uri)
{
    // The client's unsaved changes are gone: back to the file on disk. A file deleted
    // meanwhile keeps its last contents.
    lsp_project_untrack_document(uri);
    ProjectFile *pf = lsp_project_get_file(uri);
    char *src = pf ? load_file(pf->path, NULL, g_project->ctx->config) : NULL;
    if (src)
    {
        lsp_project_update_file(uri, src);
        zfree(src);
    }

    // Diagnostics are only computed for open documents, so a closed one's would go stale.
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "jsonrpc", "2.0");
    cJSON_AddStringToObject(root, "method", "textDocument/publishDiagnostics");
    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "uri", uri);
    cJSON_AddItemToObject(params, "diagnostics", cJSON_CreateArray());
    cJSON_AddItemToObject(root, "params", params);
    lsp_send_message(root);
}

void lsp_check_file(const char *uri, const char *json_src)
{
    if (!g_project)
    {
        const char *root_path = lsp_compiler()->config.root_path;
        if (root_path)
        {
            lsp_project_init(root_path);
        }
        else
        {
            char cwd[MAX_PATH_LEN];
            if (getcwd(cwd, sizeof(cwd)))
            {
                lsp_project_init(cwd);
            }
            else
            {
                lsp_project_init(".");
            }
        }
    }

    // Setup error capture on the global project context
    DiagnosticList diagnostics = {0};

    // The consumer collects the diagnostics of this document while it is parsed.
    void *old_consumer_data = lsp_compiler()->diag_consumer_data;
    lsp_compiler()->diag_consumer_data = &diagnostics;

    // Update and Parse
    lsp_project_update_file(uri, json_src);

    // Restore
    lsp_compiler()->diag_consumer_data = old_consumer_data;

    // Construct JSON Response (publishDiagnostics)
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "jsonrpc", "2.0");
    cJSON_AddStringToObject(root, "method", "textDocument/publishDiagnostics");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "uri", uri);

    cJSON *diag_array = cJSON_CreateArray();

    Diagnostic *d = diagnostics.head;
    while (d)
    {
        cJSON *diag = cJSON_CreateObject();

        cJSON *range = cJSON_CreateObject();
        cJSON *start = cJSON_CreateObject();
        cJSON_AddNumberToObject(start, "line", d->line);
        cJSON_AddNumberToObject(start, "character", d->col);

        cJSON *end = cJSON_CreateObject();
        cJSON_AddNumberToObject(end, "line", d->line);
        cJSON_AddNumberToObject(end, "character", d->col + d->len);

        cJSON_AddItemToObject(range, "start", start);
        cJSON_AddItemToObject(range, "end", end);

        cJSON_AddItemToObject(diag, "range", range);
        cJSON_AddNumberToObject(diag, "severity", d->severity);
        cJSON_AddStringToObject(diag, "message", d->message);
        if (d->id != DIAG_NONE)
        {
            char code_str[32];
            snprintf(code_str, sizeof(code_str), "W%d", d->id);
            cJSON_AddStringToObject(diag, "code", code_str);
        }

        cJSON_AddItemToArray(diag_array, diag);

        d = d->next;
    }

    const char *document_path = strncmp(uri, "file://", 7) == 0 ? uri + 7 : uri;
    lsp_c_headers_add_diagnostics(document_path, json_src, diag_array);

    cJSON_AddItemToObject(params, "diagnostics", diag_array);
    cJSON_AddItemToObject(root, "params", params);

    lsp_send_message(root);

    Diagnostic *cur = diagnostics.head;
    while (cur)
    {
        Diagnostic *next = cur->next;
        zfree(cur->message);
        zfree(cur);
        cur = next;
    }
}

cJSON *lsp_goto_definition(const char *uri, int line, int col)
{
    ProjectFile *pf = lsp_project_get_file(uri);
    LSPIndex *idx = pf ? pf->index : NULL;

    if (!idx)
    {
        return NULL;
    }

    LSPRange *r = lsp_find_at(idx, line, col);
    const char *target_uri = uri;
    int target_start_line = 0, target_start_col = 0;
    int target_end_line = 0, target_end_col = 0;
    int found = 0;

    // On a C header alias or name, and on the name of an `extern fn`, the C side is what
    // matters, even where Zen C has a definition of its own.
    CHeaderOrigin c_origin;
    CHeaderOriginKind c_origin_kind = C_HEADER_NONE;
    const CHeaderSymbol *c_symbols[8];
    int c_symbol_count = 0;
    if (pf->source)
    {
        c_origin_kind = lsp_c_headers_origin_at(pf->path, pf->source, line, col, &c_origin);
        if (c_origin_kind == C_HEADER_NONE && lsp_c_headers_is_extern_name(pf->source, line, col))
        {
            c_symbol_count =
                lsp_c_headers_find_at(pf->path, pf->source, line, col, c_symbols, 8, NULL);
        }
        if (c_origin_kind != C_HEADER_NONE || c_symbol_count > 0)
        {
            r = NULL;
        }
    }

    ProjectFile *def_file = NULL;
    LSPRange *def = r ? lsp_project_definition(pf, r, &def_file) : NULL;
    if (def)
    {
        target_uri = def_file->uri;
        target_start_line = def->start_line;
        target_start_col = def->start_col;
        target_end_line = def->end_line;
        target_end_col = def->end_col;
        found = 1;
    }

    if (!found && c_origin_kind == C_HEADER_NONE && c_symbol_count == 0 && pf->source)
    {
        c_symbol_count = lsp_c_headers_find_at(pf->path, pf->source, line, col, c_symbols, 8, NULL);
    }
    cJSON *c_location =
        c_origin_kind != C_HEADER_NONE
            ? lsp_c_headers_origin_location(&c_origin, c_origin_kind, g_lsp_definition_link_support)
            : NULL;

    if (c_location)
    {
        return c_location;
    }
    if (c_symbol_count > 0)
    {
        return lsp_c_headers_locations(c_symbols, c_symbol_count);
    }
    if (!found)
    {
        return NULL;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "uri", target_uri);

    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", target_start_line);
    cJSON_AddNumberToObject(start, "character", target_start_col);

    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", target_end_line);
    cJSON_AddNumberToObject(end, "character", target_end_col);

    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddItemToObject(range, "end", end);
    cJSON_AddItemToObject(result, "range", range);
    return result;
}

static const char *get_primitive_doc(const char *word)
{
    if (strcmp(word, "int") == 0)
    {
        return "**int**: 32-bit signed integer (guaranteed size).";
    }
    if (strcmp(word, "uint") == 0)
    {
        return "**uint**: 32-bit unsigned integer (guaranteed size).";
    }
    if (strcmp(word, "i8") == 0)
    {
        return "**i8**: 8-bit signed integer.";
    }
    if (strcmp(word, "u8") == 0)
    {
        return "**u8**: 8-bit unsigned integer (byte).";
    }
    if (strcmp(word, "i16") == 0)
    {
        return "**i16**: 16-bit signed integer.";
    }
    if (strcmp(word, "u16") == 0)
    {
        return "**u16**: 16-bit unsigned integer.";
    }
    if (strcmp(word, "i32") == 0)
    {
        return "**i32**: 32-bit signed integer.";
    }
    if (strcmp(word, "u32") == 0)
    {
        return "**u32**: 32-bit unsigned integer.";
    }
    if (strcmp(word, "i64") == 0)
    {
        return "**i64**: 64-bit signed integer.";
    }
    if (strcmp(word, "u64") == 0)
    {
        return "**u64**: 64-bit unsigned integer.";
    }
    if (strcmp(word, "f32") == 0)
    {
        return "**f32**: 32-bit floating point number.";
    }
    if (strcmp(word, "f64") == 0)
    {
        return "**f64**: 64-bit floating point number.";
    }
    if (strcmp(word, "bool") == 0)
    {
        return "**bool**: Boolean type representing `true` or `false`.";
    }
    if (strcmp(word, "rune") == 0)
    {
        return "**rune**: Unicode scalar value (32-bit UTF-32 code point).";
    }
    if (strcmp(word, "string") == 0)
    {
        return "**string**: Immutable sequence of UTF-8 characters.";
    }
    if (strcmp(word, "void") == 0)
    {
        return "**void**: The empty type.";
    }
    if (strcmp(word, "usize") == 0)
    {
        return "**usize**: Unsigned integer type of the same size as a pointer.";
    }
    if (strcmp(word, "isize") == 0)
    {
        return "**isize**: Signed integer type of the same size as a pointer.";
    }
    if (strcmp(word, "U0") == 0 || strcmp(word, "u0") == 0)
    {
        return "**u0**: Zero-sized unit type.";
    }
    if (strcmp(word, "c_int") == 0)
    {
        return "**c_int**: C-compatible `int` (size varies by platform).";
    }
    if (strcmp(word, "c_char") == 0)
    {
        return "**c_char**: C-compatible `char` (size varies by platform).";
    }
    if (strcmp(word, "c_long") == 0)
    {
        return "**c_long**: C-compatible `long` (size varies by platform).";
    }
    if (strcmp(word, "c_ulong") == 0)
    {
        return "**c_ulong**: C-compatible `unsigned long` (size varies by platform).";
    }
    if (strcmp(word, "c_longlong") == 0)
    {
        return "**c_longlong**: C-compatible `long long`.";
    }
    if (strcmp(word, "c_ulonglong") == 0)
    {
        return "**c_ulonglong**: C-compatible `unsigned long long`.";
    }
    return NULL;
}

static char *get_word_at(const char *src, int line, int col)
{
    if (!src)
    {
        return NULL;
    }
    int curr_line = 0;
    const char *p = src;
    while (curr_line < line && *p)
    {
        if (*p == '\n')
        {
            curr_line++;
        }
        p++;
    }
    if (curr_line != line)
    {
        return NULL;
    }
    p += col;

    // Boundary checks: move start to beginning of word
    const char *start = p;
    while (start > src && (isalnum(*(start - 1)) || *(start - 1) == '_'))
    {
        start--;
    }
    // move end to end of word
    const char *end = p;
    while (*end && (isalnum(*end) || *end == '_'))
    {
        end++;
    }

    if (start == end)
    {
        return NULL;
    }
    size_t len = (size_t)(end - start);
    char *word = malloc(len + 1);
    memcpy(word, start, (size_t)(len));
    word[len] = 0;
    return word;
}

cJSON *lsp_hover(const char *uri, int line, int col)
{
    ProjectFile *pf = lsp_project_get_file(uri);
    LSPIndex *idx = pf ? pf->index : NULL;

    if (!idx)
    {
        return NULL;
    }

    LSPRange *r = lsp_find_at(idx, line, col);
    const char *text = NULL;
    int is_primitive = 0;
    char *c_hover = NULL;
    cJSON *c_hover_range = NULL;

    // On a C header alias or name, and on the name of an `extern fn`, show the C side.
    if (pf->source)
    {
        CHeaderOrigin c_origin;
        CHeaderOriginKind c_origin_kind =
            lsp_c_headers_origin_at(pf->path, pf->source, line, col, &c_origin);
        if (c_origin_kind != C_HEADER_NONE)
        {
            c_hover = lsp_c_headers_origin_hover(&c_origin, c_origin_kind);
            c_hover_range = lsp_c_headers_origin_range(&c_origin, c_origin_kind);
        }
        else if (lsp_c_headers_is_extern_name(pf->source, line, col))
        {
            const CHeaderSymbol *c_symbol = NULL;
            if (lsp_c_headers_find_at(pf->path, pf->source, line, col, &c_symbol, 1, NULL) > 0)
            {
                c_hover = lsp_c_headers_hover(c_symbol, NULL);
            }
        }
        if (c_hover)
        {
            text = c_hover;
            r = NULL;
        }
    }

    if (r)
    {
        LSPRange *def = lsp_project_definition(pf, r, NULL);
        if (def)
        {
            text = def->hover_text;
        }

        // Plugin-Specific Hover Support
        if (!text && r->node && r->node->kind == NODE_PLUGIN)
        {
            ZPlugin *plugin = zptr_find_plugin(r->node->plugin_stmt.plugin_name);
            if (plugin && plugin->hover_fn)
            {
                // Calculate local offset inside the plugin block.
                // LSP line/col are 0-indexed.
                // node->plugin_stmt.start_line/col are 1-indexed.
                int local_line = (line + 1) - r->node->plugin_stmt.start_line;
                int local_col = 0;

                if (local_line == 0)
                {
                    // Same line as the opening brace: subtract the column offset of the block
                    // start.
                    local_col = col - (r->node->plugin_stmt.start_col - 1);
                }
                else
                {
                    // For subsequent lines, the column is relative to the start of the line.
                    local_col = col;
                }

                if (local_col >= 0)
                {
                    char *plugin_text =
                        (char *)plugin->hover_fn(r->node->plugin_stmt.body, local_line, local_col);
                    if (plugin_text)
                    {
                        text = plugin_text;
                    }
                }
            }
        }
    }

    if (!text && pf && pf->source)
    {
        char *word = get_word_at(pf->source, line, col);
        if (word)
        {
            const char *doc = get_primitive_doc(word);
            if (doc)
            {
                text = doc;
                is_primitive = 1;
            }
            zfree(word);
        }
    }

    if (!text && pf && pf->source)
    {
        const CHeaderSymbol *c_symbol = NULL;
        CHeaderOrigin via;
        if (lsp_c_headers_find_at(pf->path, pf->source, line, col, &c_symbol, 1, &via) > 0)
        {
            c_hover = lsp_c_headers_hover(c_symbol, &via);
            text = c_hover;
        }
    }

    cJSON *result = NULL;
    if (text)
    {
        result = cJSON_CreateObject();
        cJSON *contents = cJSON_CreateObject();
        cJSON_AddStringToObject(contents, "kind", "markdown");

        if (is_primitive || c_hover)
        {
            cJSON_AddStringToObject(contents, "value", text);
        }
        else
        {
            // Need to wrap in ```zc code block (using zc for highlighting)
            size_t block_size = strlen(text) + 16;
            char *code_block = malloc(block_size);
            snprintf(code_block, block_size, "```zc\n%s\n```", text);
            cJSON_AddStringToObject(contents, "value", code_block);
            zfree(code_block);
        }

        cJSON_AddItemToObject(result, "contents", contents);
        if (c_hover_range)
        {
            cJSON_AddItemToObject(result, "range", c_hover_range);
            c_hover_range = NULL;
        }
    }

    libc_free(c_hover);
    cJSON_Delete(c_hover_range);
    return result;
}

// Helper to resolve type of local var or arg
static Type *resolve_local_type(ASTNode *func, const char *name)
{
    if (!func)
    {
        return NULL;
    }
    // Check args
    if (func->func.param_names && func->func.arg_types)
    {
        for (int i = 0; i < func->func.count; i++)
        {
            if (strcmp(func->func.param_names[i], name) == 0)
            {
                return func->func.arg_types[i];
            }
        }
    }
    // Check body vars
    ASTNode *decl = lsp_find_local_decl(func, name);
    if (decl && decl->kind == NODE_VAR_DECL)
    {
        return decl->var_decl.type_info;
    }
    return NULL;
}

// Helper to determine completion context
static LSPContext lsp_get_completion_context(const char *source, int line, int col,
                                             ASTNode *target_func)
{
    // Derive ptr to the start of the current line
    int cur_l = 0;
    const char *line_ptr = source;
    while (*line_ptr && cur_l < line)
    {
        if (*line_ptr == '\n')
        {
            cur_l++;
        }
        line_ptr++;
    }

    // Scan backward from cursor for assignment or recent block close
    int i = col - 1;
    int line_len = 0;
    while (line_ptr[line_len] && line_ptr[line_len] != '\n' && line_ptr[line_len] != '\r')
    {
        line_len++;
    }

    if (i >= line_len)
    {
        i = line_len - 1;
    }

    while (i >= 0 && (line_ptr[i] == ' ' || line_ptr[i] == '\t'))
    {
        i--;
    }

    if (i >= 0 && line_ptr[i] == '=')
    {
        return CTX_ASSIGNMENT;
    }
    if (i >= 0 && line_ptr[i] == '}')
    {
        return CTX_AFTER_IF; // Heuristic for else/else if
    }

    // Scan for balanced braces to see if we are still inside a block
    int depth = 0;
    const char *s = source;
    cur_l = 0;
    int cur_c = 0;
    while (*s && (cur_l < line || (cur_l == line && cur_c < col)))
    {
        if (*s == '{')
        {
            depth++;
        }
        else if (*s == '}')
        {
            depth--;
        }

        if (*s == '\n')
        {
            cur_l++;
            cur_c = 0;
        }
        else
        {
            cur_c++;
        }
        s++;
    }

    if (depth <= 0)
    {
        return CTX_GLOBAL;
    }
    if (!target_func)
    {
        return CTX_GLOBAL;
    }

    // AST-based check for loops
    if (lsp_has_loop_before(target_func, line))
    {
        return CTX_LOOP;
    }
    return CTX_FUNCTION;
}

static int has_completion(cJSON *items, const char *label)
{
    for (int i = 0; i < cJSON_GetArraySize(items); i++)
    {
        cJSON *item_label = cJSON_GetObjectItem(cJSON_GetArrayItem(items, i), "label");
        if (cJSON_IsString(item_label) && strcmp(item_label->valuestring, label) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// Whether `name` is offered as written in Zen C: what the program declares, by the name written
// where it is declared (`t`), not the C name the compiler built for a method or a module member
// (`Point__len2`, `util__describe`); nor an internal name starting with `__`, reserved in C.
// What the compiler made without a declaration (`va_list`, the instance `Vec__int32_t`) is
// offered unless its name has `__`.
static int offered_by_name(Token t, const char *name)
{
    if (strncmp(name, "__", 2) == 0)
    {
        return 0;
    }
    if (!t.start)
    {
        return strstr(name, "__") == NULL;
    }
    return t.len == strlen(name) && strncmp(t.start, name, t.len) == 0;
}

// Adds the functions of the impl blocks of `type_name` under the name they are called by: the
// methods that take `self` when `with_self`, the other ones otherwise. A generic, written
// (`Vec<int>`) or instantiated (`Vec__int32_t`), takes those of its template (`impl Vec<T>`).
// Returns how many were added.
static int add_impl_completions(cJSON *items, const char *type_name, int with_self)
{
    size_t base_len = strcspn(type_name, "<");
    Instantiation *inst = find_instantiation(g_project->ctx, type_name);
    int added = 0;
    StructRef *ref = g_project->ctx->parsed_impls_list;
    GenericImplTemplate *tmpl = g_project->ctx->impl_templates;
    while (ref || tmpl)
    {
        int from_template = !ref;
        ASTNode *impl = ref ? ref->node : tmpl->impl_node;
        if (ref)
        {
            ref = ref->next;
        }
        else
        {
            tmpl = tmpl->next;
        }
        if (!impl)
        {
            continue;
        }
        const char *target = impl->kind == NODE_IMPL ? type_impl_name(impl->impl.target)
                             : impl->kind == NODE_IMPL_TRAIT
                                 ? type_impl_name(impl->impl_trait.target)
                                 : NULL;
        size_t target_len = target ? strlen(target) : 0;
        int same = target && target_len == base_len && strncmp(target, type_name, base_len) == 0;
        int instance = target && from_template && inst && strcmp(inst->template_name, target) == 0;
        if (!same && !instance)
        {
            continue;
        }
        ASTNode *method = impl->kind == NODE_IMPL ? impl->impl.methods : impl->impl_trait.methods;
        while (method)
        {
            int takes_self = method->kind == NODE_FUNCTION && method->func.count > 0 &&
                             method->func.param_names && method->func.param_names[0] &&
                             strcmp(method->func.param_names[0], "self") == 0;
            if (method->kind == NODE_FUNCTION && takes_self == with_self && method->token.start)
            {
                // The declaration's name token: `func.name` is the C name (`Point__len2`).
                char *name = token_strdup(method->token);
                // The same impl block can be listed more than once, and a trait can repeat a name.
                if (has_completion(items, name))
                {
                    method = method->next;
                    continue;
                }
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "label", name);
                cJSON_AddNumberToObject(
                    item, "kind", with_self ? LSP_COMPLETION_METHOD : LSP_COMPLETION_FUNCTION);
                char detail[MAX_SHORT_MSG_LEN];
                snprintf(detail, sizeof(detail), "fn %s::%s", target, name);
                cJSON_AddStringToObject(item, "detail", detail);
                char snippet[MAX_VAR_NAME_LEN];
                snprintf(snippet, sizeof(snippet), "%s(%s)", name,
                         method->func.count > with_self ? "$1" : "");
                cJSON_AddStringToObject(item, "insertText", snippet);
                cJSON_AddNumberToObject(item, "insertTextFormat", LSP_INSERT_SNIPPET);
                cJSON_AddItemToArray(items, item);
                added++;
            }
            method = method->next;
        }
    }
    return added;
}

// Adds the completion item of a local, `decl`, to the array `data`.
static void add_local_completion(ASTNode *decl, void *data)
{
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "label", decl->var_decl.name);
    cJSON_AddNumberToObject(
        item, "kind", decl->kind == NODE_CONST ? LSP_COMPLETION_CONSTANT : LSP_COMPLETION_VARIABLE);
    cJSON_AddStringToObject(item, "sortText", "002"); // Local priority
    cJSON_AddItemToArray((cJSON *)data, item);
}

cJSON *lsp_completion(const char *uri, int line, int col)
{
    ProjectFile *pf = lsp_project_get_file(uri);
    if (!g_project || !g_project->ctx || !pf)
    {
        return NULL;
    }

    // After the alias of a C header (`rl::`), the declarations of that header.
    cJSON *c_items = pf->source ? lsp_c_headers_completion(pf->path, pf->source, line, col) : NULL;
    if (c_items)
    {
        return c_items;
    }

    cJSON *items = cJSON_CreateArray();

    ASTNode *target_func = NULL;
    int best_line = -1;
    if (pf->index)
    {
        LSPRange *r = pf->index->head;
        while (r)
        {
            if (r->kind == RANGE_DEFINITION && r->symbol.kind == DECLARATION_FUNCTION && r->node)
            {
                if (r->start_line <= line && r->start_line > best_line)
                {
                    // Basic check: is the cursor AFTER the function start?
                    // With depth check in lsp_get_completion_context, this is safer.
                    best_line = r->start_line;
                    target_func = r->node;
                }
            }
            r = r->next;
        }
    }

    LSPContext context = lsp_get_completion_context(pf->source, line, col, target_func);
    if (context == CTX_GLOBAL)
    {
        target_func = NULL; // Reset if we are top-level
    }

    // Extract the prefix being typed (word before cursor on current line)
    char prefix[MAX_VAR_NAME_LEN] = "";
    int prefix_len = 0;
    char *ptr = pf->source ? pf->source : NULL;
    int line_len = 0;

    if (pf->source)
    {
        int cur_line = 0;
        ptr = pf->source;
        while (*ptr && cur_line < line)
        {
            if (*ptr == '\n')
            {
                cur_line++;
            }
            ptr++;
        }

        line_len = 0;
        while (ptr[line_len] && ptr[line_len] != '\n' && ptr[line_len] != '\r')
        {
            line_len++;
        }

        // Walk backward from cursor to find the start of the current word
        if (col > 0 && col <= line_len)
        {
            int i = col - 1;
            while (i >= 0 && (isalnum((unsigned char)ptr[i]) || ptr[i] == '_'))
            {
                i--;
            }
            prefix_len = col - 1 - i;
            if (prefix_len > 0 && prefix_len < (int)sizeof(prefix))
            {
                strncpy(prefix, ptr + i + 1, (size_t)prefix_len);
                prefix[prefix_len] = 0;
            }
        }
    }

    int dot_completed = 0;

    // A member being typed (`p.le`, `String::fr`) completes like `p.` and `String::`: the
    // filter by prefix at the end keeps the ones that match.
    int at = col - prefix_len;
    int is_scoped =
        pf->source && at > 1 && at <= line_len && ptr[at - 2] == ':' && ptr[at - 1] == ':';
    if (pf->source && at > 0 && at <= line_len && (ptr[at - 1] == '.' || is_scoped))
    {
        // After a `.` or a `::`, no keyword or free name fits.
        dot_completed = 1;
        int i = at - (is_scoped ? 3 : 2);
        while (i >= 0 && (ptr[i] == ' ' || ptr[i] == '\t'))
        {
            i--;
        }
        if (i >= 0)
        {
            int end_ident = i;
            while (i >= 0 && (isalnum((unsigned char)ptr[i]) || ptr[i] == '_' || ptr[i] == '.' ||
                              ptr[i] == '-' || ptr[i] == '>' || ptr[i] == '<'))
            {
                i--;
            }
            int start_ident = i + 1;

            if (start_ident <= end_ident)
            {
                int len = end_ident - start_ident + 1;
                char expr_chain[MAX_VAR_NAME_LEN * 4];
                if (len >= (int)sizeof(expr_chain))
                {
                    len = sizeof(expr_chain) - 1;
                }
                strncpy(expr_chain, ptr + start_ident, (size_t)(len));
                expr_chain[len] = 0;

                // e.g. "a.b.c" -> parts: "a", "b", "c"
                char *parts[32];
                int part_count = 0;

                // Replace '->' with '.' to simplify splitting
                for (int c = 0; expr_chain[c]; c++)
                {
                    if (expr_chain[c] == '-' && expr_chain[c + 1] == '>')
                    {
                        expr_chain[c] = '.';
                        expr_chain[c + 1] = '.'; // Will be skipped empty parts
                    }
                }

                char *copy = xstrdup(expr_chain);
                char *t = strtok(copy, ".");
                while (t && part_count < 32)
                {
                    parts[part_count++] = t;
                    t = strtok(NULL, ".");
                }

                if (is_scoped && part_count == 1 && g_project->ctx)
                {
                    EnumVariantReg *ev = g_project->ctx->enum_variants;
                    while (ev)
                    {
                        if (strcmp(ev->enum_name, parts[0]) == 0)
                        {
                            cJSON *item = cJSON_CreateObject();
                            cJSON_AddStringToObject(item, "label", ev->variant_name);
                            cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_ENUM_MEMBER);
                            cJSON_AddStringToObject(item, "detail", "enum variant");
                            cJSON_AddItemToArray(items, item);
                        }
                        ev = ev->next;
                    }
                    add_impl_completions(items, parts[0], 0);
                }

                if (!is_scoped && part_count > 0)
                {
                    char *base_name = parts[0];
                    Type *resolved_type = resolve_local_type(target_func, base_name);
                    char *type_name = NULL;

                    if (resolved_type)
                    {
                        type_name = type_to_string(resolved_type);
                    }
                    else
                    {
                        ASTNode *decl = lsp_find_local_decl(target_func, base_name);
                        if (decl && decl->kind == NODE_VAR_DECL && decl->type_info)
                        {
                            type_name = type_to_string(decl->type_info);
                        }
                        else
                        {
                            ZenSymbol *sym = find_symbol_in_all(g_project->ctx, base_name);
                            if (sym)
                            {
                                if (sym->type_info)
                                {
                                    type_name = type_to_string(sym->type_info);
                                }
                            }
                        }
                    }

                    // Now traverse properties
                    for (int p = 1; p < part_count && type_name; p++)
                    {
                        char clean_name[MAX_VAR_NAME_LEN];
                        char *src = type_name;
                        if (strncmp(src, "struct ", 7) == 0)
                        {
                            src += 7;
                        }
                        char *dst = clean_name;
                        while (*src && *src != '*' && *src != '<' && *src != '[')
                        {
                            *dst++ = *src++;
                        }
                        *dst = 0;

                        ASTNode *struct_node = find_struct_def(g_project->ctx, clean_name);
                        int found_field = 0;
                        if (struct_node && struct_node->kind == NODE_STRUCT)
                        {
                            ASTNode *field = struct_node->strct.fields;
                            while (field)
                            {
                                if (strcmp(field->field.name, parts[p]) == 0)
                                {
                                    zfree(type_name);
                                    type_name = type_to_string(field->type_info);
                                    found_field = 1;
                                    break;
                                }
                                field = field->next;
                            }
                        }
                        char c_type[MAX_SHORT_MSG_LEN];
                        if (!found_field && !struct_node &&
                            lsp_c_headers_field_type(pf->path, pf->source, clean_name, parts[p],
                                                     c_type, sizeof(c_type)))
                        {
                            // A field of a struct from a C header.
                            zfree(type_name);
                            type_name = xstrdup(c_type);
                            found_field = 1;
                        }
                        if (!found_field)
                        {
                            zfree(type_name);
                            type_name = NULL;
                        }
                    }

                    if (type_name)
                    {
                        char clean_name[MAX_VAR_NAME_LEN];
                        char *src = type_name;
                        if (strncmp(src, "struct ", 7) == 0)
                        {
                            src += 7;
                        }

                        char *dst = clean_name;
                        while (*src && *src != '*' && *src != '<' && *src != '[')
                        {
                            *dst++ = *src++;
                        }
                        *dst = 0;

                        ASTNode *struct_node = find_struct_def(g_project->ctx, clean_name);
                        if (!struct_node)
                        {
                            // A struct from a C header: its fields as the header declares them.
                            lsp_c_headers_add_fields(pf->path, pf->source, clean_name, items);
                        }
                        else
                        {
                            if (struct_node->kind == NODE_STRUCT)
                            {
                                ASTNode *field = struct_node->strct.fields;
                                while (field)
                                {
                                    cJSON *item = cJSON_CreateObject();
                                    cJSON_AddStringToObject(item, "label", field->field.name);
                                    cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_FIELD);
                                    cJSON_AddStringToObject(
                                        item, "detail",
                                        zen_c_type_spelling(g_project->ctx, field->type_info));
                                    cJSON_AddItemToArray(items, item);
                                    field = field->next;
                                }
                            }
                            else if (struct_node->kind == NODE_ENUM)
                            {
                                ASTNode *variant = struct_node->enm.variants;
                                while (variant)
                                {
                                    cJSON *item = cJSON_CreateObject();
                                    cJSON_AddStringToObject(item, "label", variant->variant.name);
                                    cJSON_AddNumberToObject(item, "kind",
                                                            LSP_COMPLETION_ENUM_MEMBER);
                                    cJSON_AddItemToArray(items, item);
                                    variant = variant->next;
                                }
                            }
                        }

                        add_impl_completions(items, clean_name, 1);
                        zfree(type_name);
                    }
                }
                zfree(copy);
            }
        }
    }

    if (!dot_completed)
    {
        typedef struct
        {
            const char *label;
            const char *snippet;
            int context_mask; // 1: Global, 2: Function, 4: Loop, 8: AfterIf
        } KeywordSnippet;

        KeywordSnippet keywords[] = {
            {"fn", "fn ${1:name}(${2:args}) {\n    $0\n}", 1},
            {"struct", "struct ${1:Name} {\n    ${2:field}: ${3:type};\n}", 1},
            {"enum", "enum ${1:Name} {\n    ${2:Variant}\n}", 1},
            {"trait", "trait ${1:Name} {\n    fn ${2:method}(self);\n}", 1},
            {"impl", "impl ${1:Struct} {\n    $0\n}", 1},
            {"if", "if (${1:condition}) {\n    $0\n}", 2 | 4 | 8},
            {"else", "else {\n    $0\n}", 8},
            {"for", "for (${1:let i = 0}; ${2:i < 10}; ${3:i++}) {\n    $0\n}", 2 | 4 | 8},
            {"for..in", "for (${1:item} in ${2:iterator}) {\n    $0\n}", 2 | 4 | 8},
            {"while", "while (${1:condition}) {\n    $0\n}", 2 | 4 | 8},
            {"match", "match (${1:expression}) {\n    ${2:pattern} => ${3:action},\n    _ => $0\n}",
             2 | 4 | 8},
            {"defer", "defer {\n    $0\n}", 2 | 4 | 8},
            {"return", "return $0;", 2 | 4 | 8},
            {"let", "let ${1:var}: ${2:type} = ${3:value};", 2 | 4 | 8},
            {"def", "def ${1:CONST} = ${2:value};", 1 | 2 | 4 | 8},
            {"test", "test \"${1:description}\" {\n    $0\n}", 1},
            {"assert", "assert(${1:expression}, \"${2:error message}\");", 2 | 4 | 8},
            {"break", "break;", 4},
            {"continue", "continue;", 4},
            {NULL, NULL, 0}};

        int active_mask = 0;
        if (context == CTX_GLOBAL)
        {
            active_mask = 1;
        }
        else if (context == CTX_FUNCTION)
        {
            active_mask = 2;
        }
        else if (context == CTX_LOOP)
        {
            active_mask = 4;
        }
        else if (context == CTX_AFTER_IF)
        {
            active_mask = 8;
        }
        else if (context == CTX_ASSIGNMENT)
        {
            active_mask = 2 | 4 | 8; // Assignments usually in functions
        }

        // If we are after an if, we still want to allow 'if' keywords (else if)
        if (context == CTX_AFTER_IF)
        {
            active_mask |= 2;
        }

        for (int i = 0; keywords[i].label; i++)
        {
            if (!(keywords[i].context_mask & active_mask))
            {
                continue;
            }

            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "label", keywords[i].label);
            cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_KEYWORD);
            cJSON_AddStringToObject(item, "insertText", keywords[i].snippet);
            cJSON_AddNumberToObject(item, "insertTextFormat", LSP_INSERT_SNIPPET);
            cJSON_AddStringToObject(item, "sortText", "001"); // Keywords highest priority
            cJSON_AddItemToArray(items, item);
        }

        // Primitive type names — reuse the authoritative table from primitives.c
        int type_count = 0;
        const ZenPrimitive *types = get_zen_primitives(&type_count);
        for (int i = 0; i < type_count; i++)
        {
            if (context == CTX_GLOBAL)
            {
                continue;
            }
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "label", types[i].zen_name);
            cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_KEYWORD);
            cJSON_AddStringToObject(item, "sortText", "008");
            cJSON_AddItemToArray(items, item);
        }

        // Non-type keywords and builtins
        // Not the keywords above with a snippet, nor print/println: functions of std/io.zc.
        const char *plain_keywords[] = {"alias",
                                        "and",
                                        "asm",
                                        "code",
                                        "compile_error",
                                        "compile_warn",
                                        "comptime",
                                        "__COMPTIME_FILE__",
                                        "__COMPTIME_TARGET__",
                                        "const",
                                        "do",
                                        "embed",
                                        "exit",
                                        "expect",
                                        "export",
                                        "extern",
                                        "false",
                                        "fopen",
                                        "free",
                                        "goto",
                                        "guard",
                                        "import",
                                        "include",
                                        "inline",
                                        "it",
                                        "launch",
                                        "loop",
                                        "malloc",
                                        "mkdir",
                                        "module",
                                        "noreturn",
                                        "not",
                                        "null",
                                        "opaque",
                                        "or",
                                        "plugin",
                                        "printf",
                                        "raw",
                                        "realloc",
                                        "ref",
                                        "register",
                                        "repeat",
                                        "restrict",
                                        "self",
                                        "sizeof",
                                        "snprintf",
                                        "sprintf",
                                        "static",
                                        "stdout",
                                        "step",
                                        "strcmp",
                                        "strcpy",
                                        "strlen",
                                        "system",
                                        "true",
                                        "type",
                                        "typedef",
                                        "union",
                                        "unless",
                                        "unsafe",
                                        "use",
                                        "usleep",
                                        "va_list",
                                        "volatile",
                                        "with",
                                        "yield",
                                        NULL};

        for (int i = 0; plain_keywords[i]; i++)
        {
            if (context == CTX_GLOBAL)
            {
                const char *kw = plain_keywords[i];
                if (strcmp(kw, "import") != 0 && strcmp(kw, "module") != 0 &&
                    strcmp(kw, "alias") != 0 && strcmp(kw, "use") != 0 &&
                    strcmp(kw, "const") != 0 && strcmp(kw, "def") != 0 &&
                    strcmp(kw, "export") != 0 && strcmp(kw, "extern") != 0 &&
                    strcmp(kw, "static") != 0 && strcmp(kw, "type") != 0 &&
                    strcmp(kw, "comptime") != 0 && strcmp(kw, "include") != 0 &&
                    strcmp(kw, "plugin") != 0 && strcmp(kw, "opaque") != 0 &&
                    strcmp(kw, "expect") != 0 && strcmp(kw, "test") != 0 &&
                    strcmp(kw, "raw") != 0 && strcmp(kw, "embed") != 0 && strcmp(kw, "do") != 0 &&
                    strcmp(kw, "loop") != 0 && strcmp(kw, "unsafe") != 0)
                {
                    continue;
                }
            }

            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "label", plain_keywords[i]);
            cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_KEYWORD);
            cJSON_AddStringToObject(item, "sortText", "008");
            cJSON_AddItemToArray(items, item);
        }

        StructRef *g = g_project->ctx->parsed_globals_list;
        while (g)
        {
            if (g->node)
            {
                cJSON *item = cJSON_CreateObject();
                char *name = g->node->var_decl.name;
                cJSON_AddStringToObject(item, "label", name);
                cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_CONSTANT);
                cJSON_AddStringToObject(item, "sortText", "050"); // Globals in middle
                cJSON_AddItemToArray(items, item);
            }
            g = g->next;
        }

        // Methods, instances and module members complete after `.` and `::`.
        StructDef *s = g_project->ctx->struct_defs;
        for (; s; s = s->next)
        {
            if (!offered_by_name(s->node ? s->node->token : TOKEN_UNKNOWN, s->name))
            {
                continue;
            }
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "label", s->name);
            cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_STRUCT);
            cJSON_AddStringToObject(item, "sortText", "060");
            cJSON_AddItemToArray(items, item);
        }

        FuncSig *f = g_project->ctx->func_registry;
        for (; f; f = f->next)
        {
            if (!offered_by_name(f->decl_token, f->name))
            {
                continue;
            }
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "label", f->name);
            cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_FUNCTION);
            cJSON_AddStringToObject(item, "sortText", "070");

            // The signature as written, as the hover shows it
            char detail[MAX_SHORT_MSG_LEN * 4];
            if (lsp_function_signature(f->decl_token, detail, sizeof(detail)))
            {
                cJSON_AddStringToObject(item, "detail", detail);
            }

            // Snippet to jump inside parens
            char snippet[MAX_VAR_NAME_LEN];
            if (f->total_args > 0)
            {
                snprintf(snippet, sizeof(snippet), "%s($1)", f->name);
            }
            else
            {
                snprintf(snippet, sizeof(snippet), "%s()", f->name);
            }
            cJSON_AddStringToObject(item, "insertText", snippet);
            cJSON_AddNumberToObject(item, "insertTextFormat", LSP_INSERT_SNIPPET);

            cJSON_AddItemToArray(items, item);
        }

        if (target_func)
        {
            if (target_func->func.param_names)
            {
                for (int i = 0; i < target_func->func.count; i++)
                {
                    cJSON *item = cJSON_CreateObject();
                    cJSON_AddStringToObject(item, "label", target_func->func.param_names[i]);
                    cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_VARIABLE);
                    cJSON_AddStringToObject(item, "detail", "argument");
                    cJSON_AddStringToObject(item, "sortText", "001"); // Arg priority
                    cJSON_AddItemToArray(items, item);
                }
            }
            lsp_for_each_local(target_func, line, add_local_completion, items);
        }
    }

    if (prefix_len > 0)
    {
        size_t plen = (size_t)prefix_len;
        int i = 0;
        while (i < cJSON_GetArraySize(items))
        {
            cJSON *item = cJSON_GetArrayItem(items, i);
            cJSON *label_obj = cJSON_GetObjectItem(item, "label");
            const char *label = label_obj ? label_obj->valuestring : "";
            int match = 1;
            size_t llen = strlen(label);
            if (llen < plen)
            {
                match = 0;
            }
            else
            {
                for (size_t j = 0; j < plen; j++)
                {
                    if (tolower((unsigned char)label[j]) != tolower((unsigned char)prefix[j]))
                    {
                        match = 0;
                        break;
                    }
                }
            }
            if (!match)
            {
                cJSON_DeleteItemFromArray(items, i);
            }
            else
            {
                i++;
            }
        }
    }

    return items;
}

// Name and LSP SymbolKind of a declaration, or NULL when the node is not one.
static const char *declaration_name(ASTNode *node, LspSymbolKind *kind)
{
    if (node->kind == NODE_FUNCTION)
    {
        *kind = LSP_SYMBOL_FUNCTION;
        return node->func.name;
    }
    if (node->kind == NODE_STRUCT)
    {
        *kind = LSP_SYMBOL_STRUCT;
        return node->strct.name;
    }
    if (node->kind == NODE_VAR_DECL)
    {
        *kind = LSP_SYMBOL_VARIABLE;
        return node->var_decl.name;
    }
    if (node->kind == NODE_CONST)
    {
        *kind = LSP_SYMBOL_CONSTANT;
        return node->var_decl.name;
    }
    if (node->kind == NODE_ENUM)
    {
        *kind = LSP_SYMBOL_ENUM;
        return node->enm.name;
    }
    if (node->kind == NODE_FIELD)
    {
        *kind = LSP_SYMBOL_FIELD;
        return node->field.name;
    }
    return NULL;
}

static cJSON *ast_to_symbol(ASTNode *node)
{
    if (!node)
    {
        return NULL;
    }
    LspSymbolKind kind = LSP_SYMBOL_VARIABLE;
    const char *name = declaration_name(node, &kind);
    if (!name)
    {
        return NULL;
    }

    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "name", name);
    cJSON_AddNumberToObject(item, "kind", kind);

    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", node->token.line > 0 ? node->token.line - 1 : 0);
    cJSON_AddNumberToObject(start, "character", node->token.col > 0 ? node->token.col - 1 : 0);

    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", node->token.line > 0 ? node->token.line - 1 : 0);
    cJSON_AddNumberToObject(
        end, "character",
        (double)((node->token.col > 0 ? node->token.col - 1 : 0) + (int)node->token.len));

    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddItemToObject(range, "end", end);

    cJSON_AddItemToObject(item, "range", range);
    cJSON *selRange = cJSON_Duplicate(range, 1);
    cJSON_AddItemToObject(item, "selectionRange", selRange);

    if (node->kind == NODE_STRUCT && node->strct.fields)
    {
        cJSON *children = cJSON_CreateArray();
        ASTNode *f = node->strct.fields;
        while (f)
        {
            cJSON *child = ast_to_symbol(f);
            if (child)
            {
                cJSON_AddItemToArray(children, child);
            }
            f = f->next;
        }
        cJSON_AddItemToObject(item, "children", children);
    }

    return item;
}

cJSON *lsp_document_symbol(const char *uri)
{
    ProjectFile *pf = lsp_project_get_file(uri);
    if (!pf || !pf->ast)
    {
        return NULL;
    }

    cJSON *items = cJSON_CreateArray();
    ASTNode *node = pf->ast;

    // Unwrap ROOT node if present
    if (node && node->kind == NODE_ROOT)
    {
        node = node->root.children;
    }

    while (node)
    {
        cJSON *s = ast_to_symbol(node);
        if (s)
        {
            cJSON_AddItemToArray(items, s);
        }
        node = node->next; // Top level siblings
    }

    return items;
}

// `query` matches `name` when its characters appear in it in order, ignoring case. The
// client ranks and filters the results further.
static int matches_query(const char *name, const char *query)
{
    for (; *query; query++)
    {
        int wanted = tolower((unsigned char)*query);
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

static void add_workspace_symbol(cJSON *items, const char *name, LspSymbolKind kind,
                                 const char *uri, Token token, const char *container)
{
    int line = token.line > 0 ? token.line - 1 : 0;
    int col = token.col > 0 ? token.col - 1 : 0;
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "name", name);
    cJSON_AddNumberToObject(item, "kind", kind);
    if (container)
    {
        cJSON_AddStringToObject(item, "containerName", container);
    }
    cJSON *location = cJSON_CreateObject();
    cJSON_AddStringToObject(location, "uri", uri);
    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", line);
    cJSON_AddNumberToObject(start, "character", col);
    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", line);
    cJSON_AddNumberToObject(end, "character", col + (int)token.len);
    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddItemToObject(range, "end", end);
    cJSON_AddItemToObject(location, "range", range);
    cJSON_AddItemToObject(item, "location", location);
    cJSON_AddItemToArray(items, item);
}

#define MAX_WORKSPACE_SYMBOLS 500

cJSON *lsp_workspace_symbol(const char *query)
{
    cJSON *items = cJSON_CreateArray();
    int count = 0;

    for (ProjectFile *pf = g_project ? g_project->files : NULL; pf && count < MAX_WORKSPACE_SYMBOLS;
         pf = pf->next)
    {
        // The symbols of the workspace, not of the modules it imports from outside it.
        ASTNode *node = pf->is_external ? NULL : pf->ast;
        if (node && node->kind == NODE_ROOT)
        {
            node = node->root.children;
        }
        for (; node && count < MAX_WORKSPACE_SYMBOLS; node = node->next)
        {
            LspSymbolKind kind = LSP_SYMBOL_VARIABLE;
            const char *name = declaration_name(node, &kind);
            if (!name && node->kind == NODE_TRAIT)
            {
                name = node->trait.name;
                kind = LSP_SYMBOL_INTERFACE;
            }
            if (name && matches_query(name, query))
            {
                add_workspace_symbol(items, name, kind, pf->uri, node->token, NULL);
                count++;
            }

            ASTNode *methods = NULL;
            const char *container = NULL;
            if (node->kind == NODE_IMPL)
            {
                methods = node->impl.methods;
                container = type_impl_name(node->impl.target);
            }
            else if (node->kind == NODE_IMPL_TRAIT)
            {
                methods = node->impl_trait.methods;
                container = type_impl_name(node->impl_trait.target);
            }
            for (ASTNode *method = methods; method && count < MAX_WORKSPACE_SYMBOLS;
                 method = method->next)
            {
                if (method->kind != NODE_FUNCTION || !method->func.method_name)
                {
                    continue;
                }
                // `func.name` is the C name (`Rect__area`).
                const char *short_name = method->func.method_name;
                if (matches_query(short_name, query))
                {
                    add_workspace_symbol(items, short_name, LSP_SYMBOL_METHOD, pf->uri,
                                         method->token, container);
                    count++;
                }
            }
        }
    }

    return items;
}

cJSON *lsp_references(const char *uri, int line, int col)
{
    ProjectFile *pf = lsp_project_get_file(uri);
    cJSON *items = cJSON_CreateArray();

    if (pf && pf->index)
    {
        LSPRange *r = lsp_find_at(pf->index, line, col);
        if (r && r->symbol.name)
        {
            ReferenceResult *refs = lsp_project_find_references(r->symbol.definition);
            for (ReferenceResult *curr = refs; curr; curr = curr->next)
            {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "uri", curr->uri);
                cJSON *range = cJSON_CreateObject();
                cJSON *start = cJSON_CreateObject();
                cJSON_AddNumberToObject(start, "line", curr->range->start_line);
                cJSON_AddNumberToObject(start, "character", curr->range->start_col);

                cJSON *end = cJSON_CreateObject();
                cJSON_AddNumberToObject(end, "line", curr->range->end_line);
                cJSON_AddNumberToObject(end, "character", curr->range->end_col);

                cJSON_AddItemToObject(range, "start", start);
                cJSON_AddItemToObject(range, "end", end);
                cJSON_AddItemToObject(item, "range", range);
                cJSON_AddItemToArray(items, item);
            }
        }
    }

    return items;
}

// The parameters of a signature as written, split at the commas of its parentheses only: a
// parameter such as `m: Map<K, V>` or `f: fn(int, int) -> int` is one.
static cJSON *signature_parameters(const char *signature)
{
    cJSON *params = cJSON_CreateArray();
    const char *p = strchr(signature, '(');
    if (!p)
    {
        return params;
    }
    const char *start = ++p;
    int depth = 0;
    char quote = 0;
    for (; *p; p++)
    {
        if (quote)
        {
            if (*p == '\\' && p[1])
            {
                p++;
            }
            else if (*p == quote)
            {
                quote = 0;
            }
            continue;
        }
        if (*p == '"' || *p == '\'')
        {
            quote = *p;
        }
        else if (*p == '(' || *p == '[' || *p == '<')
        {
            depth++;
        }
        else if (depth > 0 && (*p == ')' || *p == ']' || (*p == '>' && p[-1] != '-')))
        {
            depth--;
        }
        else if (depth == 0 && (*p == ',' || *p == ')'))
        {
            while (start < p && *start == ' ')
            {
                start++;
            }
            if (p > start)
            {
                char *param = xmalloc((size_t)(p - start) + 1);
                memcpy(param, start, (size_t)(p - start));
                param[p - start] = 0;
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "label", param);
                cJSON_AddItemToArray(params, item);
            }
            if (*p == ')')
            {
                break;
            }
            start = p + 1;
        }
    }
    return params;
}

cJSON *lsp_signature_help(const char *uri, int line, int col)
{
    ProjectFile *pf = lsp_project_get_file(uri);
    if (!g_project || !g_project->ctx || !pf || !pf->source)
    {
        return NULL;
    }

    // ... [Scan backwards logic same as before] ...
    char *ptr = pf->source;
    int cur_line = 0;
    while (*ptr && cur_line < line)
    {
        if (*ptr == '\n')
        {
            cur_line++;
        }
        ptr++;
    }
    if (col > 0)
    {
        ptr += col;
    }

    if (ptr > pf->source + strlen(pf->source))
    {
        return NULL;
    }

    cJSON *result = NULL;
    char *p = ptr - 1;
    while (p >= pf->source)
    {
        if (*p == ')')
        {
            break;
        }
        if (*p == '(')
        {
            // Found open paren
            char *ident_end = p - 1;
            while (ident_end >= pf->source && isspace(*ident_end))
            {
                ident_end--;
            }
            if (ident_end < pf->source)
            {
                break;
            }
            char *ident_start = ident_end;
            while (ident_start >= pf->source && (isalnum(*ident_start) || *ident_start == '_'))
            {
                ident_start--;
            }
            ident_start++;

            ptrdiff_t len = ident_end - ident_start + 1;
            if (len > 0 && len < MAX_FUNC_NAME_LEN - 1)
            {
                char func_name[MAX_FUNC_NAME_LEN];
                strncpy(func_name, ident_start, (size_t)(len));
                func_name[len] = 0;
                // Lookup
                FuncSig *fn = g_project->ctx->func_registry;
                while (fn)
                {
                    if (strcmp(fn->name, func_name) == 0)
                    {
                        // The signature as written, as the hover shows it
                        char label[MAX_SHORT_MSG_LEN * 4];
                        if (!lsp_function_signature(fn->decl_token, label, sizeof(label)))
                        {
                            break;
                        }
                        result = cJSON_CreateObject();
                        cJSON *sigs = cJSON_CreateArray();
                        cJSON *sig = cJSON_CreateObject();
                        cJSON_AddStringToObject(sig, "label", label);
                        cJSON_AddItemToObject(sig, "parameters", signature_parameters(label));
                        cJSON_AddItemToArray(sigs, sig);

                        cJSON_AddItemToObject(result, "signatures", sigs);

                        // Calculate Active Parameter
                        int active = 0;
                        char *active_p = ptr - 1;
                        while (active_p > p)
                        {
                            if (*active_p == ',')
                            {
                                active++;
                            }
                            active_p--;
                        }
                        cJSON_AddNumberToObject(result, "activeSignature", 0);
                        cJSON_AddNumberToObject(result, "activeParameter", active);
                        break;
                    }
                    fn = fn->next;
                }
            }
            break;
        }
        p--;
    }

    return result;
}

// Whether renaming a symbol of this kind edits every place that names it. The index has no uses
// of fields and variants, nor of traits and aliases outside type names; `self` is the receiver
// by its name (see parse_and_convert_args), and with another one the method has none.
static int is_renamable(DeclarationKind kind)
{
    return kind == DECLARATION_FUNCTION || kind == DECLARATION_VARIABLE ||
           kind == DECLARATION_CONSTANT || kind == DECLARATION_PARAMETER ||
           kind == DECLARATION_STRUCT;
}

cJSON *lsp_rename(const char *uri, int line, int col, const char *new_name)
{
    ProjectFile *pf = lsp_project_get_file(uri);

    // The symbol is renamed where its program declares it: not in a module from outside the
    // workspace, which rename does not edit.
    LSPRange *r = pf && pf->index ? lsp_find_at(pf->index, line, col) : NULL;
    ProjectFile *def_file = NULL;
    LSPRange *def = r ? lsp_project_definition(pf, r, &def_file) : NULL;
    if (!def || !def->symbol.name || !is_renamable(def->symbol.kind) || def_file->is_external)
    {
        return NULL;
    }

    ReferenceResult *refs = lsp_project_find_references(def->symbol.definition);
    if (!refs)
    {
        return NULL;
    }

    cJSON *result = cJSON_CreateObject();
    cJSON *changes = cJSON_CreateObject();

    ReferenceResult *cur = refs;
    for (; cur; cur = cur->next)
    {
        // An edit replaces the text of the range: a use written with another name (a name
        // that an import gives it) is not that text.
        if (strcmp(cur->range->symbol.name, def->symbol.name) != 0)
        {
            continue;
        }
        cJSON *edits = cJSON_GetObjectItem(changes, cur->uri);
        if (!edits)
        {
            edits = cJSON_CreateArray();
            cJSON_AddItemToObject(changes, cur->uri, edits);
        }

        cJSON *edit = cJSON_CreateObject();
        cJSON *range = cJSON_CreateObject();
        cJSON *start = cJSON_CreateObject();
        cJSON *end = cJSON_CreateObject();

        cJSON_AddNumberToObject(start, "line", cur->range->start_line);
        cJSON_AddNumberToObject(start, "character", cur->range->start_col);
        cJSON_AddNumberToObject(end, "line", cur->range->end_line);
        cJSON_AddNumberToObject(end, "character", cur->range->end_col);

        cJSON_AddItemToObject(range, "start", start);
        cJSON_AddItemToObject(range, "end", end);
        cJSON_AddItemToObject(edit, "range", range);
        cJSON_AddStringToObject(edit, "newText", new_name);

        cJSON_AddItemToArray(edits, edit);
    }

    cJSON_AddItemToObject(result, "changes", changes);
    return result;
}

cJSON *lsp_code_action(const char *uri, cJSON *diagnostics)
{
    cJSON *actions = cJSON_CreateArray();

    int diag_count = cJSON_GetArraySize(diagnostics);
    for (int i = 0; i < diag_count; i++)
    {
        cJSON *diag = cJSON_GetArrayItem(diagnostics, i);
        cJSON *code = cJSON_GetObjectItem(diag, "code");
        if (!code || !code->valuestring)
        {
            continue;
        }

        cJSON *range = cJSON_GetObjectItem(diag, "range");
        if (!range)
        {
            continue;
        }

        // Note: The code string is "W" + DiagnosticID enum value.
        // We'll check for the specific ones we care about.
        // For robustness, we check the message too or use a lookup.
        cJSON *msg = cJSON_GetObjectItem(diag, "message");
        int is_var = (msg && strstr(msg->valuestring, "'var' is deprecated"));
        int is_const = (msg && strstr(msg->valuestring, "'const' for declarations is deprecated"));

        if (is_var)
        {
            cJSON *action = cJSON_CreateObject();
            cJSON_AddStringToObject(action, "title", "Replace 'var' with 'let'");
            cJSON_AddStringToObject(action, "kind", "quickfix");

            cJSON *edit = cJSON_CreateObject();
            cJSON *changes = cJSON_CreateObject();
            cJSON *edits = cJSON_CreateArray();

            cJSON *text_edit = cJSON_CreateObject();
            cJSON_AddItemToObject(text_edit, "range", cJSON_Duplicate(range, 1));
            cJSON_AddStringToObject(text_edit, "newText", "let");
            cJSON_AddItemToArray(edits, text_edit);

            cJSON_AddItemToObject(changes, uri, edits);
            cJSON_AddItemToObject(edit, "changes", changes);
            cJSON_AddItemToObject(action, "edit", edit);

            cJSON_AddItemToArray(actions, action);
        }
        else if (is_const)
        {
            cJSON *action = cJSON_CreateObject();
            cJSON_AddStringToObject(action, "title", "Replace 'const' with 'def'");
            cJSON_AddStringToObject(action, "kind", "quickfix");

            cJSON *edit = cJSON_CreateObject();
            cJSON *changes = cJSON_CreateObject();
            cJSON *edits = cJSON_CreateArray();

            cJSON *text_edit = cJSON_CreateObject();
            cJSON_AddItemToObject(text_edit, "range", cJSON_Duplicate(range, 1));
            cJSON_AddStringToObject(text_edit, "newText", "def");
            cJSON_AddItemToArray(edits, text_edit);

            cJSON_AddItemToObject(changes, uri, edits);
            cJSON_AddItemToObject(edit, "changes", changes);
            cJSON_AddItemToObject(action, "edit", edit);

            cJSON_AddItemToArray(actions, action);
        }
    }

    return actions;
}
