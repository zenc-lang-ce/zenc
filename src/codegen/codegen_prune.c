// SPDX-License-Identifier: MIT
// Writing only the functions a whole program reaches.
//
// zc emits every method of every instance of a generic type and every function of the std
// modules a program imports, and most of them the program never calls. In a whole program, built
// with --prune-unused (see whole_program_obstacle in the driver), only the generated C can call
// them, so codegen writes a body only when the C already written names it. The references are
// read from the C itself: it names every function it calls, including the calls codegen adds
// (drop, iterators, operators) and the ones inside raw blocks, which the AST does not show.
#include "codegen.h"
#include "../parser/parser.h"
#include "../platform/os.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// A body emit_function_bodies may write: a function, a method of an impl, or another node of the
// list, written whole.
typedef struct
{
    ASTNode *top;     ///< The node of the list.
    ASTNode *method;  ///< The method of `top` (an impl) this body is, or NULL for `top` itself.
    const char *name; ///< Its C name when it may be left out; NULL when it is always written.
    int written;
    long start; ///< Its text in the scratch file: [start, end).
    long end;
} Body;

// The bodies that may be left out, by C name (several with one name under @cfg).
typedef struct
{
    const char *name;
    int first; ///< Index of its first body in the sorted order.
    int count;
    int reached;
} NamedBodies;

typedef struct
{
    Body *bodies;
    int body_count;
    int *by_name; ///< Indices of the omissible bodies, sorted by name.
    NamedBodies *names;
    int name_count;
    int *pending; ///< Indices into `names` reached and not yet written.
    int pending_count;
} BodySet;

static BodySet *sort_context;

static int compare_bodies_by_name(const void *a, const void *b)
{
    const Body *x = &sort_context->bodies[*(const int *)a];
    const Body *y = &sort_context->bodies[*(const int *)b];
    return strcmp(x->name, y->name);
}

// The index into set->names of the identifier text[0..len), or -1.
static int find_name(const BodySet *set, const char *text, size_t len)
{
    int lo = 0;
    int hi = set->name_count - 1;
    while (lo <= hi)
    {
        int mid = lo + (hi - lo) / 2;
        const char *name = set->names[mid].name;
        int cmp = strncmp(name, text, len);
        if (cmp == 0)
        {
            cmp = name[len] == '\0' ? 0 : 1;
        }
        if (cmp == 0)
        {
            return mid;
        }
        if (cmp < 0)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid - 1;
        }
    }
    return -1;
}

static int is_identifier_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

// Marks as reached every omissible function named in text[0..len).
static void reach_names_in(BodySet *set, const char *text, long len)
{
    long i = 0;
    while (i < len)
    {
        if (!is_identifier_char(text[i]))
        {
            i++;
            continue;
        }
        long j = i;
        while (j < len && is_identifier_char(text[j]))
        {
            j++;
        }
        int index =
            isdigit((unsigned char)text[i]) ? -1 : find_name(set, text + i, (size_t)(j - i));
        if (index >= 0 && !set->names[index].reached)
        {
            set->names[index].reached = 1;
            set->pending[set->pending_count++] = index;
        }
        i = j;
    }
}

// The position in `file` of what is read back later. Without it the bodies to write cannot be
// chosen, so it is an error of the system, like running out of memory.
static long position_in(FILE *file)
{
    long position = ftell(file);
    if (position < 0)
    {
        zfatal("could not read back the generated C: %s", strerror(errno));
    }
    return position;
}

// The bytes [start, end) of `file`.
static char *read_range(FILE *file, long start, long end)
{
    char *text = xmalloc((size_t)(end - start) + 1);
    fflush(file);
    fseek(file, start, SEEK_SET);
    size_t got = fread(text, 1, (size_t)(end - start), file);
    text[got] = '\0';
    fseek(file, 0, SEEK_END);
    return text;
}

