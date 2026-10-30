// SPDX-License-Identifier: MIT
#ifndef LSP_MAIN_H
#define LSP_MAIN_H

struct ZenCompiler;

// Entry point for the standalone `zc-lsp` binary (JSON-RPC over stdio). The server
// compiles with `compiler` for its whole lifetime.
int lsp_main(struct ZenCompiler *compiler, int argc, char **argv);

// The editor that started the server has exited: its parent changed. Long work that reads no
// messages, like indexing the workspace, checks it so that an abandoned server does not keep
// running until it reads the end of its input. Always 0 on Windows.
int lsp_client_gone(void);

#endif // LSP_MAIN_H
