// SPDX-License-Identifier: MIT
#include "../constants.h"
#include "move_check.h"
#include "../parser/parser.h"
#include "typecheck.h"
#include "../diagnostics/diagnostics.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zprep.h"

MoveState *move_state_create(MoveState *parent)
{
    MoveState *s = xmalloc(sizeof(MoveState));
    s->entries = NULL;
    s->parent = parent;
    return s;
}

MoveState *move_state_clone(MoveState *src)
{
    if (!src)
    {
        return NULL;
    }
    MoveState *new_state = move_state_create(src->parent);

    MoveEntry *curr = src->entries;
    MoveEntry **tail = &new_state->entries;
    while (curr)
    {
        MoveEntry *new_entry = xmalloc(sizeof(MoveEntry));
        new_entry->symbol_name = xstrdup(curr->symbol_name);
        new_entry->status = curr->status;
        new_entry->moved_at = curr->moved_at;
        new_entry->next = NULL;
        *tail = new_entry;
        tail = &new_entry->next;
        curr = curr->next;
    }
    return new_state;
}

void move_state_free(MoveState *state)
{
    if (!state)
    {
        return;
    }
    MoveEntry *e = state->entries;
    while (e)
    {
        MoveEntry *next = e->next;
        zfree(e->symbol_name);
        zfree(e);
        e = next;
    }
    zfree(state);
}

char *get_node_path(ASTNode *node, int depth, ParserContext *ctx)
{
    if (!node || depth > 32)
    {
        return NULL;
    }
    RECURSION_GUARD_TOKEN(ctx, node->token, NULL);

    char *path = NULL;

    if (node->kind == NODE_EXPR_VAR)
    {
        path = xstrdup(node->var_ref.name);
    }
    else if (node->kind == NODE_EXPR_MEMBER)
    {
        // `(*pp).items` is the place `pp.items` is: `.` reaches the field through every
        // pointer, and checking turns the target `pp` of `pp.items` into `*pp`
        // (deref_to_one_pointer).
        ASTNode *target = node->member.target;
        while (target && target->kind == NODE_EXPR_UNARY && target->unary.op &&
               unary_operator_of(target->unary.op) == OP_DEREF)
        {
            target = target->unary.operand;
        }
        char *target_path = get_node_path(target, depth + 1, ctx);
        if (target_path)
        {
            char buffer[MAX_ERROR_MSG_LEN];
            snprintf(buffer, sizeof(buffer), "%s.%s", target_path, node->member.field);
            zfree(target_path);
            path = xstrdup(buffer);
        }
    }

    RECURSION_EXIT(ctx);
    return path;
}

static void mark_moved_in_state(MoveState *state, const char *path, Token t)
{
    if (!state || !path)
    {
        return;
    }

    MoveEntry *e = state->entries;
    while (e)
    {
        if (strcmp(e->symbol_name, path) == 0)
        {
            e->status = MOVE_STATE_MOVED;
            e->moved_at = t;
            return;
        }
        e = e->next;
    }

    MoveEntry *new_entry = xmalloc(sizeof(MoveEntry));
    new_entry->symbol_name = xstrdup(path);
    new_entry->status = MOVE_STATE_MOVED;
    new_entry->moved_at = t;
    new_entry->next = state->entries;
    state->entries = new_entry;
}

// The entry of `path` in `state` or in the states it is nested in; NULL when there is none.
static MoveEntry *find_move_entry(MoveState *state, const char *path)
{
    for (MoveState *s = path ? state : NULL; s; s = s->parent)
    {
        for (MoveEntry *e = s->entries; e; e = e->next)
        {
            if (strcmp(e->symbol_name, path) == 0)
            {
                return e;
            }
        }
    }
    return NULL;
}

MoveStatus get_move_status(MoveState *state, const char *path)
{
    MoveEntry *e = find_move_entry(state, path);
    return e ? e->status : MOVE_STATE_VALID;
}

