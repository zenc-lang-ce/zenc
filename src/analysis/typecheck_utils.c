// SPDX-License-Identifier: MIT
#include "../utils/colors.h"
#include "typecheck_internal.h"
#include "../constants.h"

#include "typecheck.h"
#include "../platform/os.h"
#include "comptime_interpreter.h"
#include "diagnostics/diagnostics.h"
#include "move_check.h"
#include "platform/misra.h"
#include <ctype.h>
#include <string.h>

// Whether `op` is written with a `=`: the assignments and `== != <= >=`. The side-effect checks
// below and the step of a `for` (check_loop_passes) used to test the operator's text for a `=`;
// this keeps their rule as it was.
int writes_with_equals(OperatorKind op)
{
    return is_assignment_operator(op) || op == OP_EQ || op == OP_NE || op == OP_LE || op == OP_GE;
}

// External helpers from parser

int tc_expr_has_side_effects(ASTNode *node)
{
    if (!node)
    {
        return 0;
    }

    switch (node->kind)
    {
    case NODE_EXPR_CALL:
        // Function calls are always considered to have potential side effects in MISRA
        return 1;

    case NODE_EXPR_BINARY:
        // Assignment operators are side effects. Any operator written with `=` counts, the
        // comparisons `== != <= >=` too, as when this tested the text for a `=`.
        if (writes_with_equals(node->binary.op))
        {
            return 1;
        }
        return tc_expr_has_side_effects(node->binary.left) ||
               tc_expr_has_side_effects(node->binary.right);

    case NODE_EXPR_UNARY:
        // Increment/Decrement are side effects (prefix and postfix)
        if (is_increment_operator(node->unary.op))
        {
            return 1;
        }
        return tc_expr_has_side_effects(node->unary.operand);

    case NODE_RAW_STMT:
    case NODE_EXPR_FSTRING:
        // Inline assembly always has potential side effects
        return 1;

    case NODE_EXPR_STRUCT_INIT:
    {
        ASTNode *f = node->struct_init.fields;
        while (f)
        {
            if (tc_expr_has_side_effects(f->var_decl.init_expr))
            {
                return 1;
            }
            f = f->next;
        }
        return 0;
    }

    case NODE_EXPR_ARRAY_LITERAL:
    {
        ASTNode *e = node->array_literal.elements;
        while (e)
        {
            if (tc_expr_has_side_effects(e))
            {
                return 1;
            }
            e = e->next;
        }
        return 0;
    }

    case NODE_EXPR_TUPLE_LITERAL:
    {
        ASTNode *e = node->tuple_literal.elements;
        while (e)
        {
            if (tc_expr_has_side_effects(e))
            {
                return 1;
            }
            e = e->next;
        }
        return 0;
    }

    case NODE_EXPR_MEMBER:
        return tc_expr_has_side_effects(node->member.target);

    case NODE_EXPR_INDEX:
        return tc_expr_has_side_effects(node->index.array) ||
               tc_expr_has_side_effects(node->index.index);

    case NODE_EXPR_CAST:
        return tc_expr_has_side_effects(node->cast.expr);

    case NODE_EXPR_SLICE:
        return tc_expr_has_side_effects(node->slice.array) ||
               tc_expr_has_side_effects(node->slice.start) ||
               tc_expr_has_side_effects(node->slice.end);

    default:
        // Most other nodes (LITERAL, VAR, SIZEOF itself if nested) are side-effect free
        return 0;
    }
}

