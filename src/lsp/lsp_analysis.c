// SPDX-License-Identifier: MIT
#include "../utils/cJSON.h"
#include "../constants.h"
#include "../ast/primitives.h"
#include "../lexer/keywords.h"
#include "../utils/utils.h"
#include "../utils/emitter.h"
#include "lsp_project.h"
#include "lsp_analysis.h" // Includes lsp_index.h, parser.h
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

// Warns about the type names of `pf` that name no type of the program nor of the C headers
// the document includes or imports. `zc build` warns about them too, and also about the types
// of those headers, which it does not read.
static void warn_unknown_types(ProjectFile *pf)
{
    for (TypeUsage *use = pf->unknown_types; use; use = use->next)
    {
        const CHeaderSymbol *declaration;
        if (lsp_c_headers_find_at(pf->path, pf->source, use->location.line - 1,
                                  use->location.col - 1, &declaration, 1, NULL) == 0)
        {
            warn_unknown_type(g_project->ctx, use);
        }
    }
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
    ProjectFile *pf = lsp_project_get_file(uri);
    if (pf)
    {
        warn_unknown_types(pf);
    }

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
        // The DiagnosticID, as the JSON output of the command line gives it.
        if (d->id != DIAG_NONE)
        {
            cJSON_AddNumberToObject(diag, "code", d->id);
        }

        cJSON_AddItemToArray(diag_array, diag);

        d = d->next;
    }

    lsp_c_headers_add_diagnostics(lsp_uri_to_path(uri), json_src, diag_array);

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

// What a value of a primitive type is, for its hover. NULL for a kind that no primitive has.
ZEN_CONST static const char *primitive_description(TypeKind kind)
{
    switch (kind)
    {
    case TYPE_VOID:
        return "The empty type: it has no values.";
    case TYPE_BOOL:
        return "Boolean: `true` or `false`.";
    case TYPE_CHAR:
        return "A C character.";
    case TYPE_STRING:
        return "A string of UTF-8 characters.";
    case TYPE_I8:
        return "8-bit signed integer.";
    case TYPE_U8:
        return "8-bit unsigned integer.";
    case TYPE_BYTE:
        return "A byte: 8-bit unsigned integer.";
    case TYPE_I16:
        return "16-bit signed integer.";
    case TYPE_U16:
        return "16-bit unsigned integer.";
    case TYPE_I32:
        return "32-bit signed integer.";
    case TYPE_U32:
        return "32-bit unsigned integer.";
    case TYPE_RUNE:
        return "A Unicode scalar value (a 32-bit code point).";
    case TYPE_I64:
        return "64-bit signed integer.";
    case TYPE_U64:
        return "64-bit unsigned integer.";
    case TYPE_I128:
        return "128-bit signed integer.";
    case TYPE_U128:
        return "128-bit unsigned integer.";
    case TYPE_F32:
        return "32-bit floating point number.";
    case TYPE_F64:
        return "64-bit floating point number.";
    case TYPE_USIZE:
        return "Unsigned integer of the size of a pointer.";
    case TYPE_ISIZE:
        return "Signed integer of the size of a pointer.";
    case TYPE_C_INT:
    case TYPE_C_UINT:
    case TYPE_C_LONG:
    case TYPE_C_ULONG:
    case TYPE_C_LONGLONG:
    case TYPE_C_ULONGLONG:
    case TYPE_C_SHORT:
    case TYPE_C_USHORT:
    case TYPE_C_CHAR:
    case TYPE_C_UCHAR:
        return "The C type, whose size depends on the platform.";
    default:
        return NULL;
    }
}

// The hover of a primitive type's name (`i32`, `int`, `c_long`...), as `primitives.c` defines
// them; NULL when `word` is none. In the arena.
static char *primitive_hover(const char *word)
{
    const ZenPrimitive *primitive = find_primitive_by_name(word);
    const char *description = primitive ? primitive_description(primitive->kind) : NULL;
    if (!description)
    {
        return NULL;
    }
    size_t size = strlen(word) + strlen(description) + strlen(primitive->c_name) + 32;
    char *hover = xmalloc(size);
    snprintf(hover, size, "**%s**: %s `%s` in C.", word, description, primitive->c_name);
    return hover;
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
            const char *doc = primitive_hover(word);
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

// The name a top-level declaration of the project is offered and found by, as it is written; NULL
// for a node that declares no global name, or whose name is not written (a module's, `m__f`).
static const char *declared_name(ASTNode *node)
{
    const char *name = node->kind == NODE_FUNCTION     ? node->func.name
                       : node->kind == NODE_STRUCT     ? node->strct.name
                       : node->kind == NODE_ENUM       ? node->enm.name
                       : node->kind == NODE_TRAIT      ? node->trait.name
                       : node->kind == NODE_TYPE_ALIAS ? node->type_alias.alias
                       : node->kind == NODE_VAR_DECL || node->kind == NODE_CONST
                           ? node->var_decl.name
                           : NULL;
    return name && offered_by_name(node->token, name) ? name : NULL;
}

// Calls `visit` with each declaration of the project that is not inside another one, as the
// files' last parse left them: the top level of each file's AST, and its generic functions, which
// are templates and no nodes of the AST. Stops at the first node for which `visit` returns
// nonzero, and returns it; NULL when it visits them all.
static ASTNode *visit_declarations(int (*visit)(ASTNode *node, void *data), void *data)
{
    for (ProjectFile *pf = g_project->files; pf; pf = pf->next)
    {
        for (ASTNode *node = lsp_project_top_level(pf); node; node = node->next)
        {
            if (visit(node, data))
            {
                return node;
            }
        }
        for (int i = 0; i < pf->generic_function_count; i++)
        {
            if (visit(pf->generic_functions[i], data))
            {
                return pf->generic_functions[i];
            }
        }
    }
    return NULL;
}

typedef struct
{
    const char *name;
    NodeType kind;
} DeclarationQuery;

static int is_declaration_of(ASTNode *node, void *data)
{
    const DeclarationQuery *query = data;
    const char *declared = node->kind == query->kind ? declared_name(node) : NULL;
    return declared && strcmp(declared, query->name) == 0;
}

// The declaration of the project named `name` of the kind `kind` (see visit_declarations); NULL if
// none.
static ASTNode *find_declaration(const char *name, NodeType kind)
{
    DeclarationQuery query = {name, kind};
    return visit_declarations(is_declaration_of, &query);
}

// Adds the completion item of a declaration (see visit_declarations) to the array `data`, under
// its written name.
static int add_declaration_completion(ASTNode *node, void *data)
{
    cJSON *items = data;
    const char *name = declared_name(node);
    if (!name || has_completion(items, name))
    {
        return 0;
    }
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "label", name);
    if (node->kind == NODE_FUNCTION)
    {
        cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_FUNCTION);
        cJSON_AddStringToObject(item, "sortText", "070");
        // The signature as written, as the hover shows it
        char detail[MAX_SHORT_MSG_LEN * 4];
        if (lsp_function_signature(node->token, detail, sizeof(detail)))
        {
            cJSON_AddStringToObject(item, "detail", detail);
        }
        // Snippet to jump inside parens
        char snippet[MAX_VAR_NAME_LEN];
        snprintf(snippet, sizeof(snippet), "%s(%s)", name, node->func.count > 0 ? "$1" : "");
        cJSON_AddStringToObject(item, "insertText", snippet);
        cJSON_AddNumberToObject(item, "insertTextFormat", LSP_INSERT_SNIPPET);
    }
    else if (node->kind == NODE_TRAIT)
    {
        cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_INTERFACE);
        cJSON_AddStringToObject(item, "sortText", "060");
    }
    else if (node->kind == NODE_STRUCT || node->kind == NODE_ENUM || node->kind == NODE_TYPE_ALIAS)
    {
        // An alias goes as a struct, as other servers show it: the protocol has no kind for it.
        cJSON_AddNumberToObject(
            item, "kind", node->kind == NODE_ENUM ? LSP_COMPLETION_ENUM : LSP_COMPLETION_STRUCT);
        cJSON_AddStringToObject(item, "sortText", "060");
    }
    else
    {
        cJSON_AddNumberToObject(item, "kind",
                                node->kind == NODE_CONST ? LSP_COMPLETION_CONSTANT
                                                         : LSP_COMPLETION_VARIABLE);
        cJSON_AddStringToObject(item, "sortText", "050"); // Globals in middle
    }
    cJSON_AddItemToArray(items, item);
    return 0;
}

