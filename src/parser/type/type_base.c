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

// The type an opaque alias name stands for: a TYPE_ALIAS wrapping its underlying type.
static Type *opaque_alias_type(TypeAlias *alias_node)
{
    Type *wrapper = type_new(TYPE_ALIAS);
    wrapper->name = xstrdup(alias_node->alias);
    wrapper->inner = type_clone(alias_node->type_info);
    wrapper->alias.is_opaque_alias = 1;
    wrapper->alias.alias_defined_in_file =
        alias_node->defined_in_file ? xstrdup(alias_node->defined_in_file) : NULL;
    return wrapper;
}

// The type an alias name stands for: an opaque alias keeps its name, any other
// alias is the type it names.
Type *alias_type(TypeAlias *alias_node)
{
    if (alias_node->is_opaque)
    {
        return opaque_alias_type(alias_node);
    }
    return type_clone(alias_node->type_info);
}

// The type for a non-primitive type name that is not parsed from source (the
// receiver of a method, a generic argument): the type of the alias of that name if
// there is one, a struct otherwise.
Type *type_from_name(ParserContext *ctx, const char *name)
{
    TypeAlias *alias_node = ctx ? find_type_alias_node(ctx, name) : NULL;
    if (alias_node)
    {
        return alias_type(alias_node);
    }
    Type *tuple = ctx ? find_tuple_type(ctx, name) : NULL;
    if (tuple)
    {
        return tuple;
    }
    Type *t = type_new(TYPE_STRUCT);
    t->name = xstrdup(name);
    return t;
}

// The `>` that closes a list of generic arguments. A `>>` (two lists closing together, as in
// `Vec<Vec<int>>`) is split: its first `>` is consumed and the second is left for the outer
// list. Returns 0 when neither comes next.
static int consume_generic_close(Lexer *l)
{
    Token t = lexer_peek(l);
    if (t.kind == TOK_RANGLE)
    {
        lexer_next(l);
        return 1;
    }
    if (t.kind == TOK_OP && t.len == 2 && strncmp(t.start, ">>", 2) == 0)
    {
        l->pos += 1;
        l->col += 1;
        return 1;
    }
    return 0;
}