void collect_symbols(ASTNode *node, SymbolSet *reads, SymbolSet *writes)
{
    if (!node)
    {
        return;
    }

    switch (node->kind)
    {
    case NODE_EXPR_BINARY:
        if (writes_with_equals(node->binary.op))
        {
            // LHS is a write
            if (node->binary.left->kind == NODE_EXPR_VAR)
            {
                if (writes->count < 32)
                {
                    writes->syms[writes->count++] = node->binary.left->var_ref.symbol;
                }
            }
            collect_symbols(node->binary.left, reads, writes); // In case of complex LHS
            collect_symbols(node->binary.right, reads, writes);
        }
        else
        {
            collect_symbols(node->binary.left, reads, writes);
            collect_symbols(node->binary.right, reads, writes);
        }
        break;

    case NODE_EXPR_UNARY:
        if (is_increment_operator(node->unary.op))
        {
            if (node->unary.operand->kind == NODE_EXPR_VAR)
            {
                if (writes->count < 32)
                {
                    writes->syms[writes->count++] = node->unary.operand->var_ref.symbol;
                }
            }
        }
        collect_symbols(node->unary.operand, reads, writes);
        break;

    case NODE_EXPR_VAR:
        if (reads->count < 32)
        {
            reads->syms[reads->count++] = node->var_ref.symbol;
        }
        break;

    case NODE_EXPR_CALL:
        collect_symbols(node->call.callee, reads, writes);
        ASTNode *arg = node->call.args;
        while (arg)
        {
            collect_symbols(arg, reads, writes);
            arg = arg->next;
        }
        break;

    default:
        // Generic traversal? For now just handle these.
        break;
    }
}
void check_side_effect_collision(TypeChecker *tc, ASTNode *left, ASTNode *right, Token token)
{
    CompilerConfig *cfg = &tc->pctx->compiler->config;
    if (!cfg->misra_mode || !left || !right)
    {
        return;
    }

    SymbolSet l_reads = {0}, l_writes = {0};
    SymbolSet r_reads = {0}, r_writes = {0};

    collect_symbols(left, &l_reads, &l_writes);
    collect_symbols(right, &r_reads, &r_writes);

    // Rule 13.2: Modification collision
    for (int i = 0; i < l_writes.count; i++)
    {
        ZenSymbol *s = l_writes.syms[i];
        if (!s)
        {
            continue;
        }

        // Check against other writes
        for (int j = 0; j < r_writes.count; j++)
        {
            if (s == r_writes.syms[j])
            {
                tc_error(tc, token, "MISRA Rule 13.2: symbol modified in multiple sub-expressions");
                return;
            }
        }

        // Check against other reads
        for (int j = 0; j < r_reads.count; j++)
        {
            if (s == r_reads.syms[j])
            {
                tc_error(tc, token,
                         "MISRA Rule 13.2: symbol both read and modified in same expression");
                return;
            }
        }
    }

    // Vice versa for r_writes against l_reads
    for (int i = 0; i < r_writes.count; i++)
    {
        ZenSymbol *s = r_writes.syms[i];
        if (!s)
        {
            continue;
        }

        for (int j = 0; j < l_reads.count; j++)
        {
            if (s == l_reads.syms[j])
            {
                tc_error(tc, token,
                         "MISRA Rule 13.2: symbol both read and modified in same expression");
                return;
            }
        }
    }
}

void check_all_args_side_effects(TypeChecker *tc, ASTNode *receiver, ASTNode *args, Token token)
{
    CompilerConfig *cfg = &tc->pctx->compiler->config;
    if (!cfg->misra_mode)
    {
        return;
    }

    SymbolSet reads = {0}, writes = {0};

    if (receiver)
    {
        collect_symbols(receiver, &reads, &writes);
    }

    ASTNode *arg = args;
    while (arg)
    {
        SymbolSet next_reads = {0}, next_writes = {0};
        collect_symbols(arg, &next_reads, &next_writes);

        // Check against cumulative sets
        for (int i = 0; i < next_writes.count; i++)
        {
            ZenSymbol *s = next_writes.syms[i];
            for (int r = 0; r < reads.count; r++)
            {
                if (s == reads.syms[r])
                {
                    tc_error(tc, token, "MISRA Rule 13.2: argument read and modified in same call");
                    return;
                }
            }
            for (int w = 0; w < writes.count; w++)
            {
                if (s == writes.syms[w])
                {
                    tc_error(tc, token, "MISRA Rule 13.2: symbol modified in multiple arguments");
                    return;
                }
            }
        }
        for (int i = 0; i < next_reads.count; i++)
        {
            ZenSymbol *s = next_reads.syms[i];
            for (int w = 0; w < writes.count; w++)
            {
                if (s == writes.syms[w])
                {
                    tc_error(tc, token, "MISRA Rule 13.2: argument read and modified in same call");
                    return;
                }
            }
        }

        // Add to cumulative sets
        for (int i = 0; i < next_reads.count && reads.count < 32; i++)
        {
            reads.syms[reads.count++] = next_reads.syms[i];
        }
        for (int i = 0; i < next_writes.count && writes.count < 32; i++)
        {
            writes.syms[writes.count++] = next_writes.syms[i];
        }

        arg = arg->next;
    }
}

