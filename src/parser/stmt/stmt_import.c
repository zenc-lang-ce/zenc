// SPDX-License-Identifier: MIT

#include "parser.h"
#include "constants.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ast/ast.h"
#include "utils/format_expr.h"
#include "plugins/plugin_manager.h"
#include "zen/zen_facts.h"
#include "zprep_plugin.h"
#include "analysis/move_check.h"
#include "utils/c_headers.h"

// Registers what a C header included with `include "x.h"` declares, so the program can use
// it: its functions and variables, its struct and union tags (`struct tag` in C), the names
// its typedefs give a struct or union, its constants, and the headers it includes with "...".
static void scan_c_header_contents(ParserContext *ctx, const char *path, int depth)
{
    if (depth > C_HEADERS_MAX_INCLUDE_DEPTH || is_file_imported(ctx, path))
    {
        return;
    }
    mark_file_imported(ctx, path);

    char *resolved = z_resolve_path(path, ctx->current_filename, ctx->config);
    CHeaderFile *header = resolved ? c_headers_get(resolved) : NULL;
    if (!header || !header->text)
    {
        return;
    }

    register_c_header_types(ctx, header);
    for (int i = 0; i < header->symbol_count; i++)
    {
        const CHeaderEntry *entry = &header->symbols[i];
        if (entry->kind == C_SYMBOL_FUNCTION || entry->kind == C_SYMBOL_VARIABLE)
        {
            register_extern_symbol(ctx, entry->name);
        }
        else if (entry->kind == C_SYMBOL_CONSTANT)
        {
            // The `#define` line, with its continuations.
            int from = header->line_offsets[entry->symbol.start_line];
            int to = entry->symbol.end_line + 1 < header->line_count
                         ? header->line_offsets[entry->symbol.end_line + 1]
                         : (int)strlen(header->text);
            char *line = xmalloc((size_t)(to - from + 1));
            memcpy(line, header->text + from, (size_t)(to - from));
            line[to - from] = 0;
            try_parse_macro_const(ctx, line);
        }
    }

    // The names its typedefs give a struct or union, then the tags of those that have none of
    // the same name (`typedef struct Point {...} Point;` is used as `Point`). A tag is written
    // `struct tag` (or `union tag`) in C; a typedef name, alone.
    for (int pass = 0; pass < 2; pass++)
    {
        for (int i = 0; i < header->symbol_count; i++)
        {
            const CHeaderEntry *entry = &header->symbols[i];
            int is_tag = entry->tag == C_TAG_STRUCT || entry->tag == C_TAG_UNION;
            if (entry->kind != C_SYMBOL_STRUCT || is_tag != pass ||
                (is_tag && find_type_alias_node(ctx, entry->name)))
            {
                continue;
            }
            Type *c_type = type_new(TYPE_STRUCT);
            c_type->name = xstrdup(entry->name);
            c_type->is_explicit_struct = is_tag;
            c_type->is_union = entry->tag == C_TAG_UNION;
            register_type_alias(ctx, entry->name, c_type, 1, NULL, TOKEN_UNKNOWN, 0);
            register_extern_symbol(ctx, entry->name);
        }
    }

    // Headers included with "..." are looked up next to this one, as the C preprocessor does.
    char header_dir[MAX_PATH_LEN];
    c_headers_directory_of(path, header_dir, sizeof(header_dir));
    for (int i = 0; i < header->include_count; i++)
    {
        if (header->includes[i].is_system)
        {
            continue;
        }
        char nested_path[MAX_PATH_LEN * 2];
        if (header_dir[0])
        {
            snprintf(nested_path, sizeof(nested_path), "%s/%s", header_dir,
                     header->includes[i].name);
        }
        else
        {
            snprintf(nested_path, sizeof(nested_path), "%s", header->includes[i].name);
        }
        scan_c_header_contents(ctx, nested_path, depth + 1);
    }
}

