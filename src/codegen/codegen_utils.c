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
#include "analysis/drop_traits.h"

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

// Whether `t` is an enum, by its declaration: a value of a user's enum may have TYPE_STRUCT.
int is_enum_type(ParserContext *ctx, Type *t)
{
    ASTNode *def = named_type_def(ctx, t);
    return def && def->kind == NODE_ENUM;
}

const char *c_name(ParserContext *ctx, const char *name)
{
    return c_safe_name(name, ctx->config->use_cpp);
}

const char *local_decl_c_name(ParserContext *ctx, Token site, const char *name)
{
    const char *renamed = local_c_name_at(ctx, site.start, name);
    return renamed ? renamed : c_name(ctx, name);
}

const char *var_ref_c_name(ParserContext *ctx, ASTNode *ref)
{
    ZenSymbol *sym = ref->var_ref.symbol;
    return sym && sym->link_name ? sym->link_name : c_name(ctx, ref->var_ref.name);
}

const char *local_decl_flag_name(ParserContext *ctx, Token site, const char *name)
{
    const char *renamed = local_c_name_at(ctx, site.start, name);
    return renamed ? renamed : name;
}

const char *var_ref_flag_name(ASTNode *ref)
{
    ZenSymbol *sym = ref->var_ref.symbol;
    return sym && sym->link_name ? sym->link_name : ref->var_ref.name;
}

char *c_type_spelling(ParserContext *ctx, Type *t)
{
    return type_to_c_string_with(t, ctx->config->misra_mode, link_name_of, ctx);
}

// Where a function's declarator goes in the declaration its return type makes
// (emit_func_signature).
#define FUNCTION_DECLARATOR_MARK "\x01"

char *c_declaration(ParserContext *ctx, Type *t, const char *declarator)
{
    return type_to_c_declaration_with(t, declarator, ctx->config->misra_mode, link_name_of, ctx);
}

int has_c_spelling(Type *t)
{
    return t && (t->c_typedef_name || (t->kind != TYPE_UNKNOWN && t->kind != TYPE_U0));
}

char *node_c_type(ParserContext *ctx, ASTNode *node)
{
    Type *t = node ? node->type_info : NULL;
    return has_c_spelling(t) ? c_type_spelling(ctx, t) : NULL;
}

int is_fixed_size_array(Type *t)
{
    t = resolve_alias(t);
    return t && t->kind == TYPE_ARRAY && t->array_size > 0;
}

const char *zero_initializer(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return NULL;
    }
    TypeKind k = t->kind;
    if (k == TYPE_ARRAY || k == TYPE_STRUCT)
    {
        return ctx->config->use_cpp ? "{}" : "{0}";
    }
    if (k == TYPE_BOOL)
    {
        return "false";
    }
    if (integer_kind(k) || k == TYPE_ENUM)
    {
        return "0";
    }
    if (k == TYPE_F32)
    {
        return "0.0f";
    }
    if (k == TYPE_F64)
    {
        return "0.0";
    }
    return NULL;
}

int capture_needs_drop(ParserContext *ctx, ASTNode *lambda, int index)
{
    Type *t = lambda->lambda.captured_types_info[index];
    if (t && (t->kind == TYPE_FUNCTION || t->kind == TYPE_TUPLE))
    {
        return type_needs_drop(ctx, t);
    }
    ASTNode *def = t && (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->name
                       ? named_type_def(ctx, t)
                       : NULL;
    return def && def->type_info && def->type_info->traits.has_drop;
}

const char *field_c_type(ParserContext *ctx, ASTNode *field)
{
    return c_type_spelling(ctx, field->type_info);
}

void emit_c_decl(ParserContext *ctx, Type *t, const char *name)
{
    char *decl = c_declaration(ctx, t, name);
    EMIT(ctx, "%s", decl);
    zfree(decl);
}

void emit_var_decl_type(ParserContext *ctx, Type *t, const char *var_name)
{
    emit_c_decl(ctx, t, c_name(ctx, var_name));
}

void emit_variant_constructor_params(ParserContext *ctx, Type *tuple)
{
    for (int i = 0; i < tuple->count; i++)
    {
        char name[16];
        snprintf(name, sizeof(name), "_%d", i);
        emit_c_decl(ctx, tuple->args[i], name);
        EMIT(ctx, "%s", i < tuple->count - 1 ? ", " : "");
    }
}

void emit_auto_declaration(ParserContext *ctx, ASTNode *expr, const char *declarator)
{
    Type *t = expr ? expr->type_info : NULL;
    if (has_c_spelling(t))
    {
        emit_c_decl(ctx, t, declarator);
    }
    else if (z_path_match_compiler(ctx->config->cc, "tcc") && expr)
    {
        EMIT(ctx, "__typeof__((");
        codegen_expression(ctx, expr);
        EMIT(ctx, ")) %s", declarator);
    }
    else
    {
        EMIT(ctx, "ZC_AUTO %s", declarator);
    }
}

char *emit_function_head(ParserContext *ctx, Type *ret, const char *declarator)
{
    size_t size = strlen(declarator) + 2;
    char *with_mark = xmalloc(size);
    snprintf(with_mark, size, "%s%s", declarator, FUNCTION_DECLARATOR_MARK);
    char *decl = c_declaration(ctx, ret, with_mark);
    // c_declaration writes the declarator once: what follows the mark comes after the `)`.
    char *mark = strstr(decl, FUNCTION_DECLARATOR_MARK);
    char *after = xstrdup(mark ? mark + 1 : "");
    if (mark)
    {
        *mark = 0;
    }
    EMIT(ctx, "%s(", decl);
    zfree(decl);
    zfree(with_mark);
    return after;
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

    EMIT(ctx, "(");
    if (has_c_spelling(expr->type_info))
    {
        emit_c_decl(ctx, expr->type_info, "[]");
    }
    else
    {
        EMIT(ctx, "__typeof__((");
        codegen_expression(ctx, expr);
        EMIT(ctx, "))[]");
    }
    EMIT(ctx, "){");
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
           strcmp(function_c_name(func), "main") != 0 &&
           !c_headers_declare_function(ctx, c_function_name);
}

// The name of `func` in C.
static const char *func_c_name(ParserContext *ctx, ASTNode *func, const char *name_override)
{
    return func->link_name ? func->link_name
                           : c_name(ctx, name_override ? name_override : function_c_name(func));
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
        // Without its position the prototype cannot be removed: it stays, which is always valid C.
        long start = ftell(ctx->cg.emitter.file);
        if (start < 0)
        {
            ctx->cg.open_span_name = NULL;
            return;
        }
        ctx->cg.open_span_start = start;
    }
}

