// SPDX-License-Identifier: MIT
// Parsing Zen C source from a unit test.
#ifndef UNIT_PARSE_H
#define UNIT_PARSE_H

#include "parser/parser.h"

// Parses `src` as a single file with the LSP's settings (fault tolerant, built-ins registered),
// with warnings off and no import paths, and returns its AST. Each call starts from a fresh
// parser context.
ASTNode *parse_source(const char *src);

// parse_source keeping the comments, as zc-doc does: doc comments are attached to their
// declarations (node->doc_comment).
ASTNode *parse_source_keeping_comments(const char *src);

// The parser context of the last parse_source call, to query its registries.
ParserContext *parse_source_context(void);

#endif
