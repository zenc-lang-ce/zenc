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
 * @brief A range in the source code mapping to semantic info.
 */
typedef struct LSPRange
{
    int start_line;   ///< Start line (1-based).
    int start_col;    ///< Start column (1-based).
    int end_line;     ///< End line.
    int end_col;      ///< End column (approximated).
    RangeType kind;   ///< Type of range (def or ref).
    int def_line;     ///< Line of definition (if reference).
    int def_col;      ///< Column of definition (if reference).
    char *hover_text; ///< Tooltip text / signature.
    ASTNode *node;    ///< Associated AST node.
    struct LSPRange *next;
} LSPRange;

/**
 * @brief Index of a single file.
 */
typedef struct LSPIndex
{
    LSPRange *head; ///< First range in the file.
    LSPRange *tail; ///< Last range in the file.
    /// The nodes that hold the names of the types the file refers to (see lsp_index_add_type_ref),
    /// one per name, linked by `next`.
    ASTNode *type_names;
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
void lsp_index_add_def(LSPIndex *idx, Token t, const char *hover, ASTNode *node);
void lsp_index_add_def_range(LSPIndex *idx, Token start, Token end, const char *hover,
                             ASTNode *node);
void lsp_index_add_ref(LSPIndex *idx, Token t, Token def_t, ASTNode *node);
/**
 * @brief Adds the use of a type name as a reference by name: its definition is looked up, in any
 * file of the project, when it is asked for. The reference carries a node that only holds the
 * name, as the node of a variable's reference does.
 */
void lsp_index_add_type_ref(LSPIndex *idx, Token t, const char *name);
LSPRange *lsp_find_at(LSPIndex *idx, int line, int col);

/**
 * @brief Writes "fn " and a function's declaration as written in the source, from its name
 * token up to the body. Returns 0 when the token is not the name in a source.
 */
int lsp_function_signature(Token name, char *out, size_t size);

// Walker.
/**
 * @brief Indexes the nodes of a file parsed from `source`. Code that the parser generates from a
 * text of its own (`@derive`, `comptime`) has tokens with the file's name but not its positions:
 * the ranges of a token that is not written at its position in `source` are left out.
 */
void lsp_build_index(ParserContext *ctx, LSPIndex *idx, ASTNode *root, const char *source);

#endif