// Adds the methods of `impl` that take `self` when `with_self`, the other ones otherwise, if it is
// an impl of `type_name`: the same type, or the template of the instance `inst` when the impl is
// a template. Returns how many were added.
// Whether `method` takes `self` first.
static int takes_self(ASTNode *method)
{
    return method->func.count > 0 && method->func.param_names && method->func.param_names[0] &&
           strcmp(method->func.param_names[0], "self") == 0;
}

// Adds the completion item of `method`, of `owner` (an impl's target or a trait), with `kind`
// (method or function), unless an item has its name. Returns whether it added it.
static int add_method_completion(cJSON *items, ASTNode *method, const char *owner, int kind)
{
    // The name as written: `func.name` is the C name of an impl's method (`Point__len2`), and a
    // trait's method has the token of its `fn`.
    const char *name = method->func.method_name;
    if (!name)
    {
        return 0;
    }
    // A trait can repeat a name.
    if (has_completion(items, name))
    {
        return 0;
    }
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "label", name);
    cJSON_AddNumberToObject(item, "kind", kind);
    char detail[MAX_SHORT_MSG_LEN];
    snprintf(detail, sizeof(detail), "fn %s::%s", owner, name);
    cJSON_AddStringToObject(item, "detail", detail);
    char snippet[MAX_VAR_NAME_LEN];
    snprintf(snippet, sizeof(snippet), "%s(%s)", name,
             method->func.count > takes_self(method) ? "$1" : "");
    cJSON_AddStringToObject(item, "insertText", snippet);
    cJSON_AddNumberToObject(item, "insertTextFormat", LSP_INSERT_SNIPPET);
    cJSON_AddItemToArray(items, item);
    return 1;
}

// Adds the methods of `trait`: through a value of the trait all of them are called, with `self`
// or without it.
static void add_trait_methods(cJSON *items, ASTNode *trait)
{
    for (ASTNode *method = trait->trait.methods; method; method = method->next)
    {
        if (method->kind == NODE_FUNCTION && method->token.start)
        {
            add_method_completion(items, method, trait->trait.name, LSP_COMPLETION_METHOD);
        }
    }
}

static int add_impl_methods(cJSON *items, ASTNode *impl, const char *type_name,
                            const Instantiation *inst, int is_template, int with_self)
{
    size_t base_len = strcspn(type_name, "<");
    const char *target = impl->kind == NODE_IMPL         ? type_impl_name(impl->impl.target)
                         : impl->kind == NODE_IMPL_TRAIT ? type_impl_name(impl->impl_trait.target)
                                                         : NULL;
    size_t target_len = target ? strlen(target) : 0;
    int same = target && target_len == base_len && strncmp(target, type_name, base_len) == 0;
    int instance = target && is_template && inst && strcmp(inst->template_name, target) == 0;
    if (!same && !instance)
    {
        return 0;
    }
    int added = 0;
    ASTNode *method = impl->kind == NODE_IMPL ? impl->impl.methods : impl->impl_trait.methods;
    for (; method; method = method->next)
    {
        if (method->kind != NODE_FUNCTION || takes_self(method) != with_self ||
            !method->token.start)
        {
            continue;
        }
        added += add_method_completion(items, method, target,
                                       with_self ? LSP_COMPLETION_METHOD : LSP_COMPLETION_FUNCTION);
    }
    return added;
}

// What a `.` on a value of type `t` reaches: `t` through its pointers (`p.x` on a `Pak*`).
static Type *member_owner(Type *t)
{
    while (t && t->kind == TYPE_POINTER && t->inner)
    {
        t = t->inner;
    }
    return t;
}

// The struct, enum or trait whose members follow a `.` on a value of type `t`, whatever qualifies
// it (`const Pak*`), in the file `path`: the one that file sees (lsp_project_visible_type), not
// another of that name elsewhere in the workspace. A value of a trait (`h: Header`) has the struct
// type named like the trait. A struct the program does not declare is one of a C header: its name
// goes to `c_struct`, which is left empty otherwise, and there is no definition.
static ASTNode *member_definition(Type *t, const char *path, char *c_struct, size_t size)
{
    c_struct[0] = 0;
    int is_pointer = 0;
    char *allocated = NULL;
    char *name = resolve_struct_name_from_type(g_project->ctx, t, &is_pointer, &allocated);
    ASTNode *visible = name ? lsp_project_visible_type(path, name) : NULL;
    ASTNode *def = visible && visible->kind != NODE_TYPE_ALIAS ? visible
                   : name ? find_struct_def(g_project->ctx, name)
                          : NULL;
    if (name && !def && member_owner(t)->kind == TYPE_STRUCT)
    {
        snprintf(c_struct, size, "%s", name);
    }
    return def;
}

