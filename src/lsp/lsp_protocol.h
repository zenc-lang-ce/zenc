// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "lsp/lsp_protocol.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef LSP_PROTOCOL_H
#define LSP_PROTOCOL_H

// The numbers the Language Server Protocol gives its constants, named as its specification names
// them. The server writes them in its messages; nothing else should know them.

/** Severity of a published diagnostic (DiagnosticSeverity). */
typedef enum
{
    LSP_SEVERITY_ERROR = 1,
    LSP_SEVERITY_WARNING = 2,
    LSP_SEVERITY_INFORMATION = 3,
    LSP_SEVERITY_HINT = 4
} LspDiagnosticSeverity;

/** What a completion item is (CompletionItemKind): the icon the client shows beside it. */
typedef enum
{
    LSP_COMPLETION_METHOD = 2,
    LSP_COMPLETION_FUNCTION = 3,
    LSP_COMPLETION_FIELD = 5,
    LSP_COMPLETION_VARIABLE = 6,
    LSP_COMPLETION_INTERFACE = 8,
    LSP_COMPLETION_ENUM = 13,
    LSP_COMPLETION_KEYWORD = 14,
    LSP_COMPLETION_ENUM_MEMBER = 20,
    LSP_COMPLETION_CONSTANT = 21,
    LSP_COMPLETION_STRUCT = 22,
    LSP_COMPLETION_TYPE_PARAMETER = 25
} LspCompletionItemKind;

/** How the client reads a completion item's insertText (InsertTextFormat). */
typedef enum
{
    LSP_INSERT_SNIPPET = 2 ///< With tab stops: `${1:name}`, `$0`.
} LspInsertTextFormat;

/**
 * What a symbol of the outline or of workspace/symbol is (SymbolKind). Not numbered as
 * LspCompletionItemKind: an enum member is 22 here and 20 there.
 */
typedef enum
{
    LSP_SYMBOL_METHOD = 6,
    LSP_SYMBOL_FIELD = 8,
    LSP_SYMBOL_ENUM = 10,
    LSP_SYMBOL_INTERFACE = 11,
    LSP_SYMBOL_FUNCTION = 12,
    LSP_SYMBOL_VARIABLE = 13,
    LSP_SYMBOL_CONSTANT = 14,
    LSP_SYMBOL_STRUCT = 23
} LspSymbolKind;

#endif