void prototype_span_end(ParserContext *ctx)
{
    if (!ctx->cg.open_span_name)
    {
        return;
    }
    long end = ftell(ctx->cg.emitter.file);
    if (end < 0)
    {
        ctx->cg.open_span_name = NULL;
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
    span->end = end;
    ctx->cg.open_span_name = NULL;
}

// Emit function signature using Type info for correct C codegen
void emit_func_signature(ParserContext *ctx, ASTNode *func, const char *name_override)
{
    if (!func || func->kind != NODE_FUNCTION)
    {
        return;
    }

    // In the prototype as in the definition: a call before the definition must know it too.
    if (func->func.noreturn)
    {
        EMIT(ctx, "__attribute__((noreturn)) ");
    }

    // Emit MISRA static linkage
    if (ctx->config->misra_mode && !func->func.is_export &&
        strcmp(function_c_name(func), "main") != 0)
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
    ret_suffix = emit_function_head(ctx, func->func.ret_type_info, final_name);

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

            const char *name = "";
            if (func->func.param_names && func->func.param_names[i])
            {
                name = c_name(ctx, func->func.param_names[i]);
            }

            if (func->func.c_type_overrides && func->func.c_type_overrides[i])
            {
                // An @ctype is C the program writes: the name follows it.
                EMIT(ctx, "%s %s", func->func.c_type_overrides[i], name);
            }
            else if (func->func.arg_types && func->func.arg_types[i])
            {
                emit_c_decl(ctx, func->func.arg_types[i], name);
            }
            else
            {
                EMIT(ctx, "void* %s", name);
            }
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
    Type *t = value->type_info;
    if (t && t->kind == TYPE_TUPLE)
    {
        return type_needs_drop(ctx, t) ? drop_glue_name(t) : NULL;
    }
    ASTNode *def = named_type_def(ctx, t);
    return def && def->type_info && def->type_info->traits.has_drop ? type_def_name(t) : NULL;
}

char *drop_glue_name(Type *t)
{
    return (t && t->kind == TYPE_TUPLE) ? type_mangle_spelling(t) : type_def_name(t);
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
    if (node->kind == NODE_EXPR_VAR && node->var_ref.variant.member)
    {
        return 0;
    }
    Type *t = node->type_info;
    if (!t)
    {
        return 0;
    }
    if (t->kind == TYPE_FUNCTION || t->kind == TYPE_TUPLE)
    {
        return type_needs_drop(ctx, t);
    }
    if (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM)
    {
        return type_has_drop(ctx, t);
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

    return moves_drop_value(ctx, node) ? emit_moved_out(ctx, node) : 0;
}

int emit_moved_out(ParserContext *ctx, ASTNode *node)
{
    if (!node)
    {
        return 0;
    }
    if (node->kind == NODE_EXPR_MEMBER)
    {
        // For members: memset(&foo.bar, 0, sizeof(foo.bar))
        EMIT(ctx, "memset(&");
        codegen_expression(ctx, node);
        EMIT(ctx, ", 0, sizeof(");
        codegen_expression(ctx, node);
        EMIT(ctx, "))");
        return 1;
    }
    if (node->kind != NODE_EXPR_VAR)
    {
        return 0;
    }

    const char *df_prefix = "";
    if (ctx->cg.current_lambda)
    {
        for (int i = 0; i < ctx->cg.current_lambda->lambda.capture_count; i++)
        {
            if (strcmp(node->var_ref.name, ctx->cg.current_lambda->lambda.captured_vars[i]) == 0)
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
    EMIT(ctx, "%s__z_drop_flag_%s = 0", df_prefix, var_ref_flag_name(node));
    return 1;
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

int impl_on_template(ParserContext *ctx, ASTNode *impl)
{
    Type *target = impl->kind == NODE_IMPL ? impl->impl.target : impl->impl_trait.target;
    ASTNode *def = named_type_def(ctx, target);
    return def && ((def->kind == NODE_STRUCT && def->strct.is_template) ||
                   (def->kind == NODE_ENUM && def->enm.is_template));
}