// The name of the struct that `c_type`, the type of a field as a C header writes it
// (`struct Font *`, `const Texture2D`), has; empty for a type that is no struct.
static void c_struct_name(const char *c_type, char *out, size_t size)
{
    const char *p = c_type;
    for (;;)
    {
        while (*p == ' ')
        {
            p++;
        }
        if (strncmp(p, "const ", 6) == 0)
        {
            p += 6;
        }
        else if (strncmp(p, "struct ", 7) == 0)
        {
            p += 7;
        }
        else
        {
            break;
        }
    }
    size_t n = 0;
    while ((isalnum((unsigned char)p[n]) || p[n] == '_') && n + 1 < size)
    {
        out[n] = p[n];
        n++;
    }
    out[n] = 0;
}

// Adds the functions of the impl blocks of `type_name` under the name they are called by: the
// methods that take `self` when `with_self`, the other ones otherwise. A generic, written
// (`Vec<int>`) or instantiated (`Vec__int32_t`), takes those of its template (`impl Vec<T>`).
// The impls are those of the files of the project as their last parse left them, so a method
// that was deleted is not offered. Returns how many were added.
static int add_impl_completions(cJSON *items, const char *type_name, int with_self)
{
    const Instantiation *inst = find_instantiation(g_project->ctx, type_name);
    int added = 0;
    for (ProjectFile *pf = g_project->files; pf; pf = pf->next)
    {
        for (ASTNode *node = lsp_project_top_level(pf); node; node = node->next)
        {
            added += add_impl_methods(items, node, type_name, inst, 0, with_self);
        }
        for (int i = 0; i < pf->generic_impl_count; i++)
        {
            added += add_impl_methods(items, pf->generic_impls[i], type_name, inst, 1, with_self);
        }
    }
    return added;
}

// How completion writes the construct a keyword starts. A keyword may have more than one (`for`
// over a range or an iterator).
static const struct
{
    const char *keyword;
    const char *label;
    const char *snippet;
} KEYWORD_SNIPPETS[] = {
    {"fn", "fn", "fn ${1:name}(${2:args}) {\n    $0\n}"},
    {"struct", "struct", "struct ${1:Name} {\n    ${2:field}: ${3:type};\n}"},
    {"enum", "enum", "enum ${1:Name} {\n    ${2:Variant}\n}"},
    {"trait", "trait", "trait ${1:Name} {\n    fn ${2:method}(self);\n}"},
    {"impl", "impl", "impl ${1:Struct} {\n    $0\n}"},
    {"if", "if", "if (${1:condition}) {\n    $0\n}"},
    {"else", "else", "else {\n    $0\n}"},
    {"for", "for", "for (${1:let i = 0}; ${2:i < 10}; ${3:i++}) {\n    $0\n}"},
    {"for", "for..in", "for (${1:item} in ${2:iterator}) {\n    $0\n}"},
    {"while", "while", "while (${1:condition}) {\n    $0\n}"},
    {"match", "match",
     "match (${1:expression}) {\n    ${2:pattern} => ${3:action},\n    _ => $0\n}"},
    {"defer", "defer", "defer {\n    $0\n}"},
    {"return", "return", "return $0;"},
    {"let", "let", "let ${1:var}: ${2:type} = ${3:value};"},
    {"def", "def", "def ${1:CONST} = ${2:value};"},
    {"test", "test", "test \"${1:description}\" {\n    $0\n}"},
    {"assert", "assert", "assert(${1:expression}, \"${2:error message}\");"},
    {"break", "break", "break;"},
    {"continue", "continue", "continue;"},
};

// Names that completion offers in a body besides the program's: functions of C and the built-ins
// of comptime and lambdas. Not keywords.
static const struct
{
    const char *name;
    LspCompletionItemKind kind;
} BUILTINS[] = {
    {"exit", LSP_COMPLETION_FUNCTION},
    {"fopen", LSP_COMPLETION_FUNCTION},
    {"free", LSP_COMPLETION_FUNCTION},
    {"malloc", LSP_COMPLETION_FUNCTION},
    {"mkdir", LSP_COMPLETION_FUNCTION},
    {"printf", LSP_COMPLETION_FUNCTION},
    {"realloc", LSP_COMPLETION_FUNCTION},
    {"snprintf", LSP_COMPLETION_FUNCTION},
    {"sprintf", LSP_COMPLETION_FUNCTION},
    {"strcmp", LSP_COMPLETION_FUNCTION},
    {"strcpy", LSP_COMPLETION_FUNCTION},
    {"strlen", LSP_COMPLETION_FUNCTION},
    {"system", LSP_COMPLETION_FUNCTION},
    {"usleep", LSP_COMPLETION_FUNCTION},
    {"stdout", LSP_COMPLETION_VARIABLE},
    {"va_list", LSP_COMPLETION_STRUCT},
    {"code", LSP_COMPLETION_FUNCTION},
    {"compile_error", LSP_COMPLETION_FUNCTION},
    {"compile_warn", LSP_COMPLETION_FUNCTION},
    {"yield", LSP_COMPLETION_FUNCTION},
    {"__COMPTIME_FILE__", LSP_COMPLETION_CONSTANT},
    {"__COMPTIME_TARGET__", LSP_COMPLETION_CONSTANT},
    {"it", LSP_COMPLETION_VARIABLE},
};

// The places (KeywordPlace) of the keywords that start a construct in `context`.
static unsigned int keyword_places(LSPContext context)
{
    switch (context)
    {
    case CTX_GLOBAL:
        return KEYWORD_AT_TOP_LEVEL;
    case CTX_LOOP:
        return KEYWORD_IN_BODY | KEYWORD_IN_LOOP;
    case CTX_AFTER_IF:
        return KEYWORD_IN_BODY | KEYWORD_AFTER_IF;
    default:
        return KEYWORD_IN_BODY;
    }
}

// What a bracket open before the cursor encloses, for read_cursor_position.
typedef enum
{
    OPENER_BODY,          // `{` of a block
    OPENER_IMPL,          // `{` of an impl or a trait
    OPENER_LITERAL,       // `{` after a name: a struct literal (`Point {`) or a block (`if done {`)
    OPENER_FIELDS,        // `{` of a struct or a union
    OPENER_VARIANTS,      // `{` of an enum
    OPENER_PARENTHESES,   // `(` of a call or an expression
    OPENER_PARAMETERS,    // `(` of the parameters of a function or a lambda
    OPENER_PAYLOAD,       // `(` of the payload of an enum variant
    OPENER_BRACKETS,      // `[` of an index or an array literal
    OPENER_TYPE_BRACKETS, // `[` of a slice type (`[u8]`)
    OPENER_TYPE_ARGUMENTS, // `<` of the arguments of a generic type (`Vec<`)
} Opener;

// A bracket open before the cursor.
typedef struct
{
    Opener opener;
    Token name;         // For a literal, the name before its `{`; for an impl, its `impl`
    const char *inside; // Where what it encloses starts
} OpenBracket;

