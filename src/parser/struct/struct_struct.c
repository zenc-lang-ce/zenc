// SPDX-License-Identifier: MIT
#include "parser.h"
#include "constants.h"
#include "ast/ast.h"
#include "analysis/move_check.h"
#include "plugins/plugin_manager.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analysis/const_fold.h"
#include "utils/utils.h"
#include "ast/primitives.h"

ASTNode *parse_struct(ParserContext *ctx, Lexer *l, int is_union, int is_opaque, int is_extern,
                      const char *link_name, int is_export)
{

    lexer_next(l); // eat struct or union
    // `register_generic` skips a name that is already a generic parameter: what the struct
    // registers is undone by restoring the count, not by subtracting what it declared.
    int generics_before = ctx->known_generics_count;
    Token n = lexer_next(l);
    check_identifier(n);
    char *name = token_strdup(n);
    Token name_token = n;

    // Capture a pending /// doc comment (written before `struct`) here: the
    // fields are parsed before the struct node is created, so they would
    // otherwise consume the struct's own documentation.
    char *pending_doc = ctx->last_doc_comment;
    ctx->last_doc_comment = NULL;

    // Generic Params <T> or <K, V>
    char **gps = NULL;
    int gp_count = 0;
    if (lexer_peek(l).kind == TOK_LANGLE)
    {
        lexer_next(l); // eat <
        while (1)
        {
            Token g = lexer_next(l);
            check_identifier(g);
            gps = realloc(gps, sizeof(char *) * (size_t)(gp_count + 1));
            gps[gp_count++] = token_strdup(g);
            register_generic(ctx, g);

            Token next = lexer_peek(l);
            if (next.kind == TOK_EOF)
            {
                zpanic_at(next, "Expected '>' in generic parameter list");
                break;
            }
            if (next.kind == TOK_COMMA)
            {
                lexer_next(l); // eat ,
            }
            else if (next.kind == TOK_RANGLE)
            {
                lexer_next(l); // eat >
                break;
            }
            else
            {
                zpanic_at(next, "Expected ',' or '>' in generic parameter list");
                return NULL;
            }
        }
    }

    // Check for prototype (forward declaration)
    if (lexer_peek(l).kind == TOK_SEMICOLON)
    {
        lexer_next(l);
        ASTNode *node = ast_create(NODE_STRUCT);
        node->doc_comment = pending_doc; // struct-level /// doc (captured on entry)
        node->strct.name = name;
        node->link_name = definition_link_name(ctx, link_name, name);
        node->strct.is_template = (gp_count > 0);
        node->strct.generic_params = gps;
        node->strct.generic_param_count = gp_count;
        node->strct.is_union = is_union;
        node->strct.fields = NULL;
        node->strct.is_incomplete = 1;
        node->strct.is_opaque = is_opaque;
        node->strct.is_export = is_export;

        return node;
    }

    lexer_next(l);
    ASTNode *h = 0, *tl = 0;

    // Temp storage for used structs
    char **temp_used_structs = NULL;
    int temp_used_count = 0;
    int temp_used_cap = 0;

    while (1)
    {
        // Fault-tolerant recovery for struct fields
        if (ctx->is_fault_tolerant && ctx->had_error)
        {
            ctx->had_error = 0;
            while (1)
            {
                Token r = lexer_peek(l);
                if (r.kind == TOK_EOF || r.kind == TOK_RBRACE || r.kind == TOK_SEMICOLON)
                {
                    if (r.kind == TOK_SEMICOLON)
                    {
                        lexer_next(l);
                    }
                    break;
                }
                lexer_next(l);
            }
            continue;
        }

        skip_comments(l);
        Token t = lexer_peek(l);

        if (t.kind == TOK_RBRACE)
        {
            lexer_next(l);
            break;
        }
        if (t.kind == TOK_EOF)
        {
            zpanic_at(t, "Unterminated struct body — expected '}'");
            break;
        }
        if (t.kind == TOK_SEMICOLON || t.kind == TOK_COMMA)
        {
            lexer_next(l);
            continue;
        }

        // Handle 'use' (Struct Embedding)
        if (t.kind == TOK_USE)
        {
            lexer_next(l); // eat use

            // Check for named use: use name: Type;
            Token t1 = lexer_peek(l);
            Token t2 = lexer_peek2(l);

            if (t1.kind == TOK_IDENT && t2.kind == TOK_COLON)
            {
                // Named use -> Composition (Add field, don't flatten)
                Token field_name = lexer_next(l);
                check_identifier(field_name);
                lexer_next(l); // eat :
                Type *ft = parse_type_formal(ctx, l);
                if (!ft)
                {
                    return NULL;
                }
                z_parse_expect(l, TOK_SEMICOLON, "Expected ;");

                ASTNode *nf = ast_create(NODE_FIELD);
                ATTACH_DOC_COMMENT(ctx, nf);
                nf->token = field_name;
                nf->field.name = token_strdup(field_name);
                nf->type_info = ft;

                if (!h)
                {
                    h = nf;
                }
                else
                {
                    tl->next = nf;
                }
                tl = nf;
                continue;
            }

            // Normal use -> Mixin (Flatten)
            // Parse the type (e.g. Header<I32>)
            Type *use_type = parse_type_formal(ctx, l);
            if (!use_type)
            {
                return NULL;
            }
            char *use_name = type_mangle_spelling(use_type);

            z_parse_expect(l, TOK_SEMICOLON, "Expected ; after use");

            // Find the definition and COPY fields
            ASTNode *def = find_struct_def(ctx, use_name);
            if (!def && is_known_generic(ctx, use_type->name))
            {
                // Try to force instantiation if not found?
                // For now, rely on parse_type having triggered instantiation.
                char *mangled = type_mangle_spelling(
                    use_type); // This works if type_to_string returns mangled name
                def = find_struct_def(ctx, mangled);
                zfree(mangled);
            }

            if (def && def->kind == NODE_STRUCT)
            {
                if (temp_used_count == temp_used_cap)
                {
                    temp_used_cap = temp_used_cap ? temp_used_cap * 2 : 8;
                    temp_used_structs =
                        xrealloc(temp_used_structs, sizeof(char *) * (size_t)temp_used_cap);
                }
                temp_used_structs[temp_used_count++] = xstrdup(use_name);

                ASTNode *f = def->strct.fields;
                while (f)
                {
                    ASTNode *nf = ast_create(NODE_FIELD);
                    nf->token = f->token; // Declared in the struct it comes from.
                    nf->field.name = xstrdup(f->field.name);
                    nf->type_info = f->type_info;

                    if (!h)
                    {
                        h = nf;
                    }
                    else
                    {
                        tl->next = nf;
                    }
                    tl = nf;
                    f = f->next;
                }
            }
            zfree(use_name);
            continue;
        }

        // `name {` is not a field: an anonymous `struct { }` (reported below).
        if (t.kind == TOK_IDENT && lexer_peek2(l).kind != TOK_LBRACE)
        {
            Token f_name = lexer_next(l);
            check_identifier(f_name);
            z_parse_expect(l, TOK_COLON, "Expected :");
            Type *ft = parse_type_formal(ctx, l);
            if (!ft)
            {
                return NULL;
            }
            ASTNode *f = ast_create(NODE_FIELD);
            ATTACH_DOC_COMMENT(ctx, f);
            f->token = f_name;
            f->field.name = token_strdup(f_name);
            f->type_info = ft;
            f->field.bit_width = 0;

            // Optional bit width: name: type : 3
            if (lexer_peek(l).kind == TOK_COLON)
            {
                lexer_next(l); // eat :
                Token width_tok = lexer_next(l);
                if (width_tok.kind != TOK_INT)
                {
                    zpanic_at(width_tok, "Expected bit width integer");
                    return NULL;
                }
                f->field.bit_width = (int)strtol(token_strdup(width_tok), NULL, 10);
            }

            if (!h)
            {
                h = f;
            }
            else
            {
                tl->next = f;
            }
            tl = f;

            if (lexer_peek(l).kind == TOK_SEMICOLON || lexer_peek(l).kind == TOK_COMMA)
            {
                lexer_next(l);
            }
        }
        else
        {
            report_not_a_field(l, t);
        }
    }

    // Auto-prefix struct name if in module context
    char *written_name = NULL;
    if (ctx->imports.current_module_prefix && gp_count == 0 && !is_extern &&
        !is_extern_symbol(ctx, name))
    { // Don't prefix generic templates
        written_name = module_written_name(ctx, name);
        char *prefixed_name = mangle_module_symbol(ctx->imports.current_module_prefix, name);
        zfree(name);
        name = prefixed_name;
    }

    // Generic templates are registered separately, so a concrete struct and a
    // generic one with the same name (a user `struct Box` and the std's
    // `Box<T>`) would be silently merged: the generic's impls, Drop included,
    // would apply to the concrete struct.
    if (!ctx->config->mode_lsp)
    {
        if (gp_count == 0)
        {
            ASTNode *existing = find_concrete_struct_def(ctx, name);
            if (existing)
            {
                zerror_at(name_token, "Redefinition of %s '%s'", is_union ? "union" : "struct",
                          name);
            }
        }
        GenericTemplate *tpl = ctx->templates;
        while (tpl && strcmp(tpl->name, name) != 0)
        {
            tpl = tpl->next;
        }
        ASTNode *concrete = gp_count > 0 ? find_concrete_struct_def(ctx, name) : NULL;
        if ((gp_count == 0 && tpl) || concrete)
        {
            ASTNode *other = tpl ? tpl->struct_node : concrete;
            const char *where =
                (other && other->strct.defined_in_file) ? other->strct.defined_in_file : "?";
            char msg[MAX_SHORT_MSG_LEN];
            snprintf(msg, sizeof(msg), "Struct '%s' clashes with the %s struct '%s' from %s", name,
                     gp_count == 0 ? "generic" : "concrete", name, where);
            const char *hints[] = {"A generic and a concrete struct cannot share a name",
                                   "Rename one of them (std types such as Box, Vec, Option, "
                                   "Map or String keep their names)",
                                   NULL};
            zpanic_with_hints(name_token, msg, hints);
        }
    }

    ASTNode *node = ast_create(NODE_STRUCT);
    node->token = name_token;
    node->link_name = definition_link_name(ctx, link_name, name);
    add_to_struct_list(ctx, node);
    node->doc_comment = pending_doc; // struct-level /// doc (captured on entry)

    node->strct.name = name;
    node->strct.written_name = written_name;

    // Initialize Type Info so we can track traits (like Drop)
    node->type_info = type_new(TYPE_STRUCT);
    node->type_info->name = xstrdup(name);
    if (gp_count > 0)
    {
        node->type_info->kind = TYPE_GENERIC;
        node->type_info->count = gp_count;
        node->type_info->args = xmalloc(sizeof(Type *) * (size_t)(gp_count));
        for (int i = 0; i < gp_count; i++)
        {
            node->type_info->args[i] = type_new(TYPE_GENERIC);
            node->type_info->args[i]->name = xstrdup(gps[i]);
        }
    }

    node->strct.fields = h;
    node->strct.generic_params = gps;
    node->strct.generic_param_count = gp_count;
    node->strct.is_union = is_union;
    node->strct.is_opaque = is_opaque;
    node->strct.is_export = is_export;
    node->strct.used_structs = temp_used_structs;
    node->strct.used_struct_count = temp_used_count;
    node->strct.defined_in_file = ctx->current_filename ? xstrdup(ctx->current_filename) : NULL;

    if (gp_count > 0)
    {
        node->strct.is_template = 1;
        ctx->known_generics_count = generics_before;
        register_template(ctx, name, node);
    }

    // Register definition for 'use' lookups and LSP
    if (gp_count == 0)
    {
        register_struct_def(ctx, name, node);
    }

    return node;
}

Type *parse_type_obj(ParserContext *ctx, Lexer *l)
{
    // Parse the base type (int, U32, MyStruct, etc.)
    Type *t = parse_type_base(ctx, l);

    // Handle Pointers
    while (lexer_peek(l).kind == TOK_OP && lexer_peek(l).start[0] == '*')
    {
        lexer_next(l); // eat *
        // Wrap the current type in a Pointer type
        Type *ptr = type_new(TYPE_POINTER);
        ptr->inner = t;
        t = ptr;
    }

    return t;
}