// The C written so far, without the prototypes of the omissible functions: what it names is
// reached.
static void reach_names_in_written_text(ParserContext *ctx, BodySet *set)
{
    FILE *out = ctx->cg.emitter.file;
    fflush(out);
    long size = position_in(out);
    char *text = read_range(out, 0, size);
    long from = 0;
    for (int s = 0; s < ctx->cg.prototype_span_count; s++)
    {
        const PrototypeSpan *span = &ctx->cg.prototype_spans[s];
        reach_names_in(set, text + from, span->start - from);
        from = span->end;
    }
    reach_names_in(set, text + from, size - from);
}

// Writes `body` at the end of `scratch`, as the list of the program would write it.
static void write_body(ParserContext *ctx, Body *body, FILE *scratch)
{
    Emitter saved = ctx->cg.emitter;
    emitter_init_file(&ctx->cg.emitter, scratch);
    // A body may follow another than in the list: its first #line must not be skipped.
    forget_source_mapping();
    body->start = position_in(scratch);
    if (body->top->cfg_condition)
    {
        EMIT(ctx, "#if %s\n", body->top->cfg_condition);
    }
    if (body->method)
    {
        Type *target =
            body->top->kind == NODE_IMPL ? body->top->impl.target : body->top->impl_trait.target;
        ctx->cg.current_impl_type = type_impl_name(target);
        emit_source_mapping(ctx, body->method);
        codegen_node_single(ctx, body->method);
        ctx->cg.current_impl_type = NULL;
    }
    else
    {
        codegen_node_single(ctx, body->top);
    }
    if (body->top->cfg_condition)
    {
        EMIT(ctx, "#endif\n");
    }
    fflush(scratch);
    body->end = position_in(scratch);
    body->written = 1;
    ctx->cg.emitter = saved;
}

static void add_body(BodySet *set, int *cap, ParserContext *ctx, ASTNode *top, ASTNode *method)
{
    if (set->body_count == *cap)
    {
        *cap = *cap ? *cap * 2 : 256;
        set->bodies = xrealloc(set->bodies, sizeof(Body) * (size_t)*cap);
    }
    Body *body = &set->bodies[set->body_count++];
    memset(body, 0, sizeof(*body));
    body->top = top;
    body->method = method;
    ASTNode *func = method ? method : top;
    if (func->kind == NODE_FUNCTION && func->func.body && !func->func.generic_params)
    {
        body->name = omissible_function_name(ctx, func, NULL);
    }
}

// Writes every node of `funcs`, as when the program is not whole.
static void emit_all_bodies(ParserContext *ctx, ASTNode *funcs)
{
    for (ASTNode *node = funcs; node; node = node->next)
    {
        // An impl of a template is emitted with each instance.
        if ((node->kind == NODE_IMPL || node->kind == NODE_IMPL_TRAIT) &&
            impl_on_template(ctx, node))
        {
            continue;
        }
        if (node->cfg_condition)
        {
            EMIT(ctx, "#if %s\n", node->cfg_condition);
        }
        codegen_node_single(ctx, node);
        if (node->cfg_condition)
        {
            EMIT(ctx, "#endif\n");
        }
    }
}