ASTNode *parse_include(ParserContext *ctx, Lexer *l)
{
    lexer_next(l);
    Token t = lexer_next(l);
    char *path = NULL;
    int is_system = 0;

    if (t.kind == TOK_LANGLE)
    {
        is_system = 1;
        char buf[MAX_SHORT_MSG_LEN];
        size_t used = 0;
        buf[0] = 0;
        while (1)
        {
            // The path is written on one line: a `>` that is missing (`include <std`, while it is
            // being typed) must not swallow the rest of the file into the path.
            Token i = lexer_peek(l);
            if (i.kind == TOK_EOF || i.line != t.line)
            {
                zpanic_at(t, "Unclosed include path, expected '>'");
                break;
            }
            lexer_next(l);
            if (i.kind == TOK_RANGLE)
            {
                break;
            }
            if (i.len >= sizeof(buf) - used)
            {
                zpanic_at(t, "Include path is too long");
                break;
            }
            memcpy(buf + used, i.start, i.len);
            used += i.len;
            buf[used] = 0;
        }
        path = xstrdup(buf);

        ctx->has_external_includes = 1;
        string_list_add(&ctx->c_system_includes, path);
    }
    else
    {
        is_system = 0;
        path = token_get_string_content(t);
    }

    ASTNode *n = ast_create(NODE_INCLUDE);
    n->include.path = path;
    n->include.is_system = is_system;

    if (!is_system && path)
    {
        scan_c_header_contents(ctx, path, 0);
    }

    return n;
}