// What the declaration being read in a brace has had since its `{`, or since the last `;` or `}`
// in it: the bracket that comes next opens fields, variants, parameters or an impl's body.
typedef struct
{
    int has_struct;
    int has_enum;
    int has_fn;
    int has_parameters;
    Token impl; // The `impl` or `trait` it starts with; TOK_EOF if none
} DeclarationSoFar;

#define CURSOR_READER_MAX_DEPTH 64

static int is_word(Token t, const char *word)
{
    return t.kind == TOK_IDENT && t.len == strlen(word) && strncmp(t.start, word, t.len) == 0;
}

static int is_brace_opener(Opener opener)
{
    return opener == OPENER_BODY || opener == OPENER_IMPL || opener == OPENER_LITERAL ||
           opener == OPENER_FIELDS || opener == OPENER_VARIANTS;
}

static int is_paren_opener(Opener opener)
{
    return opener == OPENER_PARENTHESES || opener == OPENER_PARAMETERS || opener == OPENER_PAYLOAD;
}

// The `:` after `name` (the token `before`, with `before_name` ahead of it) gives its type: in
// `let`, `def`, `static` and `const`, a parameter or a field.
static int colon_gives_type(Token before_name, Token before, Opener innermost)
{
    if (before.kind != TOK_IDENT)
    {
        return 0;
    }
    if (before_name.kind == TOK_DEF || is_word(before_name, "let") ||
        is_word(before_name, "static") || is_word(before_name, "const"))
    {
        return 1;
    }
    if (innermost == OPENER_PARAMETERS)
    {
        return before_name.kind == TOK_LPAREN || before_name.kind == TOK_COMMA;
    }
    return innermost == OPENER_FIELDS &&
           (before_name.kind == TOK_LBRACE || before_name.kind == TOK_SEMICOLON ||
            before_name.kind == TOK_COMMA);
}

// What the `(` after `before` opens. After a name, a `)`, a `]` or a `>` it is a call (whose
// arguments may be named, `f(a: 1)`); elsewhere, the parameters of a lambda or an expression.
static Opener paren_opener(Token before, Opener innermost, DeclarationSoFar *declaration)
{
    if (innermost == OPENER_VARIANTS && before.kind == TOK_IDENT)
    {
        return OPENER_PAYLOAD;
    }
    if (declaration->has_fn && !declaration->has_parameters)
    {
        declaration->has_parameters = 1;
        return OPENER_PARAMETERS;
    }
    int after_callee = before.kind == TOK_IDENT || before.kind == TOK_RPAREN ||
                       before.kind == TOK_RBRACKET || before.kind == TOK_RANGLE;
    return after_callee ? OPENER_PARENTHESES : OPENER_PARAMETERS;
}

// The state of read_cursor_position while it reads the tokens before the cursor.
typedef struct
{
    OpenBracket brackets[CURSOR_READER_MAX_DEPTH];
    int open;
    DeclarationSoFar declarations[CURSOR_READER_MAX_DEPTH];
    int braces;
    Token before_previous;
    Token before;
    Opener closed;        // What the last `)` closed
    int type_next;        // A type can start at the next token
    int previous_is_type; // The previous token is a type's name
} CursorReader;

static Opener innermost_opener(const CursorReader *r)
{
    return r->open ? r->brackets[r->open - 1].opener : OPENER_BODY;
}

// Reads a `(`, `[`, `{`, or the `<` of a generic type; returns 0 for any other token.
static int read_opener(CursorReader *r, Token t, int type_here, int after_type)
{
    DeclarationSoFar *declaration = &r->declarations[r->braces];
    Opener opener;
    Token name = r->before;
    switch (t.kind)
    {
    case TOK_LANGLE:
        if (!after_type)
        {
            return 1; // A comparison
        }
        opener = OPENER_TYPE_ARGUMENTS;
        r->type_next = 1;
        break;
    case TOK_LBRACKET:
        opener = type_here ? OPENER_TYPE_BRACKETS : OPENER_BRACKETS;
        r->type_next = type_here;
        break;
    case TOK_LPAREN:
        opener = paren_opener(r->before, innermost_opener(r), declaration);
        r->type_next = opener == OPENER_PAYLOAD;
        break;
    case TOK_LBRACE:
        // After a type (`-> Point {`) or in an impl (`impl Point {`), the name is no literal's.
        opener = declaration->has_struct                      ? OPENER_FIELDS
                 : declaration->has_enum                      ? OPENER_VARIANTS
                 : declaration->impl.kind != TOK_EOF          ? OPENER_IMPL
                 : r->before.kind == TOK_IDENT && !after_type ? OPENER_LITERAL
                                                              : OPENER_BODY;
        if (opener == OPENER_IMPL)
        {
            name = declaration->impl;
        }
        r->declarations[++r->braces] = (DeclarationSoFar){0};
        break;
    default:
        return 0;
    }
    r->brackets[r->open++] = (OpenBracket){opener, name, t.start + t.len};
    return 1;
}

// Reads a `)`, `]`, `}`, or the `>` (or `>>`) of generic types; returns 0 for any other token.
// A closer that does not match the innermost bracket is left as it is.
static int read_closer(CursorReader *r, Token t)
{
    Opener innermost = innermost_opener(r);
    switch (t.kind)
    {
    case TOK_RANGLE:
        r->open -= innermost == OPENER_TYPE_ARGUMENTS;
        return 1;
    case TOK_OP:
        if (t.len != 2 || t.start[0] != '>' || t.start[1] != '>')
        {
            return 0;
        }
        for (int i = 0; i < 2 && innermost_opener(r) == OPENER_TYPE_ARGUMENTS && r->open; i++)
        {
            r->open--;
        }
        return 1;
    case TOK_RBRACKET:
        r->open -= innermost == OPENER_BRACKETS || innermost == OPENER_TYPE_BRACKETS;
        return 1;
    case TOK_RPAREN:
        if (is_paren_opener(innermost) && r->open)
        {
            r->closed = innermost;
            r->open--;
        }
        return 1;
    case TOK_RBRACE:
        // Brackets left open while typing close with their brace.
        while (r->open && !is_brace_opener(r->brackets[r->open - 1].opener))
        {
            r->open--;
        }
        r->open -= r->open > 0;
        r->braces -= r->braces > 0;
        r->declarations[r->braces] = (DeclarationSoFar){0};
        return 1;
    default:
        return 0;
    }
}

