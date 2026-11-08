// SPDX-License-Identifier: MIT

#include "../ast/ast.h"
#include "../constants.h"
#include "../parser/parser.h"
#include "../zprep.h"
#include "codegen.h"
#include "../ast/primitives.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Strip template suffix from a type name (for example, "MyStruct<T>" -> "MyStruct")
// Returns newly allocated string, caller must free.

// Helper to emit a mangled name (Type__Method) with standardized underscores.
// Uses the canonical mangle_method_symbol so call sites always emit the same
// name the parser used for the definition (e.g. `_method` keeps its triple
// underscore).
void emit_mangled_name(ParserContext *ctx, const char *base, const char *method)
{
    if (!base || !method)
    {
        return;
    }
    char *merged = mangle_method_symbol(base, NULL, method);

    ZenSymbol *sym = ctx ? find_symbol_in_all(ctx, merged) : NULL;
    if (sym && sym->link_name)
    {
        EMIT(ctx, "%s", sym->link_name);
    }
    else
    {
        EMIT(ctx, "%s", merged);
    }
    zfree(merged);
}

int is_enum_type_name(ParserContext *ctx, const char *name)
{
    if (!name || !ctx)
    {
        return 0;
    }
    const char *clean = name;
    if (strncmp(clean, "struct ", 7) == 0)
    {
        clean += 7;
    }
    ASTNode *def = find_struct_def(ctx, clean);
    return (def && def->kind == NODE_ENUM);
}

const char *c_name(ParserContext *ctx, const char *name)
{
    return c_safe_name(name, ctx->config->use_cpp);
}

char *c_type_spelling(ParserContext *ctx, Type *t)
{
    return type_to_c_string_with(t, ctx->config->misra_mode, link_name_of, ctx);
}

char *declared_c_type(ParserContext *ctx, ASTNode *decl)
{
    Type *t = decl->type_info;
    return (t && t->kind != TYPE_UNKNOWN) ? c_type_spelling(ctx, t) : NULL;
}

char *capture_c_type(ParserContext *ctx, ASTNode *lambda, int index)
{
    Type *t = lambda->lambda.captured_types_info[index];
    return t ? c_type_spelling(ctx, t) : xstrdup("int");
}