ASTNode *parse_import(ParserContext *ctx, Lexer *l, int is_re_export)
{
    lexer_next(l);

    Token next = lexer_peek(l);
    if (next.kind == TOK_IDENT && next.len == 6 && strncmp(next.start, "plugin", 6) == 0)
    {
        lexer_next(l);

        Token plugin_tok = lexer_next(l);
        if (plugin_tok.kind != TOK_STRING)
        {
            zpanic_at(plugin_tok, "Expected string literal after 'import plugin'");
            return NULL;
        }

        char *plugin_name = token_get_string_content(plugin_tok);

        if (plugin_name[0] == '.' &&
            (plugin_name[1] == '/' || (plugin_name[1] == '.' && plugin_name[2] == '/')))
        {
            char *current_dir = xstrdup(ctx->current_filename);
            char *last_slash = z_path_last_sep(current_dir);
            if (last_slash)
            {
                *last_slash = 0;
                char resolved_path[MAX_PATH_LEN];
                snprintf(resolved_path, sizeof(resolved_path), "%s/%s", current_dir, plugin_name);
                zfree(plugin_name);
                plugin_name = xstrdup(resolved_path);
            }
            zfree(current_dir);
        }

        char *alias = NULL;
        Token as_tok = lexer_peek(l);
        if (as_tok.kind == TOK_IDENT && as_tok.len == 2 && strncmp(as_tok.start, "as", 2) == 0)
        {
            lexer_next(l);
            Token alias_tok = lexer_next(l);
            if (alias_tok.kind != TOK_IDENT)
            {
                zpanic_at(alias_tok, "Expected identifier after 'as'");
                return NULL;
            }
            alias = token_strdup(alias_tok);
        }

#if ZC_HAS_PLUGINS
        register_plugin(ctx, plugin_name, alias);
#endif

        if (lexer_peek(l).kind == TOK_SEMICOLON)
        {
            lexer_next(l);
        }

        return NULL;
    }

    int is_selective = 0;
    zvec_Str symbols = {0};
    zvec_Str aliases = {0};

    if (lexer_peek(l).kind == TOK_LBRACE)
    {
        is_selective = 1;
        lexer_next(l);

        while (1)
        {
            if (lexer_peek(l).kind == TOK_RBRACE)
            {
                break;
            }
            if (lexer_peek(l).kind == TOK_EOF)
            {
                zpanic_at(lexer_peek(l), "Unexpected end of file in selective import");
                break;
            }
            if (symbols.length > 0 && lexer_peek(l).kind == TOK_COMMA)
            {
                lexer_next(l);
            }

            Token sym_tok = lexer_next(l);
            if (sym_tok.kind != TOK_IDENT)
            {
                zpanic_at(sym_tok, "Expected identifier in selective import");
                for (size_t _c = 0; _c < symbols.length; _c++)
                {
                    zfree(symbols.data[_c]);
                    if (aliases.data[_c])
                    {
                        zfree(aliases.data[_c]);
                    }
                }
                zvec_free_Str(&symbols);
                zvec_free_Str(&aliases);
                return NULL;
            }

            char *sym = xmalloc(sym_tok.len + 1);
            strncpy(sym, sym_tok.start, sym_tok.len);
            sym[sym_tok.len] = 0;
            zvec_push_Str(&symbols, sym);

            Token inner_next = lexer_peek(l);
            if (inner_next.kind == TOK_IDENT && inner_next.len == 2 &&
                strncmp(inner_next.start, "as", 2) == 0)
            {
                lexer_next(l);
                Token alias_tok = lexer_next(l);
                if (alias_tok.kind != TOK_IDENT)
                {
                    zpanic_at(alias_tok, "Expected identifier after 'as'");
                    for (size_t _c = 0; _c < symbols.length; _c++)
                    {
                        zfree(symbols.data[_c]);
                        if (aliases.data[_c])
                        {
                            zfree(aliases.data[_c]);
                        }
                    }
                    zvec_free_Str(&symbols);
                    zvec_free_Str(&aliases);
                    return NULL;
                }

                char *als = xmalloc(alias_tok.len + 1);
                strncpy(als, alias_tok.start, alias_tok.len);
                als[alias_tok.len] = 0;
                zvec_push_Str(&aliases, als);
            }
            else
            {
                zvec_push_Str(&aliases, NULL);
            }
        }

        lexer_next(l);

        Token from_tok = lexer_next(l);
        if (from_tok.kind != TOK_IDENT || from_tok.len != 4 ||
            strncmp(from_tok.start, "from", 4) != 0)
        {
            zpanic_at(from_tok, "Expected 'from' after selective import list, got type=%d",
                      (int)from_tok.kind);
            for (size_t _c = 0; _c < symbols.length; _c++)
            {
                zfree(symbols.data[_c]);
                if (aliases.data[_c])
                {
                    zfree(aliases.data[_c]);
                }
            }
            zvec_free_Str(&symbols);
            zvec_free_Str(&aliases);
            return NULL;
        }
    }

    Token t = lexer_next(l);
    if (t.kind != TOK_STRING && t.kind != TOK_RAW_STRING)
    {
        zpanic_at(t, "Expected string (filename) after 'from' in selective import, got type %d",
                  (int)t.kind);
        for (size_t _c = 0; _c < symbols.length; _c++)
        {
            zfree(symbols.data[_c]);
            if (aliases.data[_c])
            {
                zfree(aliases.data[_c]);
            }
        }
        zvec_free_Str(&symbols);
        zvec_free_Str(&aliases);
        return NULL;
    }
    char *fn = token_get_string_content(t);

    char *resolved = z_resolve_path(fn, ctx->current_filename, ctx->config);
    if (!resolved)
    {
        if (strlen(fn) > 2 && strcmp(fn + strlen(fn) - 2, ".h") == 0)
        {
            resolved = xstrdup(fn);
        }
        else
        {
            zpanic_at(t, "Could not find module: %s", fn);
            for (size_t _c = 0; _c < symbols.length; _c++)
            {
                zfree(symbols.data[_c]);
                if (aliases.data[_c])
                {
                    zfree(aliases.data[_c]);
                }
            }
            zvec_free_Str(&symbols);
            zvec_free_Str(&aliases);
            zfree(fn);
            return NULL;
        }
    }
    zfree(fn);
    fn = resolved;

    if (!fn)
    {
        ASTNode *dummy = ast_create(NODE_BLOCK);
        dummy->block.statements = NULL;
        return dummy;
    }

    if (is_file_imported(ctx, fn))
    {
        // The file is not read again (a std module may already have brought in the same C
        // header), but `as name` still belongs to this import. A C header's alias is
        // registered: its symbols keep their C names. A module's symbols already carry the
        // prefix of its first import, so its alias cannot be honoured here and is consumed.
        if (!is_selective)
        {
            Token next_tok = lexer_peek(l);
            if (next_tok.kind == TOK_IDENT && next_tok.len == 2 &&
                strncmp(next_tok.start, "as", 2) == 0)
            {
                lexer_next(l);
                Token alias_tok = lexer_next(l);
                int is_header = strlen(fn) > 2 && strcmp(fn + strlen(fn) - 2, ".h") == 0;
                if (is_header && alias_tok.kind == TOK_IDENT)
                {
                    char *alias = token_strdup(alias_tok);
                    if (!zmap_get(&ctx->imports.modules, alias))
                    {
                        Module *m = xmalloc(sizeof(Module));
                        m->alias = alias;
                        m->path = xstrdup(fn);
                        m->base_name = module_prefix_for(ctx, fn);
                        m->is_c_header = 1;
                        m->is_re_export = is_re_export;
                        zmap_put(&ctx->imports.modules, alias, m);
                    }
                }
            }
        }
        for (size_t _c = 0; _c < symbols.length; _c++)
        {
            zfree(symbols.data[_c]);
            if (aliases.data[_c])
            {
                zfree(aliases.data[_c]);
            }
        }
        zvec_free_Str(&symbols);
        zvec_free_Str(&aliases);
        zfree(fn);
        return NULL;
    }

    if (zmap_get(&ctx->imports.currently_parsing, fn))
    {
        zpanic_at(t, "Circular import detected: '%s'", fn);
        for (size_t _c = 0; _c < symbols.length; _c++)
        {
            zfree(symbols.data[_c]);
            if (aliases.data[_c])
            {
                zfree(aliases.data[_c]);
            }
        }
        zvec_free_Str(&symbols);
        zvec_free_Str(&aliases);
        zfree(fn);
        return NULL;
    }
    zmap_put(&ctx->imports.currently_parsing, fn, fn);

    char *module_base_name = NULL;
    if (is_selective)
    {
        module_base_name = module_prefix_for(ctx, fn);
        for (size_t i = 0; i < symbols.length; i++)
        {
            register_selective_import(ctx, symbols.data[i], aliases.data[i], module_base_name);
        }
    }

    char *alias = NULL;
    int is_wildcard = 0;
    if (!is_selective)
    {
        Token next_tok = lexer_peek(l);
        if (next_tok.kind == TOK_IDENT && next_tok.len == 2 &&
            strncmp(next_tok.start, "as", 2) == 0)
        {
            lexer_next(l);
            Token alias_tok = lexer_next(l);
            if (alias_tok.kind != TOK_IDENT && alias_tok.kind != TOK_OP)
            {
                zpanic_at(alias_tok, "Expected identifier after 'as'");
                return NULL;
            }

            if (alias_tok.len == 1 && *alias_tok.start == '*')
            {
                is_wildcard = 1;
            }
            else
            {
                alias = xmalloc(alias_tok.len + 1);
                strncpy(alias, alias_tok.start, alias_tok.len);
                alias[alias_tok.len] = 0;
            }

            if (!is_wildcard)
            {
                int is_header = 0;
                if (strlen(fn) > 2 && strcmp(fn + strlen(fn) - 2, ".h") == 0)
                {
                    is_header = 1;
                }

                if (!zmap_get(&ctx->imports.modules, alias))
                {
                    char *mod_base = module_prefix_for(ctx, fn);
                    Module *m = xmalloc(sizeof(Module));
                    m->alias = xstrdup(alias);
                    m->path = xstrdup(fn);
                    m->base_name = mod_base;
                    m->is_c_header = is_header;
                    m->is_re_export = is_re_export;
                    zmap_put(&ctx->imports.modules, alias, m);
                }
            }
        }
    }

    if (strlen(fn) > 2 && strcmp(fn + strlen(fn) - 2, ".h") == 0)
    {
        // Headers are turned into NODE_INCLUDE without parsing their contents,
        // so they must be released from the cycle-tracking set here (the
        // currently_parsing removal below is only reached for .zc modules).
        // Otherwise a second import of the same header (e.g. via a std module)
        // would be misreported as a circular import.
        zmap_remove(&ctx->imports.currently_parsing, fn);
        // What the header declares is read as `include "x.h"` reads it, so the program can use
        // its types; a header the compiler cannot find (a system header) is left alone.
        scan_c_header_contents(ctx, fn, 0);
        mark_file_imported(ctx, fn);
        ASTNode *n = ast_create(NODE_INCLUDE);
        n->include.path = xstrdup(fn);
        n->include.is_system = 0;
        for (size_t _c = 0; _c < symbols.length; _c++)
        {
            zfree(symbols.data[_c]);
            if (aliases.data[_c])
            {
                zfree(aliases.data[_c]);
            }
        }
        zvec_free_Str(&symbols);
        zvec_free_Str(&aliases);
        return n;
    }

    char *src = load_file(fn, ctx->current_filename, ctx->config);
    if (!src)
    {
        if (!src)
        {
            if (ctx->config->mode_lsp)
            {
                zwarn_at(t, "LSP: Import not found: %s", fn);
                for (size_t _c = 0; _c < symbols.length; _c++)
                {
                    zfree(symbols.data[_c]);
                    if (aliases.data[_c])
                    {
                        zfree(aliases.data[_c]);
                    }
                }
                zvec_free_Str(&symbols);
                zvec_free_Str(&aliases);
                ASTNode *dummy = ast_create(NODE_BLOCK);
                dummy->block.statements = NULL;
                return dummy;
            }
            zpanic_at(t, "Not found: %s", fn);
            return NULL;
        }
    }

    // The module's `//> link:`, `include:`... go to the build, as those of the file it compiles.
    scan_build_directives(ctx, src);

    Lexer i;
    // The imported file's own path: tokens carry it into diagnostics and #line.
    lexer_init(&i, src, ctx->config, fn);

    char *prev_module_prefix = ctx->imports.current_module_prefix;
    char *prev_module_alias = ctx->imports.current_module_alias;
    char *temp_module_prefix = NULL;
    ctx->imports.current_module_alias = alias;

    if (alias)
    {
        temp_module_prefix = module_prefix_for(ctx, fn);
        ctx->imports.current_module_prefix = temp_module_prefix;
    }
    else if (is_selective || is_wildcard)
    {
        temp_module_prefix = module_prefix_for(ctx, fn);
        ctx->imports.current_module_prefix = temp_module_prefix;
    }
    else
    {
        temp_module_prefix = NULL;
        ctx->imports.current_module_prefix = NULL;

        char *mod_base = extract_module_name(fn);
        if (!zmap_get(&ctx->imports.modules, mod_base))
        {
            Module *m = xmalloc(sizeof(Module));
            m->alias = mod_base;
            m->path = xstrdup(fn);
            m->base_name = mod_base;
            m->is_c_header = 0;
            m->is_re_export = is_re_export;
            zmap_put(&ctx->imports.modules, m->alias, m);
        }
    }

    if (is_re_export && alias && prev_module_prefix)
    {
        char *prop_base = module_prefix_for(ctx, fn);
        re_export_propagated(ctx, alias, prev_module_prefix, prop_base);
        zfree(prop_base);
    }

    const char *saved_fn = ctx->current_filename;
    ctx->current_filename = fn;

    ASTNode *import_node = ast_create(NODE_IMPORT);
    skip_comments(&i);
    ATTACH_DOC_COMMENT(ctx, import_node);

    ASTNode *r = parse_program_nodes(ctx, &i);

    zmap_remove(&ctx->imports.currently_parsing, fn);
    mark_file_imported(ctx, fn);

    if (is_wildcard && temp_module_prefix)
    {
        re_export_wildcard_symbols(ctx, temp_module_prefix);
        zmap_put(&ctx->imports.wildcard_imports, temp_module_prefix, temp_module_prefix);
    }

    ctx->current_filename = (char *)saved_fn;

    ctx->imports.current_module_prefix = prev_module_prefix;
    ctx->imports.current_module_alias = prev_module_alias;
    if (temp_module_prefix)
    {
        zfree(temp_module_prefix);
    }

    for (size_t k = 0; k < symbols.length; k++)
    {
        zfree(symbols.data[k]);
        if (aliases.data[k])
        {
            zfree(aliases.data[k]);
        }
    }
    zvec_free_Str(&symbols);
    zvec_free_Str(&aliases);

    if (alias)
    {
        zfree(alias);
    }

    if (module_base_name)
    {
        zfree(module_base_name);
    }

    import_node->token = t;
    import_node->import_stmt.path = xstrdup(fn);
    import_node->import_stmt.module_root = r;

    return import_node;
}
