// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "ast/ast.h"
#include "analysis/move_check.h"
#include "plugins/plugin_manager.h"
#include "zen/zen_facts.h"
#include "zprep_plugin.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Loads and parses a std module once, guarding against re-import and
// re-entrancy. The caller runs its own "already loaded" check before calling.
void load_std_module(ParserContext *ctx, const char *path)
{
    char *resolved = z_resolve_path(path, ctx->current_filename, ctx->config);
    if (!resolved)
    {
        return; // Could not find the module
    }
    // Check if already imported or currently being parsed
    if (is_file_imported(ctx, resolved))
    {
        zfree(resolved);
        return;
    }
    if (zmap_get(&ctx->imports.currently_parsing, resolved))
    {
        zfree(resolved);
        return;
    }
    zmap_put(&ctx->imports.currently_parsing, resolved, resolved);
    // Load and parse the file
    char *src = load_file(resolved, ctx->current_filename, ctx->config);
    if (!src)
    {
        zmap_remove(&ctx->imports.currently_parsing, resolved);
        zfree(resolved);
        return;
    }
    scan_build_directives(ctx, src);
    Lexer i;
    lexer_init(&i, src, ctx->config, resolved);
    // Save and restore filename context
    const char *saved_fn = ctx->current_filename;
    ctx->current_filename = resolved;
    // Parse the module contents. Codegen emits its declarations from the registries; the
    // typechecker walks them through an import node of their own.
    ASTNode *import = ast_create(NODE_IMPORT);
    import->import_stmt.path = xstrdup(resolved);
    import->import_stmt.module_root = parse_program_nodes(ctx, &i);
    import->next = ctx->implicit_std_imports;
    ctx->implicit_std_imports = import;
    ctx->current_filename = saved_fn;
    zmap_remove(&ctx->imports.currently_parsing, resolved);
    mark_file_imported(ctx, resolved);
    zfree(resolved);
}

// Convenience wrapper kept for the struct_impl call site.
void auto_import_std_mem(ParserContext *ctx)
{
    load_std_module(ctx, "std/mem.zc");
}
// Trait Parsing
