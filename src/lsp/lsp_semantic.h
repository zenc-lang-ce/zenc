// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "lsp/lsp_semantic.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef LSP_SEMANTIC_H
#define LSP_SEMANTIC_H

struct cJSON;

// Semantic Tokens: the result of textDocument/semanticTokens/full, and the legend its numbers
// stand for, which initialize gives the client.
struct cJSON *lsp_semantic_tokens_full(const char *uri);
struct cJSON *lsp_semantic_tokens_legend(void);

#endif
