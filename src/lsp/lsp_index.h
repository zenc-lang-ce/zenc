// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "lsp/lsp_index.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef LSP_INDEX_H
#define LSP_INDEX_H

#include "parser.h"

/**
 * @brief Type of an indexed AST range.
 */
typedef enum
{
    RANGE_DEFINITION, ///< Defines a symbol.
    RANGE_REFERENCE   ///< References a symbol.
} RangeType;

/**
 * @brief What a definition declares.
 */
typedef enum
{
    DECLARATION_NONE, ///< A reference, whose kind is its definition's, or a range of no symbol.
    DECLARATION_FUNCTION,
    DECLARATION_VARIABLE,
    DECLARATION_CONSTANT,
    DECLARATION_PARAMETER,
    DECLARATION_RECEIVER, ///< `self`: the receiver of a method is that name (see
                          ///< parse_and_convert_args).
    DECLARATION_STRUCT,
    DECLARATION_FIELD,
    DECLARATION_ENUM,
    DECLARATION_VARIANT,
    DECLARATION_ALIAS,
    DECLARATION_TRAIT
} DeclarationKind;

/**
 * @brief The symbol a range names.
 *
 * A symbol is identified by its definition: two ranges name the same symbol when they have the
 * same one, whatever their names. A reference's definition is the declaration the parser resolved
 * it to through the scopes, so a local is not another of the same name.
 */
typedef struct LSPSymbol
{
    char *name;           ///< The name as the source writes it at the range.
    DeclarationKind kind; ///< What it declares, on a definition.
    /// The name token of the declaration (its file, 1-based line and column, length): the range's
    /// own token on a definition.
    Token definition;
} LSPSymbol;

/**
 * @brief A range in the source code mapping to semantic info.
 */
typedef struct LSPRange
{
    int start_line;   ///< Start line (0-based).
    int start_col;    ///< Start column (0-based).
    int end_line;     ///< End line.
    int end_col;      ///< End column (approximated).
    RangeType kind;   ///< Type of range (def or ref).
    LSPSymbol symbol; ///< What it names; no name for a range of no symbol (a plugin's block).
    char *hover_text; ///< Tooltip text / signature, on a definition.
    ASTNode *node;    ///< Associated AST node, where a request needs it.
    struct LSPRange *next;
} LSPRange;

/**
 * @brief Index of a single file.
 */
typedef struct LSPIndex
{
    LSPRange *head; ///< First range in the file.
    LSPRange *tail; ///< Last range in the file.
    /// The text of the file and where each of its lines starts: a range is only added where its
    /// token is written (see lsp_build_index).
    const char *source;
    size_t source_len;
    size_t *line_starts;
    int line_count;
} LSPIndex;

// API.
LSPIndex *lsp_index_new(void);
void lsp_index_free(LSPIndex *idx);
void lsp_index_add_def(LSPIndex *idx, Token t, DeclarationKind kind, const char *hover,
                       ASTNode *node);
void lsp_index_add_def_range(LSPIndex *idx, Token start, Token end, const char *hover,
                             ASTNode *node);
void lsp_index_add_ref(LSPIndex *idx, Token t, Token def_t, ASTNode *node);
/**
 * @brief Adds the use of a type name, a reference to the declaration that `ctx` has registered
 * for that name (a struct, an enum, an alias, a trait or a generic template).
 */
void lsp_index_add_type_ref(LSPIndex *idx, ParserContext *ctx, Token t, const char *name);
LSPRange *lsp_find_at(LSPIndex *idx, int line, int col);

/**
 * @brief Whether `r` names the symbol declared at `definition`: it is that definition or a
 * reference to it.
 */
int lsp_range_names(const LSPRange *r, Token definition);

/**
 * @brief Writes "fn " and a function's declaration as written in the source, from its name
 * token up to the body. Returns 0 when the token is not the name in a source.
 */
int lsp_function_signature(Token name, char *out, size_t size);

// Walker.
/**
 * @brief Indexes the nodes of a file that `ctx` parsed from `source`. Code that the parser
 * generates from a text of its own (`@derive`, `comptime`) has tokens with the file's name but not
 * its positions: the ranges of a token that is not written at its position in `source` are left
 * out.
 */
void lsp_build_index(ParserContext *ctx, LSPIndex *idx, ASTNode *root, const char *source);

#endif