// Internal MISRA helpers moved to platform/misra.c

// Whether `t` is in a file of the standard library other than the one being checked.
// Containers do not own what they store yet (the memory model is pending), so the
// instances of `Vec<T>` with a T that implements Drop get false move errors inside
// std/vec.zc. They are not the program's errors: they are not reported.
// Two characters of a path are the same, taking any two separators as one (`D:\zenc\std\vec.zc`
// on Windows, under a root that may be written with `/`).
static int same_path_char(char a, char b)
{
    return a == b || (z_is_path_sep(a) && z_is_path_sep(b));
}

int tc_file_in_std(TypeChecker *tc, const char *file)
{
    const CompilerConfig *cfg = tc->pctx->config;
    size_t len = strlen(cfg->std_root);
    if (len == 0 || !file)
    {
        return 0;
    }
    for (size_t i = 0; i < len; i++)
    {
        if (!same_path_char(file[i], cfg->std_root[i]))
        {
            return 0;
        }
    }
    if (z_find_std_dir(file + len) != file + len)
    {
        return 0;
    }
    return !cfg->input_file || strcmp(file, cfg->input_file) != 0;
}

static int tc_in_std(TypeChecker *tc, Token t)
{
    return tc_file_in_std(tc, t.file);
}

int tc_storage_depth(TypeChecker *tc, ASTNode *place)
{
    if (!place)
    {
        return tc->current_depth;
    }
    switch (place->kind)
    {
    case NODE_EXPR_VAR:
    {
        ZenSymbol *sym = tc_lookup(tc, place->var_ref.name);
        return sym ? sym->scope_depth : tc->current_depth;
    }
    case NODE_EXPR_MEMBER:
    {
        ASTNode *holder = place->member.target;
        Type *holder_type = get_inner_type(holder ? holder->type_info : NULL);
        if (holder_type && holder_type->kind == TYPE_POINTER)
        {
            return holder->type_info->lifetime_depth;
        }
        return tc_storage_depth(tc, holder);
    }
    case NODE_EXPR_INDEX:
    {
        ASTNode *holder = place->index.array;
        Type *holder_type = get_inner_type(holder ? holder->type_info : NULL);
        if (holder_type && holder_type->kind == TYPE_ARRAY)
        {
            return tc_storage_depth(tc, holder);
        }
        return holder && holder->type_info ? holder->type_info->lifetime_depth : tc->current_depth;
    }
    case NODE_EXPR_UNARY:
        if (place->unary.op == OP_DEREF && place->unary.operand && place->unary.operand->type_info)
        {
            return place->unary.operand->type_info->lifetime_depth;
        }
        return tc->current_depth;
    default:
        return tc->current_depth;
    }
}

// A type error stops `zc check`; in `zc build` (0.5) it is a warning that says so.
static int type_error_as_warning(TypeChecker *tc, Token t, const char *msg, const char *hint)
{
    if (!tc->type_errors_as_warnings)
    {
        return 0;
    }
    char text[MAX_ERROR_MSG_LEN];
    snprintf(text, sizeof(text), "%s (an error from 0.6; `zc check` reports it now)", msg);
    if (hint)
    {
        zwarn_with_suggestion(t, text, hint);
    }
    else
    {
        zwarn_at(t, "%s", text);
    }
    tc->warning_count++;
    return 1;
}