int capture_needs_drop(ParserContext *ctx, ASTNode *lambda, int index)
{
    Type *t = lambda->lambda.captured_types_info[index];
    if (t && (t->kind == TYPE_FUNCTION || t->kind == TYPE_TUPLE))
    {
        return type_needs_drop(ctx, t);
    }
    ASTNode *def = t && (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->name
                       ? find_struct_def(ctx, type_def_name(t))
                       : NULL;
    return def && def->type_info && def->type_info->traits.has_drop;
}

const char *field_c_type(ParserContext *ctx, ASTNode *field)
{
    return c_type_spelling(ctx, field->type_info);
}

// Helper to emit C declaration (handle arrays, function pointers correctly)
static void emit_c_decl(ParserContext *ctx, const char *type_str, const char *name)
{
    char *bracket = (char *)strchr(type_str, '[');
    char *generic = (char *)strchr(type_str, '<');
    char *fn_ptr = (char *)strstr(type_str, "(*");

    if (fn_ptr)
    {
        char *end_paren = (char *)strchr(fn_ptr, ')');
        if (end_paren)
        {
            ptrdiff_t prefix_len = end_paren - type_str;
            EMIT(ctx, "%.*s%s%s", (int)prefix_len, type_str, name, end_paren);
        }
        else
        {
            // Fallback if malformed (shouldn't happen)
            ptrdiff_t prefix_len = fn_ptr - type_str + 2;
            EMIT(ctx, "%.*s%s%s", (int)prefix_len, type_str, name, fn_ptr + 2);
        }
    }
    else if (generic && (!bracket || generic < bracket))
    {
        char *gt = (char *)strchr(generic, '>');
        int success = 0;

        if (gt)
        {
            ptrdiff_t base_len = generic - type_str;
            ptrdiff_t arg_len = gt - generic - 1;
            char *base = xmalloc((size_t)base_len + 1);
            memcpy(base, type_str, (size_t)base_len);
            base[base_len] = '\0';
            char *arg_list = xmalloc((size_t)arg_len + 1);
            memcpy(arg_list, generic + 1, (size_t)arg_len);
            arg_list[arg_len] = '\0';

            char *mangled_candidate = mangle_generic_name_from_arg_list(base, arg_list);
            if (find_struct_def(ctx, mangled_candidate))
            {
                EMIT(ctx, "%s %s", mangled_candidate, name);
                success = 1;
            }
        }

        if (!success)
        {
            ptrdiff_t base_len = generic - type_str;
            EMIT(ctx, "%.*s %s", (int)base_len, type_str, name);
        }
        else if (gt[1] == '*')
        {
            EMIT(ctx, "*");
        }

        if (bracket)
        {
            EMIT(ctx, "%s", bracket);
        }
    }
    else if (bracket)
    {
        ptrdiff_t base_len = bracket - type_str;
        EMIT(ctx, "%.*s %s%s", (int)base_len, type_str, name, bracket);
    }
    else
    {
        EMIT(ctx, "%s %s", type_str, name);
    }
}

// Helper to emit variable declarations with array types.
void emit_var_decl_type(ParserContext *ctx, const char *type_str, const char *var_name)
{
    emit_c_decl(ctx, type_str, c_name(ctx, var_name));
}

// Get field type from struct.
void emit_auto_type(ParserContext *ctx, ASTNode *init_expr, Token t)
{
    (void)t;
    char *inferred = NULL;
    if (init_expr)
    {
        inferred = infer_type(ctx, init_expr);
    }

    if (inferred && strcmp(inferred, "__auto_type") != 0 && strcmp(inferred, "unknown") != 0)
    {
        EMIT(ctx, "%s", inferred);
    }
    else
    {
        if (z_path_match_compiler(ctx->config->cc, "tcc") && init_expr)
        {
            EMIT(ctx, "__typeof__((");
            codegen_expression(ctx, init_expr);
            EMIT(ctx, "))");
        }
        else
        {
            EMIT(ctx, "ZC_AUTO");
        }
    }
}

// The C name of the type of expr when it can be written as a whole before a declarator
// (`T[]` in a compound literal, `T name` in a declaration), or NULL when it is unknown or
// is itself a declarator (a function pointer or an array).
char *c_type_name_or_null(ParserContext *ctx, ASTNode *expr)
{
    char *type = infer_type(ctx, expr);
    if (!type || strcmp(type, "__auto_type") == 0 || strcmp(type, "unknown") == 0)
    {
        return NULL;
    }
    if (strpbrk(type, "(["))
    {
        return NULL;
    }
    return type;
}

// Emits a pointer to a copy of expr, for a value that has no address of its own (a call
// result, a literal): a one-element compound literal in C, which lives until the end of the
// enclosing block, or _z_rval in C++, which keeps the temporary until the end of the full
// expression.
void emit_rvalue_address(ParserContext *ctx, ASTNode *expr)
{
    if (ctx->config->use_cpp)
    {
        EMIT(ctx, "_z_rval(");
        codegen_expression(ctx, expr);
        EMIT(ctx, ")");
        return;
    }

    char *type = c_type_name_or_null(ctx, expr);
    EMIT(ctx, "(");

    if (type)
    {
        EMIT(ctx, "%s", type);
    }
    else
    {
        EMIT(ctx, "__typeof__((");
        codegen_expression(ctx, expr);
        EMIT(ctx, "))");
    }

    EMIT(ctx, "[]){");
    codegen_expression(ctx, expr);
    EMIT(ctx, "}");
}

// Whether nothing outside the generated C may call the function by name, so that a whole-program
// build can give it internal linkage (ZC_FUNC is `static` with ZC_WHOLE_PROGRAM): not `main`, an
// @export, an extern declaration, a function with the C name given by @link_name, or a weak one.
// Nor one a C header declares (a program's own `abs`): C rejects a static definition after an
// extern declaration.
static int may_be_internal(ParserContext *ctx, ASTNode *func, const char *c_function_name)
{
    return !func->func.is_export && !func->func.is_extern && !func->link_name && !func->func.weak &&
           strcmp(func->func.name, "main") != 0 &&
           !c_headers_declare_function(ctx, c_function_name);
}

// The name of `func` in C.
static const char *func_c_name(ParserContext *ctx, ASTNode *func, const char *name_override)
{
    return func->link_name ? func->link_name
                           : c_name(ctx, name_override ? name_override : func->func.name);
}

// Whether the program reaches `func` only through calls by its name, which the C shows: it is
// internal, the C runtime does not run it (@constructor, @destructor), and no attribute makes the
// linker keep or place it (@section, custom ones such as @used or @alias).
static int reached_only_by_name(ParserContext *ctx, ASTNode *func, const char *c_function_name)
{
    return may_be_internal(ctx, func, c_function_name) && !func->func.constructor &&
           !func->func.destructor && !func->func.section && !func->func.attributes;
}

const char *omissible_function_name(ParserContext *ctx, ASTNode *func, const char *name_override)
{
    const char *name = func_c_name(ctx, func, name_override);
    return reached_only_by_name(ctx, func, name) ? name : NULL;
}

void prototype_span_begin(ParserContext *ctx, ASTNode *func, const char *name_override)
{
    ctx->cg.open_span_name = ctx->cg.emitter.mode == EMITTER_FILE
                                 ? omissible_function_name(ctx, func, name_override)
                                 : NULL;
    if (ctx->cg.open_span_name)
    {
        ctx->cg.open_span_start = ftell(ctx->cg.emitter.file);
    }
}

void prototype_span_end(ParserContext *ctx)
{
    if (!ctx->cg.open_span_name)
    {
        return;
    }
    if (ctx->cg.prototype_span_count == ctx->cg.prototype_span_cap)
    {
        ctx->cg.prototype_span_cap =
            ctx->cg.prototype_span_cap ? ctx->cg.prototype_span_cap * 2 : 256;
        ctx->cg.prototype_spans = xrealloc(
            ctx->cg.prototype_spans, sizeof(PrototypeSpan) * (size_t)ctx->cg.prototype_span_cap);
    }
    PrototypeSpan *span = &ctx->cg.prototype_spans[ctx->cg.prototype_span_count++];
    span->name = ctx->cg.open_span_name;
    span->start = ctx->cg.open_span_start;
    span->end = ftell(ctx->cg.emitter.file);
    ctx->cg.open_span_name = NULL;
}

// Emit function signature using Type info for correct C codegen
void emit_func_signature(ParserContext *ctx, ASTNode *func, const char *name_override)
{
    if (!func || func->kind != NODE_FUNCTION)
    {
        return;
    }

    // Emit MISRA static linkage
    if (ctx->config->misra_mode && !func->func.is_export && strcmp(func->func.name, "main") != 0)
    {
        EMIT(ctx, "static ");
    }

    // Emit CUDA qualifiers (for both forward declarations and definitions)
    if (ctx->config->use_cuda)
    {
        if (func->func.cuda_global)
        {
            EMIT(ctx, "__global__ ");
        }
        if (func->func.cuda_device)
        {
            EMIT(ctx, "__device__ ");
        }
        if (func->func.cuda_host)
        {
            EMIT(ctx, "__host__ ");
        }
    }

    char *ret_suffix = NULL;

    const char *final_name = func_c_name(ctx, func, name_override);

    if (strcmp(final_name, "z_plugin_init") != 0 && may_be_internal(ctx, func, final_name))
    {
        EMIT(ctx, "ZC_FUNC ");
    }

    // Return type
    char *ret_str;
    if (func->func.ret_type_info)
    {
        ret_str = c_type_spelling(ctx, func->func.ret_type_info);
    }
    else
    {
        ret_str = xstrdup("void");
    }

    char *fn_ptr = (char *)strstr(ret_str, "(*)");

    if (fn_ptr)
    {
        ptrdiff_t prefix_len = fn_ptr - ret_str + 2; // Include "(*"
        EMIT(ctx, "%.*s%s(", (int)prefix_len, ret_str, final_name);
        ret_suffix = xstrdup(fn_ptr + 2);
    }
    else
    {
        EMIT(ctx, "%s %s(", ret_str, final_name);
    }
    zfree(ret_str);

    // Args
    if (func->func.count == 0 && !func->func.is_varargs)
    {
        EMIT(ctx, "void");
    }
    else
    {
        for (int i = 0; i < func->func.count; i++)
        {
            if (i > 0)
            {
                EMIT(ctx, ", ");
            }

            char *type_str = NULL;
            // Check for @ctype override first
            if (func->func.c_type_overrides && func->func.c_type_overrides[i])
            {
                type_str = xstrdup(func->func.c_type_overrides[i]);
            }
            else if (func->func.arg_types && func->func.arg_types[i])
            {
                type_str = c_type_spelling(ctx, func->func.arg_types[i]);
            }
            else
            {
                type_str = xstrdup("void*"); // Fallback
            }

            const char *name = "";

            if (func->func.param_names && func->func.param_names[i])
            {
                name = c_name(ctx, func->func.param_names[i]);
            }

            // check if array type
            emit_c_decl(ctx, type_str, name);
            zfree(type_str);
        }
        if (func->func.is_varargs)
        {
            if (func->func.count > 0)
            {
                EMIT(ctx, ", ");
            }
            EMIT(ctx, "...");
        }
    }
    EMIT(ctx, ")");

    if (ret_suffix)
    {
        EMIT(ctx, "%s", ret_suffix);
        zfree(ret_suffix);
    }
}

char *value_drop_struct(ParserContext *ctx, ASTNode *value)
{
    if (value->type_info && value->type_info->kind == TYPE_TUPLE)
    {
        return type_needs_drop(ctx, value->type_info) ? drop_glue_name(value->type_info) : NULL;
    }
    char *tname = NULL;
    if (value->type_info && value->type_info->kind == TYPE_STRUCT && value->type_info->name)
    {
        tname = type_def_name(value->type_info);
    }
    else
    {
        tname = infer_type(ctx, value);
    }
    if (!tname || strchr(tname, '*'))
    {
        return NULL;
    }
    if (strncmp(tname, "struct ", 7) == 0)
    {
        tname += 7;
    }
    ASTNode *def = find_struct_def(ctx, tname);
    if (def && (def->kind == NODE_STRUCT || def->kind == NODE_ENUM) && def->type_info &&
        def->type_info->traits.has_drop)
    {
        return tname;
    }
    return NULL;
}

char *drop_glue_name(Type *t)
{
    return (t && t->kind == TYPE_TUPLE) ? type_mangle_spelling(t) : type_def_name(t);
}

ASTNode *named_type_def(ParserContext *ctx, Type *t)
{
    return (t && (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->name)
               ? find_struct_def(ctx, type_def_name(t))
               : NULL;
}

char *drop_value_stmt(Type *t, const char *lvalue, int depth)
{
    size_t lv = strlen(lvalue);
    if (is_owning_closure(t))
    {
        size_t sz = 32 + lv * 3;
        char *out = xmalloc(sz);
        snprintf(out, sz, "if (%s.drop) %s.drop(%s.ctx);", lvalue, lvalue, lvalue);
        return out;
    }
    if (t->kind == TYPE_ARRAY)
    {
        size_t esz = lv + 32;
        char *element = xmalloc(esz);
        snprintf(element, esz, "%s[_zi%d]", lvalue, depth);
        char *inner = drop_value_stmt(t->inner, element, depth + 1);
        size_t sz = 128 + strlen(inner);
        char *out = xmalloc(sz);
        snprintf(out, sz, "for (size_t _zi%d = 0; _zi%d < %d; _zi%d++) { %s }", depth, depth,
                 t->array_size, depth, inner);
        return out;
    }
    const char *glue = drop_glue_name(t);
    size_t sz = 32 + strlen(glue) + lv;
    char *out = xmalloc(sz);
    snprintf(out, sz, "%s__Drop__glue(&%s);", glue, lvalue);
    return out;
}

int moves_drop_value(ParserContext *ctx, ASTNode *node)
{
    // An enum variant (`Slot::Empty`) builds a new value each time: there is nothing to invalidate.
    if (node->kind == NODE_EXPR_VAR && node->var_ref.variant.name)
    {
        return 0;
    }
    char *type_name = infer_type(ctx, node);
    ASTNode *def = NULL;
    if (type_name)
    {
        char *clean_type = type_name;
        if (strncmp(clean_type, "struct ", 7) == 0)
        {
            clean_type += 7;
        }
        def = find_struct_def(ctx, clean_type);
    }

    Type *t = node->type_info;
    if (!t)
    {
        return def && def->type_info && def->type_info->traits.has_drop;
    }
    if (t->kind == TYPE_FUNCTION || t->kind == TYPE_TUPLE)
    {
        return type_needs_drop(ctx, t);
    }
    if (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM)
    {
        // The definition the type names, or else the one its C spelling names.
        ASTNode *tdef = named_type_def(ctx, t);
        if (tdef)
        {
            def = tdef;
        }
        return (def && def->type_info) ? def->type_info->traits.has_drop : t->traits.has_drop;
    }
    return 0;
}

int emit_move_invalidation(ParserContext *ctx, ASTNode *node)
{
    if (!node)
    {
        return 0;
    }

    // Check if it's a valid l-value we can memset
    if (node->kind != NODE_EXPR_VAR && node->kind != NODE_EXPR_MEMBER)
    {
        return 0;
    }

    if (moves_drop_value(ctx, node))
    {
        if (node->kind == NODE_EXPR_VAR)
        {
            char *df_prefix = "";
            if (ctx->cg.current_lambda)
            {
                for (int i = 0; i < ctx->cg.current_lambda->lambda.capture_count; i++)
                {
                    if (strcmp(node->var_ref.name,
                               ctx->cg.current_lambda->lambda.captured_vars[i]) == 0)
                    {
                        if (ctx->cg.current_lambda->lambda.capture_modes &&
                            ctx->cg.current_lambda->lambda.capture_modes[i] == 0)
                        {
                            df_prefix = "ctx->";
                        }
                        break;
                    }
                }
            }

            if (strcmp(node->var_ref.name, "self") != 0)
            {
                EMIT(ctx, "%s__z_drop_flag_%s = 0", df_prefix, node->var_ref.name);
                return 1;
            }
            return 0;
        }
        else if (node->kind == NODE_EXPR_MEMBER)
        {
            // For members: memset(&foo.bar, 0, sizeof(foo.bar))
            EMIT(ctx, "memset(&");
            codegen_expression(ctx, node);
            EMIT(ctx, ", 0, sizeof(");
            codegen_expression(ctx, node);
            EMIT(ctx, "))");
            return 1;
        }
    }
    return 0;
}

// Emits expression, wrapping it in a move-invalidation block if it's a consuming variable usage
void codegen_expression_with_move(ParserContext *ctx, ASTNode *node)
{
    if (!node)
    {
        return;
    }

    if (node && (node->kind == NODE_EXPR_VAR || node->kind == NODE_EXPR_MEMBER))
    {
        if (moves_drop_value(ctx, node))
        {
            if (node->kind == NODE_EXPR_VAR)
            {
                EMIT(ctx, "({ ");
                emit_move_invalidation(ctx, node);
                EMIT(ctx, "; ");
                codegen_expression(ctx, node);
                EMIT(ctx, "; })");
            }
            else
            {
                EMIT(ctx, "({ __typeof__(");
                codegen_expression(ctx, node);
                EMIT(ctx, ") _mv = ");
                codegen_expression(ctx, node);
                EMIT(ctx, "; ");
                emit_move_invalidation(ctx, node);
                EMIT(ctx, "; _mv; })");
            }
            return;
        }
    }
    codegen_expression(ctx, node);
}

// Helper: check if an enum is "simple" (no variant has a payload).
// Simple enums are emitted as plain C enums, not struct+tag+union.
int is_simple_enum(ParserContext *ctx, const char *enum_name)
{
    if (!ctx || !enum_name)
    {
        return 0;
    }
    const char *clean = enum_name;
    if (strncmp(clean, "struct ", 7) == 0)
    {
        clean += 7;
    }
    ASTNode *def = find_struct_def(ctx, clean);
    if (!def || def->kind != NODE_ENUM)
    {
        return 0;
    }
    ASTNode *v = def->enm.variants;
    while (v)
    {
        if (v->variant.payload)
        {
            return 0;
        }
        v = v->next;
    }
    return 1;
}

int impl_on_template(ParserContext *ctx, ASTNode *impl)
{
    Type *target = impl->kind == NODE_IMPL ? impl->impl.target : impl->impl_trait.target;
    ASTNode *def = target ? find_struct_def(ctx, type_impl_name(target)) : NULL;
    return def && ((def->kind == NODE_STRUCT && def->strct.is_template) ||
                   (def->kind == NODE_ENUM && def->enm.is_template));
}