void emit_function_bodies(ParserContext *ctx, ASTNode *funcs)
{
    // Without a scratch file, or when other code may call any function, every body is written.
    FILE *scratch =
        ctx->cg.emit_reached_only && ctx->cg.emitter.mode == EMITTER_FILE ? z_tmpfile() : NULL;
    if (!scratch)
    {
        emit_all_bodies(ctx, funcs);
        return;
    }
    BodySet set = {0};
    int cap = 0;
    for (ASTNode *node = funcs; node; node = node->next)
    {
        if (node->kind == NODE_IMPL || node->kind == NODE_IMPL_TRAIT)
        {
            // An impl of a template is emitted with each instance.
            if (impl_on_template(ctx, node))
            {
                continue;
            }
            ASTNode *methods =
                node->kind == NODE_IMPL ? node->impl.methods : node->impl_trait.methods;
            for (ASTNode *m = methods; m; m = m->next)
            {
                add_body(&set, &cap, ctx, node, m);
            }
        }
        else
        {
            add_body(&set, &cap, ctx, node, NULL);
        }
    }

    // The omissible bodies, grouped by name.
    set.by_name = xmalloc(sizeof(int) * (size_t)(set.body_count + 1));
    int omissible = 0;
    for (int b = 0; b < set.body_count; b++)
    {
        if (set.bodies[b].name)
        {
            set.by_name[omissible++] = b;
        }
    }
    sort_context = &set;
    qsort(set.by_name, (size_t)omissible, sizeof(int), compare_bodies_by_name);
    set.names = xcalloc((size_t)omissible + 1, sizeof(NamedBodies));
    for (int k = 0; k < omissible; k++)
    {
        const char *name = set.bodies[set.by_name[k]].name;
        if (set.name_count == 0 || strcmp(set.names[set.name_count - 1].name, name) != 0)
        {
            set.names[set.name_count].name = name;
            set.names[set.name_count].first = k;
            set.name_count++;
        }
        set.names[set.name_count - 1].count++;
    }
    set.pending = xmalloc(sizeof(int) * (size_t)(set.name_count + 1));

    reach_names_in_written_text(ctx, &set);
    // The bodies always written (main, @export, @constructor...) come first; then each body
    // the C written so far names.
    for (int b = 0; b < set.body_count; b++)
    {
        if (!set.bodies[b].name)
        {
            write_body(ctx, &set.bodies[b], scratch);
            char *text = read_range(scratch, set.bodies[b].start, set.bodies[b].end);
            reach_names_in(&set, text, set.bodies[b].end - set.bodies[b].start);
        }
    }
    while (set.pending_count > 0)
    {
        NamedBodies *named = &set.names[set.pending[--set.pending_count]];
        for (int k = named->first; k < named->first + named->count; k++)
        {
            Body *body = &set.bodies[set.by_name[k]];
            write_body(ctx, body, scratch);
            char *text = read_range(scratch, body->start, body->end);
            reach_names_in(&set, text, body->end - body->start);
        }
    }

    // The names written, sorted (set.names is), for remove_unused_prototypes, and those left out,
    // for the driver to report; then the bodies, in the order of the list.
    ctx->cg.written_bodies = xmalloc(sizeof(char *) * (size_t)(set.name_count + 1));
    ctx->cg.written_body_count = 0;
    ctx->cg.left_out_bodies = xmalloc(sizeof(char *) * (size_t)(set.name_count + 1));
    ctx->cg.left_out_body_count = 0;
    for (int n = 0; n < set.name_count; n++)
    {
        if (set.names[n].reached)
        {
            ctx->cg.written_bodies[ctx->cg.written_body_count++] = set.names[n].name;
        }
        else
        {
            ctx->cg.left_out_bodies[ctx->cg.left_out_body_count++] = set.names[n].name;
        }
    }
    for (int b = 0; b < set.body_count; b++)
    {
        Body *body = &set.bodies[b];
        if (body->written)
        {
            char *text = read_range(scratch, body->start, body->end);
            emitter_write(&ctx->cg.emitter, text, (size_t)(body->end - body->start));
        }
    }
    fclose(scratch);
}

static int compare_names(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

int remove_unused_prototypes(ParserContext *ctx, const char *path)
{
    if (ctx->cg.prototype_span_count == 0 || !ctx->cg.written_bodies)
    {
        return 0;
    }
    FILE *in = fopen(path, "rb");
    if (!in)
    {
        return -1;
    }
    fseek(in, 0, SEEK_END);
    long size = position_in(in);
    char *text = read_range(in, 0, size);
    fclose(in);

    FILE *out = fopen(path, "wb");
    if (!out)
    {
        return -1;
    }
    int removed = 0;
    long from = 0;
    for (int s = 0; s < ctx->cg.prototype_span_count; s++)
    {
        const PrototypeSpan *span = &ctx->cg.prototype_spans[s];
        const char *name = span->name;
        if (bsearch(&name, ctx->cg.written_bodies, (size_t)ctx->cg.written_body_count,
                    sizeof(char *), compare_names))
        {
            continue;
        }
        fwrite(text + from, 1, (size_t)(span->start - from), out);
        from = span->end;
        removed++;
    }
    fwrite(text + from, 1, (size_t)(size - from), out);
    fclose(out);
    return removed;
}
