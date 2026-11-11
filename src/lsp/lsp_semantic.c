// SPDX-License-Identifier: MIT
#include "../constants.h"
#include "../ast/primitives.h"
#include "lsp_project.h"
#include "../utils/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The legend that initialize gives the client: a token's type is its index in
// SEMANTIC_TOKEN_TYPES, and each of its modifiers a bit by its index in SEMANTIC_TOKEN_MODIFIERS.
typedef enum
{
    TOKEN_TYPE_VARIABLE,
    TOKEN_TYPE_FUNCTION,
    TOKEN_TYPE_STRUCT,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_TYPE,
    TOKEN_TYPE_ENUM,
    TOKEN_TYPE_MEMBER,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_PARAMETER,
    TOKEN_TYPE_MACRO,
    TOKEN_TYPE_TYPE_PARAMETER,
    TOKEN_TYPE_COUNT
} SemanticTokenType;

static const char *const SEMANTIC_TOKEN_TYPES[TOKEN_TYPE_COUNT] = {
    "variable", "function", "struct", "keyword",  "string",    "number", "comment",
    "type",     "enum",     "member", "operator", "parameter", "macro",  "typeParameter"};

typedef enum
{
    TOKEN_MODIFIER_DECLARATION,
    TOKEN_MODIFIER_DEFINITION,
    TOKEN_MODIFIER_READONLY,
    TOKEN_MODIFIER_STATIC,
    TOKEN_MODIFIER_DEPRECATED,
    TOKEN_MODIFIER_ABSTRACT,
    TOKEN_MODIFIER_ASYNC,
    TOKEN_MODIFIER_MODIFICATION,
    TOKEN_MODIFIER_DOCUMENTATION,
    TOKEN_MODIFIER_DEFAULT_LIBRARY,
    TOKEN_MODIFIER_COUNT
} SemanticTokenModifier;

static const char *const SEMANTIC_TOKEN_MODIFIERS[TOKEN_MODIFIER_COUNT] = {
    "declaration", "definition", "readonly",     "static",        "deprecated",
    "abstract",    "async",      "modification", "documentation", "defaultLibrary"};

// The bit of a modifier in a token's set of modifiers.
#define MODIFIER_BIT(modifier) (1 << (modifier))

cJSON *lsp_semantic_tokens_legend(void)
{
    cJSON *legend = cJSON_CreateObject();
    cJSON_AddItemToObject(legend, "tokenTypes",
                          cJSON_CreateStringArray(SEMANTIC_TOKEN_TYPES, TOKEN_TYPE_COUNT));
    cJSON_AddItemToObject(legend, "tokenModifiers",
                          cJSON_CreateStringArray(SEMANTIC_TOKEN_MODIFIERS, TOKEN_MODIFIER_COUNT));
    return legend;
}

typedef struct
{
    int line;
    int col;
    int length;
    int token_type;
    int token_modifiers;
    int order; ///< How many were pushed before it: of two at one position, the first is kept.
} SemanticToken;

typedef struct
{
    SemanticToken *tokens;
    int count;
    int capacity;
} TokenBuilder;

static void builder_init(TokenBuilder *b)
{
    b->count = 0;
    b->capacity = 4096;
    b->tokens = malloc(sizeof(SemanticToken) * (size_t)(b->capacity));
}

static void builder_push(TokenBuilder *b, int line, int col, int length, int type, int modifiers)
{
    if (line < 0 || col < 0)
    {
        return;
    }
    if (b->count >= b->capacity)
    {
        b->capacity *= 2;
        SemanticToken *new_tokens =
            realloc(b->tokens, sizeof(SemanticToken) * (size_t)(b->capacity));
        if (!new_tokens)
        {
            return;
        }
        b->tokens = new_tokens;
    }
    SemanticToken *t = &b->tokens[b->count++];
    t->line = line;
    t->col = col;
    t->length = length;
    t->token_type = type;
    t->token_modifiers = modifiers;
    t->order = b->count - 1;
}