// Reads a token that is no bracket: a word, `:`, `->`, `=`, `,` or `;`.
static void read_other(CursorReader *r, Token t, int type_here)
{
    DeclarationSoFar *declaration = &r->declarations[r->braces];
    Opener innermost = innermost_opener(r);
    if (t.kind == TOK_UNION || is_word(t, "struct"))
    {
        declaration->has_struct = 1;
    }
    else if (is_word(t, "enum"))
    {
        declaration->has_enum = 1;
    }
    else if (t.kind == TOK_IMPL || t.kind == TOK_TRAIT)
    {
        declaration->impl = t;
    }
    else if (is_word(t, "fn"))
    {
        // A function starts here, also after a header left unfinished above.
        declaration->has_fn = 1;
        declaration->has_parameters = 0;
    }
    else if (t.kind == TOK_IDENT)
    {
        // `const` in a type (`const u8`) leaves room for the type after it.
        r->type_next = type_here && is_word(t, "const");
        r->previous_is_type = type_here && !r->type_next;
    }
    else if (t.kind == TOK_COLON)
    {
        r->type_next = colon_gives_type(r->before_previous, r->before, innermost);
    }
    else if (t.kind == TOK_ARROW)
    {
        r->type_next = r->before.kind == TOK_RPAREN && r->closed == OPENER_PARAMETERS;
    }
    else if (t.kind == TOK_OP && t.len == 1 && t.start[0] == '=')
    {
        r->type_next = r->before.kind == TOK_IDENT && r->before_previous.kind == TOK_ALIAS;
    }
    else if (t.kind == TOK_COMMA)
    {
        r->type_next = innermost == OPENER_TYPE_ARGUMENTS || innermost == OPENER_PAYLOAD;
    }
    else if (t.kind == TOK_SEMICOLON)
    {
        *declaration = (DeclarationSoFar){0};
    }
}

// Where the word being typed goes, as the tokens before it say (see read_cursor_position).
typedef struct
{
    int is_type;
    // In a struct literal where a field's name goes (after its `{` or a `,`): the name before the
    // `{`, which may be a struct's, and where its fields start. `literal.kind` is TOK_EOF
    // elsewhere.
    Token literal;
    const char *literal_fields;
    // The impl or trait in whose header, or in whose body between its members, the cursor is: its
    // `impl` or `trait`. `impl.kind` is TOK_EOF elsewhere.
    Token impl;
    // In that body, where a member starts: after its `{`, a `}` or a `;`, or after a `fn` there
    // (`after_fn`).
    int member_start;
    int after_fn;
} CursorPosition;

// Where the word being typed at `offset` of `source` goes. A type goes after the `:` of a `let`, a
// `def`, a parameter or a field, after the `->` of a function's parameters, on the right of
// `alias Name =`, in a variant's payload, after `const` in a type, and inside the `<...>` and
// `[...]` of a type. A field's name goes after the `{` or a `,` of a literal. A member of an impl
// goes after its `{`, a `}` or a `;`, and after the `fn` that starts one. Reads the tokens
// before the word as the lexer gives them, so it works on text that does not parse yet.
static CursorPosition read_cursor_position(const char *source, size_t offset, const char *path)
{
    CursorPosition position = {0};
    CursorReader r = {.closed = OPENER_PARENTHESES};
    Lexer l;
    lexer_init(&l, source, g_project->ctx->config, path);
    l.end = source + offset;
    for (Token t = lexer_next(&l); t.kind != TOK_EOF; t = lexer_next(&l))
    {
        if (r.open == CURSOR_READER_MAX_DEPTH || r.braces == CURSOR_READER_MAX_DEPTH - 1)
        {
            return position; // Deeper than any program written by hand: not worth tracking
        }
        int type_here = r.type_next;
        int after_type = r.previous_is_type;
        r.type_next = 0;
        r.previous_is_type = 0;
        if (!read_opener(&r, t, type_here, after_type) && !read_closer(&r, t))
        {
            read_other(&r, t, type_here);
        }
        r.before_previous = r.before;
        r.before = t;
    }
    position.is_type = r.type_next;
    if (innermost_opener(&r) == OPENER_LITERAL &&
        (r.before.kind == TOK_LBRACE || r.before.kind == TOK_COMMA))
    {
        position.literal = r.brackets[r.open - 1].name;
        position.literal_fields = r.brackets[r.open - 1].inside;
    }
    if (r.declarations[r.braces].impl.kind != TOK_EOF)
    {
        position.impl = r.declarations[r.braces].impl; // In the header
    }
    else if (innermost_opener(&r) == OPENER_IMPL)
    {
        position.impl = r.brackets[r.open - 1].name;
        position.after_fn = is_word(r.before, "fn");
        Token member_start = position.after_fn ? r.before_previous : r.before;
        position.member_start = member_start.kind == TOK_LBRACE ||
                                member_start.kind == TOK_RBRACE ||
                                member_start.kind == TOK_SEMICOLON;
    }
    return position;
}

// Whether the fields of a literal, from `fields` up to `end`, give a value to `name`.
static int literal_has_field(const char *fields, const char *end, const char *name,
                             const char *path)
{
    Lexer l;
    lexer_init(&l, fields, g_project->ctx->config, path);
    l.end = end;
    int depth = 0;
    Token before = {.kind = TOK_COMMA}; // The literal's `{` reads like a `,`
    Token previous = {0};
    for (Token t = lexer_next(&l); t.kind != TOK_EOF; t = lexer_next(&l))
    {
        if (t.kind == TOK_LPAREN || t.kind == TOK_LBRACE || t.kind == TOK_LBRACKET)
        {
            depth++;
        }
        else if (t.kind == TOK_RPAREN || t.kind == TOK_RBRACE || t.kind == TOK_RBRACKET)
        {
            depth--;
        }
        else if (depth == 0 && t.kind == TOK_COLON && previous.kind == TOK_IDENT &&
                 before.kind == TOK_COMMA && previous.len == strlen(name) &&
                 strncmp(previous.start, name, previous.len) == 0)
        {
            return 1;
        }
        before = previous.kind == TOK_EOF ? before : previous;
        previous = t;
    }
    return 0;
}

// Adds the fields of the struct named `position.literal` that its literal does not give yet, up to
// `end`, as `name: `. Returns 0, adding nothing, when the name is no struct's (`if done {`).
static int add_literal_field_completions(cJSON *items, CursorPosition position, const char *end,
                                         const char *path)
{
    char name[MAX_VAR_NAME_LEN];
    if (position.literal.len >= sizeof(name))
    {
        return 0;
    }
    memcpy(name, position.literal.start, position.literal.len);
    name[position.literal.len] = 0;
    ASTNode *strct = find_declaration(name, NODE_STRUCT);
    if (!strct)
    {
        return 0;
    }
    for (ASTNode *field = strct->strct.fields; field; field = field->next)
    {
        if (literal_has_field(position.literal_fields, end, field->field.name, path))
        {
            continue;
        }
        char insert[MAX_VAR_NAME_LEN + 3];
        snprintf(insert, sizeof(insert), "%s: ", field->field.name);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", field->field.name);
        cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_FIELD);
        cJSON_AddStringToObject(item, "detail",
                                zen_c_type_spelling(g_project->ctx, field->type_info));
        cJSON_AddStringToObject(item, "insertText", insert);
        cJSON_AddItemToArray(items, item);
    }
    return 1;
}