static Type *parse_type_base_impl(ParserContext *ctx, Lexer *l)
{
    Token t = lexer_peek(l);

    if (t.kind == TOK_IDENT)
    {
        int explicit_struct = 0;
        // Handle "struct Name" or "enum Name"
        if ((t.len == 6 && strncmp(t.start, "struct", 6) == 0) ||
            (t.len == 4 && strncmp(t.start, "enum", 4) == 0))
        {
            if (strncmp(t.start, "struct", 6) == 0)
            {
                explicit_struct = 1;
            }
            lexer_next(l); // consume keyword
            t = lexer_peek(l);
            if (t.kind != TOK_IDENT)
            {
                zpanic_at(t, "Expected identifier after struct/enum");
                return NULL;
            }
        }

        lexer_next(l);
        char *name = token_strdup(t);

        // Check for alias
        TypeAlias *alias_node = find_type_alias_node(ctx, name);
        if (alias_node)
        {
            zfree(name);
            return alias_type(alias_node);
        }
        Type *tuple = find_tuple_type(ctx, name);
        if (tuple)
        {
            zfree(name);
            return tuple;
        }

        // Self type alias: Replace "Self" with current impl struct type
        if (strcmp(name, "Self") == 0 && ctx->current_impl_struct)
        {
            name = xstrdup(ctx->current_impl_struct);
        }

        // Handle Namespace :: (A::B -> A_B)
        while (lexer_peek(l).kind == TOK_DCOLON)
        {
            lexer_next(l); // eat ::
            Token next = lexer_next(l);
            if (next.kind != TOK_IDENT)
            {
                zpanic_at(t, "Expected identifier after ::");
                return NULL;
            }

            char *suffix = token_strdup(next);
            // Map aliases (I32 -> int32_t, string -> char*) using centralized logic
            const char *resolved_suffix = normalize_type_name(suffix);

            // Check if 'name' is a module alias (e.g., m::Vector)
            Module *mod = find_module(ctx, name);
            char *merged;
            if (mod)
            {
                // Module-qualified type
                if (mod->is_c_header)
                {
                    // C header: Use type name directly without prefix
                    // To prevent name mangling, we might consider changing
                    // this to also use the prefix.
                    merged = xstrdup(resolved_suffix);

                    register_extern_symbol(ctx, merged);
                }
                else
                {
                    // Zen module: Use module base name as prefix
                    merged = mangle_module_symbol(mod->base_name, resolved_suffix);
                }
            }
            else
            {
                // Regular namespace or enum variant
                merged = mangle_module_symbol(name, resolved_suffix);
            }

            zfree(name);

            name = merged;
        }

        // Check for Primitives (Base types)
        // Check for Primitives (Base types)
        const ZenPrimitive *prim = find_primitive_by_name(name);
        if (prim)
        {
            zfree(name);
            Type *t_prim = type_new(prim->kind);
            return t_prim;
        }

        // C23 BitInt Support (i42, u256, etc.)
        if ((name[0] == 'i' || name[0] == 'u') && isdigit(name[1]))
        {
            // Verify it is a purely numeric suffix
            int valid = 1;
            for (size_t k = 1; k < strlen(name); k++)
            {
                if (!isdigit(name[k]))
                {
                    valid = 0;
                    break;
                }
            }
            if (valid)
            {
                int width = (int)strtol(name + 1, NULL, 10);
                if (width > 0)
                {
                    // Map standard widths to standard types for standard ABI/C compabitility
                    if (name[0] == 'i')
                    {
                        if (width == 8)
                        {
                            zfree(name);
                            return type_new(TYPE_I8);
                        }
                        if (width == 16)
                        {
                            zfree(name);
                            return type_new(TYPE_I16);
                        }
                        if (width == 32)
                        {
                            zfree(name);
                            return type_new(TYPE_I32);
                        }
                        if (width == 64)
                        {
                            zfree(name);
                            return type_new(TYPE_I64);
                        }
                        if (width == 128)
                        {
                            zfree(name);
                            return type_new(TYPE_I128);
                        }
                    }
                    else
                    {
                        if (width == 8)
                        {
                            zfree(name);
                            return type_new(TYPE_U8);
                        }
                        if (width == 16)
                        {
                            zfree(name);
                            return type_new(TYPE_U16);
                        }
                        if (width == 32)
                        {
                            zfree(name);
                            return type_new(TYPE_U32);
                        }
                        if (width == 64)
                        {
                            zfree(name);
                            return type_new(TYPE_U64);
                        }
                        if (width == 128)
                        {
                            zfree(name);
                            return type_new(TYPE_U128);
                        }
                    }

                    Type *inner_t = type_new(name[0] == 'u' ? TYPE_UBITINT : TYPE_BITINT);
                    inner_t->array_size = width;
                    zfree(name);
                    return inner_t;
                }
            }
        }

        // Relaxed Type Check: If explicit 'struct Name', trust the user.
        if (explicit_struct)
        {
            Type *ty = type_new(TYPE_STRUCT);
            ty->name = name;
            ty->is_explicit_struct = 1;
            return ty;
        }

        // Selective imports ONLY apply when we're NOT in a module context
        if (!ctx->imports.current_module_prefix)
        {
            SelectiveImport *si = find_selective_import(ctx, name);
            if (si)
            {
                // This is a selectively imported symbol
                // Resolve to the actual struct name which was prefixed during module
                // parsing
                zfree(name);
                name = mangle_module_symbol(si->source_module, si->symbol);
            }
        }

        // If we're IN a module and no selective import matched, apply module prefix
        // to types defined WITHIN this module. Types imported from other modules
        // (identifiable by existing as bare-named structs/aliases) are not prefixed.
        if (ctx->imports.current_module_prefix && !is_known_generic(ctx, name) &&
            !is_primitive_type_name(name) && strcasecmp(name, "Self") != 0 &&
            !is_extern_symbol(ctx, name))
        {
            // Check if this is a type imported from another module (registered
            // without prefix). If so, don't prefix — the bare name already resolves.
            int is_imported_type =
                (find_struct_def(ctx, name) != NULL) || (find_type_alias(ctx, name) != NULL);

            if (!is_imported_type)
            {
                // Auto-prefix struct name if in module context (unless it's a known
                // primitive/generic or an imported type)
                char *prefixed_name =
                    mangle_module_symbol(ctx->imports.current_module_prefix, name);
                zfree(name);
                name = prefixed_name;
            }
        }

        if (!is_known_generic(ctx, name) && strcmp(name, "Self") != 0)
        {
            register_type_usage(ctx, name, t);
        }

        // A generic parameter of the template being parsed (`T`) is not a struct.
        Type *ty = type_new(is_known_generic(ctx, name) ? TYPE_GENERIC : TYPE_STRUCT);
        ty->name = name;
        ty->is_explicit_struct = explicit_struct;

        // Handle Generics <T> or <K, V>
        if (lexer_peek(l).kind == TOK_LANGLE ||
            (lexer_peek(l).kind == TOK_OP && strncmp(lexer_peek(l).start, "<", 1) == 0))
        {
            lexer_next(l); // eat <
            // An instance is its template and the types of its arguments; its C name is
            // computed from them.
            int arg_count = 0;
            Type **arg_types = NULL;
            do
            {
                if (arg_count > 0)
                {
                    lexer_next(l); // eat ,
                }
                Type *arg = parse_type_formal(ctx, l);
                if (!arg)
                {
                    return NULL;
                }
                arg_types = xrealloc(arg_types, sizeof(Type *) * (size_t)(arg_count + 1));
                arg_types[arg_count++] = arg;
            } while (lexer_peek(l).kind == TOK_COMMA);

            if (!consume_generic_close(l))
            {
                zpanic_at(t, "Expected > after generic");
                return NULL;
            }
            ty->kind = TYPE_STRUCT;
            ty->args = arg_types;
            ty->count = arg_count;
            instantiate_type(ctx, ty, t);
        }
        return ty;
    }

    if (t.kind == TOK_LBRACKET)
    {
        lexer_next(l);
        Type *inner = parse_type_formal(ctx, l);
        if (!inner)
        {
            return NULL;
        }

        // Check for fixed-size array [T; N]
        if (lexer_peek(l).kind == TOK_SEMICOLON)
        {
            lexer_next(l); // eat ;
            ASTNode *size_expr = parse_expression(ctx, l);
            long long compiled_size = 0;
            int size = 0;
            if (eval_const_int_expr(size_expr, ctx, &compiled_size))
            {
                size = (int)compiled_size;
            }
            else
            {
                zpanic_at(size_expr->token,
                          "Array size must be a compile-time constant or integer literal");
                return NULL;
            }
            if (lexer_next(l).kind != TOK_RBRACKET)
            {
                zpanic_at(lexer_peek(l), "Expected ] after array size");
                return NULL;
            }

            Type *arr = type_new(TYPE_ARRAY);
            arr->inner = inner;
            arr->array_size = size;
            return arr;
        }

        // Otherwise it's a slice [T]
        Token close = lexer_next(l);
        if (close.kind != TOK_RBRACKET)
        {
            zpanic_at(lexer_peek(l), "Expected ] in type");
            return NULL;
        }
        return instantiate_slice(ctx, inner, close);
    }

    if (t.kind == TOK_LPAREN)
    {
        lexer_next(l);
        int count = 0;
        int capacity = 4;
        Type **elements = xmalloc(sizeof(Type *) * (size_t)capacity);
        while (1)
        {
            Type *sub = parse_type_formal(ctx, l);
            if (!sub)
            {
                break;
            }
            if (count == capacity)
            {
                capacity *= 2;
                elements = xrealloc(elements, sizeof(Type *) * (size_t)capacity);
            }
            elements[count++] = sub;

            if (lexer_peek(l).kind == TOK_COMMA)
            {
                lexer_next(l);
            }
            else
            {
                break;
            }
        }
        if (lexer_next(l).kind != TOK_RPAREN)
        {
            zpanic_at(lexer_peek(l), "Expected ) in tuple");
            return NULL;
        }

        Type *tuple = type_new_tuple(elements, count);
        register_tuple(ctx, tuple);
        return tuple;
    }

    // If we have an identifier that wasn't found,
    // assume it is a valid external C type
    // (for example, a struct defined in implementation).
    if (t.kind == TOK_IDENT)
    {
        char *fallback = token_strdup(t);
        lexer_next(l);
        Type *ty = type_new(TYPE_STRUCT);
        ty->name = fallback;
        ty->is_explicit_struct = 0;
        return ty;
    }

    return type_new(TYPE_UNKNOWN);
}

// The depth guard wraps the whole parse, so no return path can leave it raised: the
// language server keeps one parser context for a session and recovers from errors.
Type *parse_type_base(ParserContext *ctx, Lexer *l)
{
    RECURSION_GUARD(ctx, l, type_new(TYPE_UNKNOWN));
    Type *t = parse_type_base_impl(ctx, l);
    RECURSION_EXIT(ctx);
    return t;
}