void tc_error(TypeChecker *tc, Token t, const char *msg)
{
    if (tc->rechecking || tc_in_std(tc, t) || type_error_as_warning(tc, t, msg, NULL))
    {
        return;
    }
    zerror_at(t, "%s", msg);
}

int is_expression_invariant(TypeChecker *tc, ASTNode *node, int *val)
{
    if (!node)
    {
        return 0;
    }
    long long out;
    if (eval_const_int_expr(node, tc->pctx, &out))
    {
        if (val)
        {
            *val = (int)out;
        }
        return 1;
    }
    return 0;
}

// Global recursion guard

void tc_warning(TypeChecker *tc, Token t, const char *msg)
{
    if (tc_in_std(tc, t))
    {
        return;
    }
    zwarn_at(t, "%s", msg);
    tc->warning_count++;
}

void tc_escape_error(TypeChecker *tc, Token t, const char *msg, const char *const *hints)
{
    for (int i = 0; i < tc->escapes_reported_count; i++)
    {
        if (tc->escapes_reported[i] == t.start)
        {
            return;
        }
    }
    tc->escapes_reported = xrealloc(
        tc->escapes_reported, sizeof(const char *) * (size_t)(tc->escapes_reported_count + 1));
    tc->escapes_reported[tc->escapes_reported_count++] = t.start;

    int prev_rechecking = tc->rechecking;
    tc->rechecking = 0;
    tc_error_with_hints(tc, t, msg, hints);
    tc->rechecking = prev_rechecking;
}

void tc_unreachable_code(TypeChecker *tc, Token t)
{
    if (tc->pctx->config->misra_mode)
    {
        const char *hints[] = {"Remove unreachable code or restructure control flow", NULL};
        tc_error_with_hints(tc, t, "MISRA Rule 2.1: Unreachable code detected", hints);
        return;
    }
    // The second pass over a loop body sees the same statements again.
    if (tc->in_loop_pass2 || tc_in_std(tc, t))
    {
        return;
    }
    warn_unreachable_code(t);
    tc->warning_count++;
}

void tc_error_with_hints(TypeChecker *tc, Token t, const char *msg, const char *const *hints)
{
    if (tc->rechecking || tc_in_std(tc, t) ||
        type_error_as_warning(tc, t, msg, hints ? hints[0] : NULL))
    {
        return;
    }
    zerror_with_hints(t, msg, hints);
}

void tc_error_from_0_6(TypeChecker *tc, Token t, const char *msg, const char *const *hints)
{
#if FF_COMPATIBILITY_0_5
    if (tc->rechecking || tc_in_std(tc, t))
    {
        return;
    }
    char text[MAX_ERROR_MSG_LEN];
    snprintf(text, sizeof(text), "%s (an error from 0.6)", msg);
    if (hints && hints[0])
    {
        zwarn_with_suggestion(t, text, hints[0]);
    }
    else
    {
        zwarn_at(t, "%s", text);
    }
    tc->warning_count++;
#else
    tc_error_with_hints(tc, t, msg, hints);
#endif
}

void tc_ctrl_flow_error(TypeChecker *tc, Token t, const char *msg, const char *const *hints)
{
    // Control flow errors (missing return, unreachable code, etc.)
    // are reported even during move-only checking.
    if (tc_in_std(tc, t))
    {
        return;
    }
    zerror_with_hints(t, msg, hints);
}

void tc_move_error_with_hints(TypeChecker *tc, Token t, const char *msg, const char *const *hints,
                              const DiagnosticNote *note)
{
    if (tc_in_std(tc, t))
    {
        return;
    }
    zerror_with_note(t, msg, hints, note);
}

// tc_check_misra_10_4 moved to misra_check_binary_op_essential_types in misra.c

void tc_enter_scope(TypeChecker *tc)
{
    tc->current_depth++;
    enter_scope(tc->pctx);
}

void tc_exit_scope(TypeChecker *tc)
{
    if (tc->current_depth > 0)
    {
        tc->current_depth--;
    }
    exit_scope(tc->pctx);
}