// The token after the `<...>` that starts at `t`, a `<`, read from `l`; the `{` or the end of
// the file where it is left open.
static Token after_angle_brackets(Lexer *l, Token t)
{
    int depth = 0;
    do
    {
        if (t.kind == TOK_LANGLE)
        {
            depth++;
        }
        else if (t.kind == TOK_RANGLE)
        {
            depth--;
        }
        else if (t.kind == TOK_OP && t.len == 2 && t.start[0] == '>' && t.start[1] == '>')
        {
            depth -= 2;
        }
        else if (t.kind == TOK_LBRACE || t.kind == TOK_EOF)
        {
            return t;
        }
        t = lexer_next(l);
    } while (depth > 0);
    return t;
}

// The name of the trait that the impl starting at `impl` implements, as written in its header
// (`Show` of `impl<T> Show for Box<T> {`). Returns 0 for an impl of a type (`impl Box {`) and for
// a trait's declaration.
static int implemented_trait_name(Token impl, char *out, size_t size, const char *path)
{
    if (impl.kind != TOK_IMPL)
    {
        return 0;
    }
    Lexer l;
    lexer_init(&l, impl.start + impl.len, g_project->ctx->config, path);
    Token name = lexer_next(&l);
    if (name.kind == TOK_LANGLE)
    {
        name = after_angle_brackets(&l, name);
    }
    Token t = lexer_next(&l);
    if (t.kind == TOK_LANGLE)
    {
        t = after_angle_brackets(&l, t);
    }
    if (name.kind != TOK_IDENT || name.len >= size || !is_word(t, "for"))
    {
        return 0;
    }
    memcpy(out, name.start, name.len);
    out[name.len] = 0;
    return 1;
}

// The trait that the impl starting at `impl` implements, as the project declares it; NULL for an
// impl of a type, a trait's declaration, and a trait that the project does not declare.
static ASTNode *implemented_trait(Token impl, const char *path)
{
    char name[MAX_VAR_NAME_LEN];
    return implemented_trait_name(impl, name, sizeof(name), path)
               ? find_declaration(name, NODE_TRAIT)
               : NULL;
}

// Where the members of the impl starting at `impl` start: after the `{` of its header. NULL when
// the header has no `{` yet.
static const char *impl_members(Token impl, const char *path)
{
    Lexer l;
    lexer_init(&l, impl.start + impl.len, g_project->ctx->config, path);
    for (Token t = lexer_next(&l); t.kind != TOK_EOF; t = lexer_next(&l))
    {
        if (t.kind == TOK_LBRACE)
        {
            return t.start + t.len;
        }
        if (t.kind == TOK_SEMICOLON || t.kind == TOK_RBRACE)
        {
            return NULL;
        }
    }
    return NULL;
}

// The `}` that closes the impl whose members start at `members`; NULL when it is left open.
static const char *impl_end(const char *members, const char *path)
{
    Lexer l;
    lexer_init(&l, members, g_project->ctx->config, path);
    int depth = 0;
    for (Token t = lexer_next(&l); t.kind != TOK_EOF; t = lexer_next(&l))
    {
        if (t.kind == TOK_LPAREN || t.kind == TOK_LBRACE || t.kind == TOK_LBRACKET)
        {
            depth++;
        }
        else if ((t.kind == TOK_RPAREN || t.kind == TOK_RBRACE || t.kind == TOK_RBRACKET) &&
                 depth-- == 0)
        {
            return t.kind == TOK_RBRACE ? t.start : NULL;
        }
    }
    return NULL;
}

// Writes to `out` the signature of `method`, a method of a trait, as the trait writes it (see
// lsp_function_signature). Returns 0 when it has none.
static int trait_method_signature(ASTNode *method, char *out, size_t size, const char *path)
{
    if (method->kind != NODE_FUNCTION || !method->func.method_name || !method->token.start)
    {
        return 0;
    }
    // The token of a trait's method is its `fn`: the signature starts at the name after it.
    Lexer l;
    lexer_init(&l, method->token.start + method->token.len, g_project->ctx->config, path);
    return lsp_function_signature(lexer_next(&l), out, size);
}

// Whether the members of an impl, from `members` up to its `}`, define a method named `name`. The
// name being typed at `word` defines none yet.
static int impl_has_method(const char *members, const char *word, const char *name,
                           const char *path)
{
    Lexer l;
    lexer_init(&l, members, g_project->ctx->config, path);
    int depth = 0;
    Token previous = {0};
    for (Token t = lexer_next(&l); t.kind != TOK_EOF; t = lexer_next(&l))
    {
        if (t.kind == TOK_LPAREN || t.kind == TOK_LBRACE || t.kind == TOK_LBRACKET)
        {
            depth++;
        }
        else if (t.kind == TOK_RPAREN || t.kind == TOK_RBRACE || t.kind == TOK_RBRACKET)
        {
            if (depth-- == 0)
            {
                return 0; // The impl's `}`
            }
        }
        else if (depth == 0 && is_word(previous, "fn") && t.start != word && is_word(t, name))
        {
            return 1;
        }
        previous = t;
    }
    return 0;
}

// Writes `text` to `out` so that a snippet inserts it as it is: `$`, `}` and `\` escaped.
static void snippet_text(const char *text, char *out, size_t size)
{
    size_t used = 0;
    for (; *text && used + 2 < size; text++)
    {
        if (*text == '$' || *text == '}' || *text == '\\')
        {
            out[used++] = '\\';
        }
        out[used++] = *text;
    }
    out[used] = 0;
}

// Adds the methods of the trait that the impl of `position` implements and that its members do not
// define yet, inserted with the signature written in the trait and an empty body (without `fn`
// when it is written already). Returns 0, adding nothing, when the impl is no impl of a trait.
static int add_trait_member_completions(cJSON *items, CursorPosition position, const char *word,
                                        const char *path)
{
    ASTNode *trait = implemented_trait(position.impl, path);
    const char *members = trait ? impl_members(position.impl, path) : NULL;
    if (!members)
    {
        return 0;
    }
    for (ASTNode *method = trait->trait.methods; method; method = method->next)
    {
        const char *method_name = method->func.method_name;
        char signature[MAX_SHORT_MSG_LEN * 4];
        if (!trait_method_signature(method, signature, sizeof(signature), path) ||
            has_completion(items, method_name) || impl_has_method(members, word, method_name, path))
        {
            continue;
        }
        char escaped[sizeof(signature) * 2];
        snippet_text(position.after_fn ? signature + strlen("fn ") : signature, escaped,
                     sizeof(escaped));
        char insert[sizeof(escaped) + 16];
        snprintf(insert, sizeof(insert), "%s {\n    $0\n}", escaped);
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", method_name);
        cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_METHOD);
        cJSON_AddStringToObject(item, "detail", signature);
        cJSON_AddStringToObject(item, "insertText", insert);
        cJSON_AddNumberToObject(item, "insertTextFormat", LSP_INSERT_SNIPPET);
        cJSON_AddItemToArray(items, item);
    }
    return 1;
}