void move_state_merge(MoveState *target, MoveState *a, MoveState *b)
{
    if (!target)
    {
        return;
    }

    if (a)
    {
        MoveEntry *e = a->entries;
        while (e)
        {
            MoveStatus status_a = e->status;
            MoveStatus status_b = b ? get_move_status(b, e->symbol_name) : MOVE_STATE_VALID;

            if (status_a == MOVE_STATE_MOVED || status_b == MOVE_STATE_MOVED)
            {
                mark_moved_in_state(target, e->symbol_name, e->moved_at);
            }
            e = e->next;
        }
    }

    if (b)
    {
        MoveEntry *e = b->entries;
        while (e)
        {
            MoveStatus status_b = e->status;
            MoveStatus status_a = a ? get_move_status(a, e->symbol_name) : MOVE_STATE_VALID;

            if (status_b == MOVE_STATE_MOVED || status_a == MOVE_STATE_MOVED)
            {
                mark_moved_in_state(target, e->symbol_name, e->moved_at);
            }
            e = e->next;
        }
    }
}

void move_state_merge_into(MoveState **target, MoveState *src)
{
    if (!src)
    {
        return;
    }

    if (!*target)
    {
        *target = move_state_clone(src);
        return;
    }

    MoveEntry *e = src->entries;
    while (e)
    {
        if (e->status == MOVE_STATE_MOVED)
        {
            mark_moved_in_state(*target, e->symbol_name, e->moved_at);
        }
        e = e->next;
    }
}

int is_type_copy(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return 1; // Default to Copy for unknown types
    }
    RECURSION_GUARD_TOKEN(ctx, TOKEN_UNKNOWN, 1);

    int result = 1;
    if (t->traits.has_drop)
    {
        result = 0;
    }
    else if (t->name && check_impl(ctx, "Drop", type_def_name(t)))
    {
        result = 0;
    }
    else
    {
        switch (t->kind)
        {
        case TYPE_INT:
        case TYPE_I8:
        case TYPE_I16:
        case TYPE_I32:
        case TYPE_I64:
        case TYPE_U8:
        case TYPE_U16:
        case TYPE_U32:
        case TYPE_U64:
        case TYPE_F32:
        case TYPE_F64:
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_VOID:
        case TYPE_POINTER:
        case TYPE_FUNCTION:
            result = 1;
            break;
        case TYPE_BITINT:
        case TYPE_UBITINT:
            result = 1;
            break;
        case TYPE_ENUM:
            // An enum with Drop, or with a payload that needs dropping, is moved.
            result = !type_needs_drop(ctx, t);
            break;
        case TYPE_STRUCT:
            if (check_impl(ctx, "Copy", type_def_name(t)))
            {
                result = 1;
            }
            else if (check_impl(ctx, "Drop", type_def_name(t)))
            {
                result = 0;
            }
            else if (t->name)
            {
                ASTNode *def = find_struct_def(ctx, type_def_name(t));
                if (def && def->type_info && def->type_info->traits.has_drop)
                {
                    result = 0;
                }
                else
                {
                    result = 1;
                }
            }
            else
            {
                result = 1;
            }
            break;
        case TYPE_ARRAY:
            result = is_type_copy(ctx, t->inner);
            break;
        case TYPE_TUPLE:
            for (int i = 0; i < t->count && result; i++)
            {
                result = is_type_copy(ctx, t->args[i]);
            }
            break;
        case TYPE_ALIAS:
            if (t->alias.is_opaque_alias)
            {
                result = 1;
            }
            else
            {
                result = is_type_copy(ctx, t->inner);
            }
            break;
        default:
            result = 1;
            break;
        }
    }
    RECURSION_EXIT(ctx);
    return result;
}

