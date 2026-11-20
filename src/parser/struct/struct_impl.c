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

// The methods of an impl body, after its `{`, up to and including its `}`. Each method belongs
// to the impl's target (impl_target), and to `trait_name` in a trait impl; the impl registers
// them (collect_declaration).
static ASTNode *parse_impl_methods(ParserContext *ctx, Lexer *l, const char *trait_name)
{
    ASTNode *h = NULL;
    ASTNode *tl = NULL;
    // While a method is read, the ones before it are its siblings (find_func); after the
    // impl, none are.
    ASTNode *saved_impl_methods = ctx->current_impl_methods;
    while (1)
    {
        ctx->current_impl_methods = h;
        skip_comments(l);
        if (lexer_peek(l).kind == TOK_RBRACE)
        {
            lexer_next(l);
            break;
        }
        if (lexer_peek(l).kind == TOK_EOF)
        {
            zpanic_at(lexer_peek(l), "Unexpected end of file in impl body");
            break;
        }
        DeclarationAttributes attrs = {0};
        if (lexer_peek(l).kind == TOK_AT)
        {
            attrs = parse_attributes(ctx, l);
        }

        if (lexer_peek(l).kind == TOK_IDENT && lexer_peek(l).len == 5 &&
            strncmp(lexer_peek(l).start, "async", 5) == 0)
        {
            zerror_at(lexer_peek(l), "'async' was removed; call the method directly");
            lexer_next(l);
            continue;
        }
        if (lexer_peek(l).kind != TOK_IDENT || strncmp(lexer_peek(l).start, "fn", 2) != 0)
        {
            lexer_next(l);
            continue;
        }

        ASTNode *f = parse_function(ctx, l, 0, attrs.link_name, 0);
        if (!f)
        {
            continue;
        }
        ATTACH_DOC_COMMENT(ctx, f);
        f->func.is_method = 1;
        f->func.trait_name = trait_name;

        if (attrs.cfg_condition)
        {
            f->cfg_condition = attrs.cfg_condition;
        }
        f->func.pure = attrs.is_pure;
        f->func.unused = attrs.is_unused;
        f->link_name = attrs.link_name;

        if (!h)
        {
            h = f;
        }
        else
        {
            tl->next = f;
        }
        tl = f;
    }
    ctx->current_impl_methods = saved_impl_methods;
    return h;
}

// The type an impl adds methods to: a primitive (`impl f64`), the struct or enum of that name
// (a template by its name alone), or the type an alias stands for (a plain alias the type it
// names, an opaque alias itself).
static Type *impl_target(ParserContext *ctx, const char *name)
{
    if (is_primitive_type_name(name))
    {
        return type_new(get_primitive_type_kind(name));
    }
    ASTNode *def = find_type_alias_node(ctx, name) ? NULL : find_struct_def(ctx, name);
    int is_template = def && ((def->kind == NODE_STRUCT && def->strct.is_template) ||
                              (def->kind == NODE_ENUM && def->enm.is_template));
    return def && def->type_info && !is_template ? def->type_info : type_from_name(ctx, name);
}