// Adds the names of the primitive types (`u8`, `usize`, `string`...), as primitives.c defines them.
static void add_primitive_completions(cJSON *items)
{
    int type_count = 0;
    const ZenPrimitive *types = get_zen_primitives(&type_count);
    for (int i = 0; i < type_count; i++)
    {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "label", types[i].zen_name);
        cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_KEYWORD);
        cJSON_AddStringToObject(item, "sortText", "008");
        cJSON_AddItemToArray(items, item);
    }
}

// Adds the completion item of a declaration that names a type (see add_declaration_completion).
static int add_type_declaration_completion(ASTNode *node, void *data)
{
    int names_type = node->kind == NODE_STRUCT || node->kind == NODE_ENUM ||
                     node->kind == NODE_TRAIT || node->kind == NODE_TYPE_ALIAS;
    return names_type ? add_declaration_completion(node, data) : 0;
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
                    ASTNode *enm = find_declaration(parts[0], NODE_ENUM);
                    for (ASTNode *v = enm ? enm->enm.variants : NULL; v; v = v->next)
                    {
                        cJSON *item = cJSON_CreateObject();
                        cJSON_AddStringToObject(item, "label", v->variant.name);
                        cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_ENUM_MEMBER);
                        cJSON_AddStringToObject(item, "detail", "enum variant");
                        cJSON_AddItemToArray(items, item);
                    }
                    add_impl_completions(items, parts[0], 0);
                }

                if (!is_scoped && part_count > 0)
                {
                    char *base_name = parts[0];
                    // The type of the base as the parser declared it.
                    Type *type = resolve_local_type(target_func, base_name);
                    if (!type)
                    {
                        ASTNode *decl = lsp_find_local_decl(target_func, base_name);
                        if (decl && decl->kind == NODE_VAR_DECL && decl->type_info)
                        {
                            type = decl->type_info;
                        }
                        else
                        {
                            ZenSymbol *sym = find_symbol_in_all(g_project->ctx, base_name);
                            type = sym ? sym->type_info : NULL;
                        }
                    }

                    // A struct of a C header is no type of the program: it goes by its name.
                    char c_struct[MAX_VAR_NAME_LEN] = "";
                    for (int p = 1; p < part_count && (type || c_struct[0]); p++)
                    {
                        if (type)
                        {
                            ASTNode *def =
                                member_definition(type, pf->path, c_struct, sizeof(c_struct));
                            type = NULL;
                            for (ASTNode *field =
                                     def && def->kind == NODE_STRUCT ? def->strct.fields : NULL;
                                 field; field = field->next)
                            {
                                if (strcmp(field->field.name, parts[p]) == 0)
                                {
                                    type = field->type_info;
                                    break;
                                }
                            }
                            if (!c_struct[0])
                            {
                                continue;
                            }
                        }
                        char c_type[MAX_SHORT_MSG_LEN];
                        if (!lsp_c_headers_field_type(pf->path, pf->source, c_struct, parts[p],
                                                      c_type, sizeof(c_type)))
                        {
                            c_struct[0] = 0;
                            break;
                        }
                        c_struct_name(c_type, c_struct, sizeof(c_struct));
                    }

                    if (type)
                    {
                        ASTNode *def =
                            member_definition(type, pf->path, c_struct, sizeof(c_struct));
                        if (def && def->kind == NODE_STRUCT)
                        {
                            for (ASTNode *field = def->strct.fields; field; field = field->next)
                            {
                                cJSON *item = cJSON_CreateObject();
                                cJSON_AddStringToObject(item, "label", field->field.name);
                                cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_FIELD);
                                cJSON_AddStringToObject(
                                    item, "detail",
                                    zen_c_type_spelling(g_project->ctx, field->type_info));
                                cJSON_AddItemToArray(items, item);
                            }
                        }
                        else if (def && def->kind == NODE_ENUM)
                        {
                            for (ASTNode *variant = def->enm.variants; variant;
                                 variant = variant->next)
                            {
                                cJSON *item = cJSON_CreateObject();
                                cJSON_AddStringToObject(item, "label", variant->variant.name);
                                cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_ENUM_MEMBER);
                                cJSON_AddItemToArray(items, item);
                            }
                        }
                        else if (def && def->kind == NODE_TRAIT)
                        {
                            add_trait_methods(items, def);
                        }
                        if (!c_struct[0])
                        {
                            char *impl_name = type_impl_name(member_owner(type));
                            if (impl_name)
                            {
                                add_impl_completions(items, impl_name, 1);
                            }
                        }
                    }
                    if (c_struct[0])
                    {
                        // A struct from a C header: its fields as the header declares them.
                        lsp_c_headers_add_fields(pf->path, pf->source, c_struct, items);
                        add_impl_completions(items, c_struct, 1);
                    }
                }
                zfree(copy);
            }
        }
    }

    // Where a type goes (`size: u`, `-> u`, `alias Word = u`), only the names of types fit; where
    // a field of a struct literal goes (`Point { x: 1, `), only the fields it does not give yet;
    // where a method of an impl of a trait goes, only the trait's methods it does not define yet.
    CursorPosition position = {0};
    const char *word = NULL;
    if (!dot_completed && pf->source)
    {
        word = ptr + (at < line_len ? at : line_len);
        position = read_cursor_position(pf->source, (size_t)(word - pf->source), pf->path);
    }
    if (position.is_type)
    {
        add_primitive_completions(items);
        visit_declarations(add_type_declaration_completion, items);
    }
    else if (position.literal.kind == TOK_IDENT &&
             add_literal_field_completions(items, position, word, pf->path))
    {
        // The fields of the literal, and nothing else
    }
    else if (position.member_start && add_trait_member_completions(items, position, word, pf->path))
    {
        // The methods of the trait that the impl does not define yet, and nothing else
    }
    else if (!dot_completed)
    {
        // The keywords that can start a construct where the cursor is: with the snippets of
        // KEYWORD_SNIPPETS, or as they are.
        unsigned int places = keyword_places(context);
        int keyword_count = 0;
        const ZenKeyword *keywords = zen_keywords(&keyword_count);
        for (int i = 0; i < keyword_count; i++)
        {
            if (!(keywords[i].places & places))
            {
                continue;
            }
            int snippets = 0;
            for (size_t k = 0; k < sizeof(KEYWORD_SNIPPETS) / sizeof(KEYWORD_SNIPPETS[0]); k++)
            {
                if (strcmp(KEYWORD_SNIPPETS[k].keyword, keywords[i].spelling) != 0)
                {
                    continue;
                }
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "label", KEYWORD_SNIPPETS[k].label);
                cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_KEYWORD);
                cJSON_AddStringToObject(item, "insertText", KEYWORD_SNIPPETS[k].snippet);
                cJSON_AddNumberToObject(item, "insertTextFormat", LSP_INSERT_SNIPPET);
                cJSON_AddStringToObject(item, "sortText", "001"); // Keywords highest priority
                cJSON_AddItemToArray(items, item);
                snippets++;
            }
            if (!snippets)
            {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "label", keywords[i].spelling);
                cJSON_AddNumberToObject(item, "kind", LSP_COMPLETION_KEYWORD);
                cJSON_AddStringToObject(item, "sortText", "008");
                cJSON_AddItemToArray(items, item);
            }
        }

        if (context != CTX_GLOBAL)
        {
            add_primitive_completions(items);
        }

        for (size_t i = 0; i < sizeof(BUILTINS) / sizeof(BUILTINS[0]) && context != CTX_GLOBAL; i++)
        {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "label", BUILTINS[i].name);
            cJSON_AddNumberToObject(item, "kind", BUILTINS[i].kind);
            cJSON_AddStringToObject(item, "sortText", "008");
            cJSON_AddItemToArray(items, item);
        }

        // The declarations of the files of the project, as their last parse left them: a
        // declaration that was deleted is not offered.
        visit_declarations(add_declaration_completion, items);

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
    ASTNode *node = lsp_project_top_level(pf);
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
        ASTNode *node = pf->is_external ? NULL : lsp_project_top_level(pf);
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
                // The function as the files of the project declare it now.
                ASTNode *fn = find_declaration(func_name, NODE_FUNCTION);
                // The signature as written, as the hover shows it
                char label[MAX_SHORT_MSG_LEN * 4];
                if (fn && lsp_function_signature(fn->token, label, sizeof(label)))
                {
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

// The byte of `source` at (line, col) of the protocol, or the end of that line when `col` is past
// it.
static const char *source_at(const char *source, int line, int col)
{
    const char *p = source;
    for (int l = 0; *p && l < line; p++)
    {
        l += *p == '\n';
    }
    for (int c = 0; *p && *p != '\n' && c < col; c++)
    {
        p++;
    }
    return p;
}

// Adds to `range` the protocol's position of `at`, a byte of `source`, under `key`.
static void add_position(cJSON *range, const char *key, const char *source, const char *at)
{
    int line = 0;
    const char *line_start = source;
    for (const char *p = source; p < at; p++)
    {
        if (*p == '\n')
        {
            line++;
            line_start = p + 1;
        }
    }
    cJSON *position = cJSON_AddObjectToObject(range, key);
    cJSON_AddNumberToObject(position, "line", line);
    cJSON_AddNumberToObject(position, "character", (double)(at - line_start));
}

// The code action that writes in the impl starting at `impl` the methods of its trait that it does
// not define, with the signature written in the trait and an empty body, after its last member and
// one level deeper than the impl. NULL when it is no impl of a trait, it is left open or it lacks
// no method.
static cJSON *implement_trait_action(const char *uri, const char *source, Token impl,
                                     const char *path)
{
    ASTNode *trait = implemented_trait(impl, path);
    const char *members = trait ? impl_members(impl, path) : NULL;
    const char *end = members ? impl_end(members, path) : NULL;
    if (!end)
    {
        return NULL;
    }
    const char *line_start = impl.start;
    while (line_start > source && line_start[-1] != '\n')
    {
        line_start--;
    }
    int indent = 0;
    while (line_start[indent] == ' ' || line_start[indent] == '\t')
    {
        indent++;
    }
    // The blank between the last member, or the `{`, and the `}` is replaced.
    const char *after_last = end;
    while (after_last > members && isspace((unsigned char)after_last[-1]))
    {
        after_last--;
    }
    Emitter e;
    emitter_init_buffer(&e);
    int added = 0;
    for (ASTNode *method = trait->trait.methods; method; method = method->next)
    {
        char signature[MAX_SHORT_MSG_LEN * 4];
        if (!trait_method_signature(method, signature, sizeof(signature), path) ||
            impl_has_method(members, NULL, method->func.method_name, path))
        {
            continue;
        }
        emitter_printf(&e, "%s\n%.*s    %s {\n%.*s    }", added || after_last > members ? "\n" : "",
                       indent, line_start, signature, indent, line_start);
        added++;
    }
    if (!added)
    {
        return NULL;
    }
    emitter_printf(&e, "\n%.*s", indent, line_start);

    cJSON *edit = cJSON_CreateObject();
    cJSON *range = cJSON_AddObjectToObject(edit, "range");
    add_position(range, "start", source, after_last);
    add_position(range, "end", source, end);
    cJSON_AddStringToObject(edit, "newText", emitter_take_string(&e));
    cJSON *edits = cJSON_CreateArray();
    cJSON_AddItemToArray(edits, edit);

    cJSON *action = cJSON_CreateObject();
    char title[MAX_SHORT_MSG_LEN];
    snprintf(title, sizeof(title), "Implement the missing methods of %s", trait->trait.name);
    cJSON_AddStringToObject(action, "title", title);
    cJSON_AddStringToObject(action, "kind", "quickfix");
    cJSON *changes = cJSON_AddObjectToObject(cJSON_AddObjectToObject(action, "edit"), "changes");
    cJSON_AddItemToObject(changes, uri, edits);
    return action;
}

cJSON *lsp_code_actions(const char *uri, int line, int col)
{
    cJSON *actions = cJSON_CreateArray();
    ProjectFile *pf = lsp_project_get_file(uri);
    if (!g_project || !g_project->ctx || !pf || !pf->source)
    {
        return actions;
    }
    const char *at = source_at(pf->source, line, col);
    CursorPosition position = read_cursor_position(pf->source, (size_t)(at - pf->source), pf->path);
    cJSON *implement = position.impl.kind == TOK_IMPL
                           ? implement_trait_action(uri, pf->source, position.impl, pf->path)
                           : NULL;
    if (implement)
    {
        cJSON_AddItemToArray(actions, implement);
    }
    return actions;
}