static void builder_free(TokenBuilder *b)
{
    (void)b;
    zfree(b->tokens);
}

static int compare_tokens(const void *a, const void *b)
{
    const SemanticToken *ta = (const SemanticToken *)a;
    const SemanticToken *tb = (const SemanticToken *)b;
    if (ta->line != tb->line)
    {
        return ta->line - tb->line;
    }
    if (ta->col != tb->col)
    {
        return ta->col - tb->col;
    }
    return ta->order - tb->order;
}

// Keyword check: reuse parser's is_reserved_keyword() + primitives table.
static int is_keyword(const char *w, size_t len)
{
    Token t = {.kind = TOK_IDENT, .start = w, .len = len};
    if (is_reserved_keyword(t))
    {
        return 1;
    }

    static const char *more[] = {
        "and",      "asm",      "alias",  "assert", "comptime", "def",      "defer",    "eprint",
        "eprintln", "expect",   "extern", "false",  "import",   "inline",   "match",    "module",
        "noreturn", "not",      "null",   "opaque", "or",       "print",    "printf",   "println",
        "private",  "public",   "sizeof", "switch", "test",     "true",     "typeof",   "union",
        "use",      "volatile", "impl",   "trait",  "in",       "external", "continue", NULL};
    for (size_t i = 0; more[i]; i++)
    {
        size_t klen = strlen(more[i]);
        if (len == klen && strncmp(w, more[i], len) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int is_type_name(const char *w, size_t len)
{
    int count;
    const ZenPrimitive *prims = get_zen_primitives(&count);
    for (int i = 0; i < count; i++)
    {
        size_t klen = strlen(prims[i].zen_name);
        if (len == klen && strncmp(w, prims[i].zen_name, len) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// Keywords and built-in type names, which are not nodes of their own in the AST. They come from
// the compiler's lexer, so a word inside a string or a comment, part of that token, is left out.
static void emit_keywords(TokenBuilder *b, const char *source, const char *path)
{
    if (!source || !g_project || !g_project->ctx)
    {
        return;
    }
    Lexer l;
    lexer_init(&l, source, g_project->ctx->config, path);
    for (Token t = lexer_next(&l); t.kind != TOK_EOF; t = lexer_next(&l))
    {
        int token_type = -1;
        if (is_type_name(t.start, t.len))
        {
            token_type = TOKEN_TYPE_TYPE;
        }
        else if (is_keyword(t.start, t.len))
        {
            token_type = TOKEN_TYPE_KEYWORD;
        }
        if (token_type >= 0)
        {
            // Tokens count lines and columns from 1, the LSP from 0
            builder_push(b, t.line - 1, t.col - 1, (int)t.len, token_type, 0);
        }
    }
}

// The type of the token that a node's own token gets, and its modifiers; -1 for a node whose
// token is not one (a statement, an operator).
static int node_token_type(const ASTNode *node, int *modifiers)
{
    *modifiers = 0;
    switch (node->kind)
    {
    case NODE_FUNCTION:
        *modifiers = MODIFIER_BIT(TOKEN_MODIFIER_DECLARATION);
        return TOKEN_TYPE_FUNCTION;
    case NODE_CONST:
        *modifiers = MODIFIER_BIT(TOKEN_MODIFIER_READONLY);
        return TOKEN_TYPE_VARIABLE;
    case NODE_VAR_DECL:
    case NODE_EXPR_VAR:
        return TOKEN_TYPE_VARIABLE;
    case NODE_TYPE_ALIAS:
        return TOKEN_TYPE_TYPE;
    case NODE_STRUCT:
    case NODE_TRAIT:
    case NODE_IMPL:
    case NODE_IMPL_TRAIT:
        return TOKEN_TYPE_STRUCT;
    case NODE_FIELD:
    case NODE_EXPR_MEMBER:
        return TOKEN_TYPE_MEMBER;
    case NODE_ENUM:
    case NODE_ENUM_VARIANT:
        return TOKEN_TYPE_ENUM;
    case NODE_EXPR_LITERAL:
        if (node->literal.kind == LITERAL_STRING || node->literal.kind == LITERAL_CHAR)
        {
            return TOKEN_TYPE_STRING;
        }
        if (node->literal.kind == LITERAL_INT || node->literal.kind == LITERAL_FLOAT)
        {
            return TOKEN_TYPE_NUMBER;
        }
        return -1;
    default:
        return -1;
    }
}

// Whether a token of the source can have a semantic token of this type. A node that the parser
// generates may carry a token that is not its own (the `{` of the body of a `for ... in`), and a
// `def` folded into a literal keeps its name.
static int token_fits(int type, Token t)
{
    switch (type)
    {
    case TOKEN_TYPE_NUMBER:
        return t.kind == TOK_INT || t.kind == TOK_FLOAT;
    case TOKEN_TYPE_STRING:
        return t.kind == TOK_STRING || t.kind == TOK_FSTRING || t.kind == TOK_RAW_STRING ||
               t.kind == TOK_CHAR;
    case TOKEN_TYPE_MEMBER:
        // A field of a tuple is its index: `pair.0`.
        return t.kind == TOK_IDENT || t.kind == TOK_INT;
    default:
        return t.kind == TOK_IDENT;
    }
}

// The tokens of the nodes of the list starting at `node`, and of their children. Only where the
// token is written in the file of `idx`: not in the code the parser generates (`@derive`).
static void traverse_node(TokenBuilder *b, const LSPIndex *idx, ASTNode *node, int depth)
{
    if (depth > 32)
    {
        return;
    }
    for (; node; node = node->next)
    {
        int modifiers = 0;
        int type = node_token_type(node, &modifiers);
        if (type >= 0 && token_fits(type, node->token) && lsp_index_is_written(idx, node->token))
        {
            builder_push(b, node->token.line - 1, node->token.col - 1, (int)(node->token.len), type,
                         modifiers);
        }

        ASTChildren children = ast_children(node);
        for (int i = 0; i < children.count; i++)
        {
            traverse_node(b, idx, *children.slots[i], depth + 1);
        }
        // The expressions of an f-string are not children (ast_children() leaves them out).
        if (node->kind == NODE_EXPR_FSTRING)
        {
            for (ASTNode *part = node->fstring.parts; part; part = part->next)
            {
                traverse_node(b, idx, part->fstring_part.expr, depth + 1);
            }
        }
    }
}

cJSON *lsp_semantic_tokens_full(const char *uri)
{
    cJSON *root_json = cJSON_CreateObject();
    cJSON *data = cJSON_AddArrayToObject(root_json, "data");
    ProjectFile *pf = lsp_project_get_file(uri);
    if (!pf || !pf->ast || !pf->index)
    {
        return root_json;
    }

    TokenBuilder b;
    builder_init(&b);

    // Emit keyword tokens from source (keywords not captured by AST traversal)
    emit_keywords(&b, pf->source, pf->path);

    traverse_node(&b, pf->index, pf->ast, 0);

    qsort(b.tokens, (size_t)(b.count), sizeof(SemanticToken), compare_tokens);

    int prev_line = 0;
    int prev_col = 0;

    for (int i = 0; i < b.count; i++)
    {
        SemanticToken t = b.tokens[i];

        if (i > 0 && t.line == b.tokens[i - 1].line && t.col == b.tokens[i - 1].col)
        {
            continue;
        }

        int delta_line = t.line - prev_line;
        int delta_col = (delta_line == 0) ? (t.col - prev_col) : t.col;

        cJSON_AddItemToArray(data, cJSON_CreateNumber(delta_line));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(delta_col));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(t.length));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(t.token_type));
        cJSON_AddItemToArray(data, cJSON_CreateNumber(t.token_modifiers));

        prev_line = t.line;
        prev_col = t.col;
    }

    builder_free(&b);
    return root_json;
}