ZenSymbol *tc_add_symbol(TypeChecker *tc, const char *name, Type *type, Token t, int is_immutable)
{
    CompilerConfig *cfg = &tc->pctx->compiler->config;
    if (cfg->misra_mode)
    {
        misra_check_shadowing(tc->pctx, name, t);
        misra_check_typographic_ambiguity(tc->pctx, name, t);
    }
    add_symbol_with_token(tc->pctx, name, type, t, 0);
    ZenSymbol *sym = symbol_lookup(tc->pctx->current_scope, name);
    if (sym)
    {
        sym->is_immutable = is_immutable;
        sym->scope_depth = tc->current_depth;
    }
    return sym;
}

CFunctionLocals *tc_enter_c_function(TypeChecker *tc, ASTNode *function)
{
    CFunctionLocals *previous = tc->c_function;
    CFunctionLocals *f = tc->c_functions;
    while (f && f->function != function)
    {
        f = f->next;
    }
    if (!f)
    {
        f = xcalloc(1, sizeof(CFunctionLocals));
        f->function = function;
        f->next = tc->c_functions;
        tc->c_functions = f;
    }
    tc->c_function = f;
    return previous;
}

void tc_leave_c_function(TypeChecker *tc, CFunctionLocals *previous)
{
    tc->c_function = previous;
}

// Whether `sym` is a local of the C function `f`.
static int is_local_of(CFunctionLocals *f, ZenSymbol *sym)
{
    for (int i = 0; i < f->count; i++)
    {
        if (f->locals[i].symbol == sym)
        {
            return 1;
        }
    }
    return 0;
}

// Whether a local of `f` seen from the current scope has the C name `c_name`: a local declared
// with it would hide that one in C, which C does not allow in every place (a lambda's body is
// the block of its parameters; in C++ the body of a `for` cannot redeclare its variable).
static int local_c_name_visible(TypeChecker *tc, CFunctionLocals *f, const char *c_name)
{
    for (Scope *scope = tc->pctx->current_scope; scope; scope = scope->parent)
    {
        for (ZenSymbol *sym = scope->symbols; sym; sym = sym->next)
        {
            const char *sym_c_name = sym->link_name ? sym->link_name : sym->name;
            if (sym_c_name && strcmp(sym_c_name, c_name) == 0 && is_local_of(f, sym))
            {
                return 1;
            }
        }
    }
    return 0;
}