ASTNode *parse_impl(ParserContext *ctx, Lexer *l)
{

    lexer_next(l); // eat impl

    // `register_generic` skips a name that is already a generic parameter, as the `T` of
    // `impl<T> Show for Box<T>` is the second time: what the impl registers is undone by
    // restoring the count, not by subtracting what it declared.
    int generics_before = ctx->known_generics_count;

    // Handle impl<T> Struct<T> syntax: generic param declared before struct name
    char *gen_param = NULL;
    if (lexer_peek(l).kind == TOK_LANGLE)
    {
        lexer_next(l); // eat <
        Token gt = lexer_next(l);
        gen_param = token_strdup(gt);
        if (lexer_next(l).kind != TOK_RANGLE)
        {
            zpanic_at(lexer_peek(l), "Expected >");
            return NULL;
        }
        register_generic(ctx, gt);
    }

    Token t1 = lexer_next(l);
    char *name1 = token_strdup(t1);

    // Map primitive types to their C representation for correct mangling
    // Normalize type name (e.g. int -> int32_t)
    const char *normalized = normalize_type_name(name1);
    char *final_name = xstrdup(normalized);
    zfree(name1);
    name1 = final_name;

    // Check for <T> on the struct name
    if (!gen_param && lexer_peek(l).kind == TOK_LANGLE)
    {
        // impl Struct<T> syntax: parse and register generic param
        lexer_next(l); // eat <
        Token gt = lexer_next(l);
        gen_param = token_strdup(gt);
        if (lexer_next(l).kind != TOK_RANGLE)
        {
            zpanic_at(lexer_peek(l), "Expected >");
            return NULL;
        }
        register_generic(ctx, gt);
    }
    else if (gen_param && lexer_peek(l).kind == TOK_LANGLE)
    {
        // impl<T> Struct<T> syntax: skip redundant <T> on struct name
        lexer_next(l);                                    // eat <
        record_generic_parameter_use(ctx, lexer_next(l)); // eat T
        if (lexer_next(l).kind != TOK_RANGLE)
        {
            zpanic_at(lexer_peek(l), "Expected >");
            return NULL;
        }
    }

    // `impl Trait for Type`: `for` is read by its text (see keywords.c).
    Token pk = lexer_peek(l);
    if (pk.kind == TOK_IDENT && pk.len == 3 && strncmp(pk.start, "for", 3) == 0)
    {
        lexer_next(l); // eat for
        Token t2 = lexer_next(l);
        char *name2 = token_strdup(t2);

        char *target_gen_param = NULL;
        if (lexer_peek(l).kind == TOK_LANGLE)
        {
            lexer_next(l); // eat <
            Token gt = lexer_next(l);
            target_gen_param = token_strdup(gt);
            if (lexer_next(l).kind != TOK_RANGLE)
            {
                zpanic_at(lexer_peek(l), "Expected > in impl struct generic");
                return NULL;
            }
            register_generic(ctx, gt);
        }
        // A type imported one by one, or in a module its own type (`impl Drop for Arena` of a
        // module imported with `as`).
        char *type_name = type_declaration_name(ctx, name2);
        if (type_name)
        {
            zfree(name2);
            name2 = type_name;
        }
        Type *target = impl_target(ctx, name2);
        name2 = type_impl_name(target);

        // Check for common error: swapped Struct and Trait
        // impl MyStruct for MyTrait (Wrong) vs impl MyTrait for MyStruct (Correct)
        if (!is_trait(ctx, name1) && is_trait(ctx, name2))
        {
            zpanic_at(t1,
                      "Incorrect usage of impl. Did you mean 'impl %s for %s'? Syntax is 'impl "
                      "<Trait> for <Struct>'",
                      name2, name1);
            return NULL;
        }

        // Auto-import std/mem.zc if implementing Drop, Copy, or Clone traits
        if (strcmp(name1, "Drop") == 0 || strcmp(name1, "Copy") == 0 || strcmp(name1, "Clone") == 0)
        {
            auto_import_std_mem(ctx);
        }

        ctx->current_impl_target = target;

        lexer_next(l);
        ASTNode *h = parse_impl_methods(ctx, l, name1);
        ctx->current_impl_target = NULL; // Restore context
        ASTNode *n = ast_create(NODE_IMPL_TRAIT);
        n->impl_trait.trait_name = name1;
        n->impl_trait.target = target;
        n->impl_trait.trait_token = t1;
        n->impl_trait.target_token = t2;
        n->impl_trait.methods = h;
        n->impl_trait.generic_param = target_gen_param;
        collect_declaration(ctx, n);

        ctx->known_generics_count = generics_before;
        return n;
    }
    else
    {
        // Regular impl Struct (impl Box or impl Box<T>)

        // A type imported one by one, or in a module its own type.
        char *type_name = type_declaration_name(ctx, name1);
        if (type_name)
        {
            zfree(name1);
            name1 = type_name;
        }

        Type *target = impl_target(ctx, name1);
        name1 = type_impl_name(target);

        // The type of `self` in its methods: in `impl Vec<T>`, a `Vec<T>`.
        ctx->current_impl_target = target;
        if (gen_param)
        {
            Type **args = xmalloc(sizeof(Type *));
            args[0] = type_new(TYPE_GENERIC);
            args[0]->name = xstrdup(gen_param);
            ctx->current_impl_target = type_new_instance(name1, args, 1);
        }

        if (gen_param)
        {
            // GENERIC IMPL TEMPLATE: impl Box<T>
            if (lexer_next(l).kind != TOK_LBRACE)
            {
                zpanic_at(lexer_peek(l), "Expected {");
                return NULL;
            }
            ASTNode *h = parse_impl_methods(ctx, l, NULL);
            ASTNode *n = ast_create(NODE_IMPL);
            n->token = t1;
            n->impl.target = target;
            n->impl.target_token = t1;
            n->impl.methods = h;
            n->impl.generic_param = gen_param;
            collect_declaration(ctx, n);
            ctx->current_impl_target = NULL;
            ctx->known_generics_count = generics_before;
            return NULL; // Do not emit generic template
        }
        else
        {
            // REGULAR IMPL
            lexer_next(l);
            ASTNode *h = parse_impl_methods(ctx, l, NULL);
            ASTNode *n = ast_create(NODE_IMPL);
            n->impl.target = target;
            n->impl.target_token = t1;
            n->impl.methods = h;
            collect_declaration(ctx, n);
            ctx->current_impl_target = NULL;

            ctx->known_generics_count = generics_before;
            return n;
        }
    }
}