void check_path_validity(TypeChecker *tc, const char *path, Token t)
{
    if (!path)
    {
        return;
    }

    // Check Flow-Sensitive State
    ParserContext *ctx = tc->pctx;
    MoveEntry *entry = ctx && ctx->move_state ? find_move_entry(ctx->move_state, path) : NULL;
    MoveStatus status = entry ? entry->status : MOVE_STATE_VALID;

    if (status == MOVE_STATE_MOVED || status == MOVE_STATE_MAYBE_MOVED)
    {
        if (tc && tc->in_loop_pass2)
        {
            if (t.line == 0)
            {
                return;
            }
            if (tc->loop_start_state)
            {
                MoveStatus start_status = get_move_status(tc->loop_start_state, path);
                if (start_status == MOVE_STATE_MOVED || start_status == MOVE_STATE_MAYBE_MOVED)
                {
                    return;
                }
            }
        }

        char msg[MAX_ERROR_MSG_LEN];
        snprintf(msg, 255, "Use of moved value '%s'", path);

        // Where it was moved: the use alone does not say which line to change.
        Token m = entry->moved_at;
        int known = m.line > 0;
        // The use moves it too, on the iteration before: `take(v)` in a loop.
        int same_place = known && m.line == t.line && m.col == t.col &&
                         (!m.file || !t.file || strcmp(m.file, t.file) == 0);
        char moved_here[MAX_ERROR_MSG_LEN];
        snprintf(moved_here, sizeof(moved_here),
                 same_place ? "'%s' was moved here in an earlier iteration of the loop"
                            : "'%s' was moved here",
                 path);
        DiagnosticNote note = {m, moved_here};

        const char *hints[4];
        int hint_count = 0;
        if (same_place)
        {
            hints[hint_count++] = moved_here;
        }
        hints[hint_count++] = "This type owns resources and cannot be implicitly copied";
        hints[hint_count++] = "Consider using a reference ('&') to borrow the value instead";
        hints[hint_count] = NULL;
        tc_move_error_with_hints(tc, t, msg, hints, known && !same_place ? &note : NULL);
    }
}

void check_use_validity(TypeChecker *tc, ASTNode *use_node)
{
    if (!use_node)
    {
        return;
    }

    // The receiver of a .forget() call is expected to have been moved already
    // (pattern: `vec.push(x); x.forget();`), so do not report use-after-move.
    if (tc->is_forget_receiver)
    {
        return;
    }

    char *path = get_node_path(use_node, 0, tc->pctx);
    if (!path && use_node->kind == NODE_EXPR_VAR)
    {
        path = xstrdup(use_node->var_ref.name);
    }

    if (!path)
    {
        return;
    }

    check_path_validity(tc, path, use_node->token);
    zfree(path);
}

void mark_symbol_moved(ParserContext *ctx, ZenSymbol *sym, ASTNode *context_node)
{
    if (!context_node)
    {
        return;
    }

    Type *t = context_node->type_info ? context_node->type_info : (sym ? sym->type_info : NULL);
    int copy = is_type_copy(ctx, t);

    if (t && ctx && !copy)
    {
        if (sym)
        {
            sym->is_moved = 1;
        }

        if (ctx->move_state)
        {
            char *path = get_node_path(context_node, 0, ctx);
            if (!path && sym)
            {
                path = xstrdup(sym->name);
            }

            if (path)
            {
                mark_moved_in_state(ctx->move_state, path, context_node->token);
                zfree(path);
            }
        }
    }
}

static void mark_valid_in_state(MoveState *state, const char *path, Token t)
{
    if (!state || !path)
    {
        return;
    }

    MoveEntry *e = state->entries;
    while (e)
    {
        if (strcmp(e->symbol_name, path) == 0)
        {
            e->status = MOVE_STATE_VALID;
            e->moved_at = t;
            return;
        }
        e = e->next;
    }

    MoveEntry *new_entry = xmalloc(sizeof(MoveEntry));
    new_entry->symbol_name = xstrdup(path);
    new_entry->status = MOVE_STATE_VALID;
    new_entry->moved_at = t;
    new_entry->next = state->entries;
    state->entries = new_entry;
}

void mark_symbol_valid(ParserContext *ctx, ZenSymbol *sym, ASTNode *context_node)
{
    if (sym)
    {
        sym->is_moved = 0;
    }

    if (ctx && ctx->move_state)
    {
        char *path = get_node_path(context_node, 0, ctx);
        if (!path && sym)
        {
            path = xstrdup(sym->name);
        }

        if (path)
        {
            mark_valid_in_state(ctx->move_state, path,
                                context_node ? context_node->token
                                             : (sym ? sym->decl_token : TOKEN_UNKNOWN));
            zfree(path);
        }
    }
}