// Whether `expr` reads the name `name`.
static int reads_name(ASTNode *expr, const char *name)
{
    if (!expr)
    {
        return 0;
    }
    char **refs = NULL;
    int count = 0;
    find_var_refs(expr, &refs, &count);
    for (int i = 0; i < count; i++)
    {
        if (strcmp(refs[i], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// The local of `f` declared at `site` with `name`: the one recorded before (a loop's second pass),
// or a new one with its C name. The C name is the name, unless the local hides a local of `f`
// that C would still see (local_c_name_visible), or hides a name its initializer reads (`let x =
// x + 1`, `for i in 0..i`): then `name__N`, with the first N that hides nothing.
static LocalCName *local_c_name(TypeChecker *tc, CFunctionLocals *f, const char *site,
                                const char *name, ASTNode *const *reads, int read_count)
{
    for (int i = 0; i < f->count; i++)
    {
        if (f->locals[i].site == site && strcmp(f->locals[i].name, name) == 0)
        {
            return &f->locals[i];
        }
    }
    const char *c_name = name;
    int hides_what_it_reads = 0;
    if (symbol_lookup(tc->pctx->current_scope, name))
    {
        for (int i = 0; i < read_count && !hides_what_it_reads; i++)
        {
            hides_what_it_reads = reads_name(reads[i], name);
        }
    }
    if (local_c_name_visible(tc, f, name) || hides_what_it_reads)
    {
        size_t size = strlen(name) + 16;
        char *candidate = xmalloc(size);
        for (int n = 1;; n++)
        {
            snprintf(candidate, size, "%s__%d", name, n);
            if (!local_c_name_visible(tc, f, candidate) &&
                !symbol_lookup(tc->pctx->current_scope, candidate))
            {
                break;
            }
        }
        c_name = candidate;
        record_local_c_name(tc->pctx, site, name, c_name);
    }
    if (f->count == f->capacity)
    {
        f->capacity = f->capacity ? f->capacity * 2 : 8;
        f->locals = xrealloc(f->locals, (size_t)f->capacity * sizeof(LocalCName));
    }
    f->locals[f->count] = (LocalCName){site, name, c_name, NULL};
    return &f->locals[f->count++];
}

ZenSymbol *tc_declare_local(TypeChecker *tc, const char *name, Type *type, Token t,
                            int is_immutable, ASTNode *const *reads, int read_count)
{
    LocalCName *local = tc->c_function && name
                            ? local_c_name(tc, tc->c_function, t.start, name, reads, read_count)
                            : NULL;
    ZenSymbol *sym = tc_add_symbol(tc, name, type, t, is_immutable);
    if (local)
    {
        local->symbol = sym;
        if (sym && strcmp(local->c_name, name) != 0)
        {
            sym->link_name = (char *)local->c_name;
        }
    }
    return sym;
}

ZenSymbol *tc_lookup(TypeChecker *tc, const char *name)
{
    ZenSymbol *sym = symbol_lookup(tc->pctx->current_scope, name);
    if (sym)
    {
        sym->is_used = 1;
    }
    return sym;
}

// Marks the struct or enum `name` as used, and the template it is an instance of
// (`Maybe__int32_t` uses `Maybe`): MISRA Rule 2.4 reports the ones a program never uses. The
// kind of a Type does not tell a struct from an enum (the type of `m: Mode` is a TYPE_STRUCT
// even when Mode is an enum), so the name is looked up as either.
static void mark_struct_or_enum_used(TypeChecker *tc, const char *name)
{
    ZenSymbol *sym = symbol_lookup_kind(tc->pctx->global_scope, name, SYM_STRUCT);
    if (!sym)
    {
        sym = symbol_lookup_kind(tc->pctx->global_scope, name, SYM_ENUM);
    }
    if (sym)
    {
        sym->is_used = 1;
    }
    Instantiation *inst = find_instantiation(tc->pctx, name);
    if (inst && inst->template_name && strcmp(inst->template_name, name) != 0)
    {
        mark_struct_or_enum_used(tc, inst->template_name);
    }
}

void mark_type_as_used(TypeChecker *tc, Type *t)
{
    if (!t)
    {
        return;
    }

    // Unroll pointers, arrays, vectors
    Type *curr = t;
    while (curr &&
           (curr->kind == TYPE_POINTER || curr->kind == TYPE_ARRAY || curr->kind == TYPE_VECTOR))
    {
        curr = curr->inner;
    }

    if (!curr)
    {
        return;
    }

    if ((curr->kind == TYPE_STRUCT || curr->kind == TYPE_ENUM) && curr->name)
    {
        mark_struct_or_enum_used(tc, curr->name);
    }
    else if (curr->kind == TYPE_ALIAS)
    {
        if (curr->name)
        {
            ZenSymbol *sym = symbol_lookup_kind(tc->pctx->global_scope, curr->name, SYM_ALIAS);
            if (sym)
            {
                sym->is_used = 1;
            }
        }
    }

    // Generic arguments
    for (int i = 0; curr->args && i < curr->count; i++)
    {
        mark_type_as_used(tc, curr->args[i]);
    }

    // Function type return and args
    if (curr->kind == TYPE_FUNCTION)
    {
        mark_type_as_used(tc, curr->inner);
        for (int i = 0; curr->args && i < curr->count; i++)
        {
            mark_type_as_used(tc, curr->args[i]);
        }
    }
}

// Internal MISRA helpers moved to platform/misra.c

// The width of the register an asm operand of type `t` takes: its own, at least 32 bits.
int get_asm_register_size(Type *t)
{
    if (!t)
    {
        return 0;
    }
    int width = resolve_alias(t)->kind == TYPE_F64 ? 64 : integer_type_width(t);
    return width > 32 ? width : 32;
}

// EXPRESSION CHECKERS
