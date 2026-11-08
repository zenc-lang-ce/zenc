// SPDX-License-Identifier: MIT
// Removing the functions of a whole program that nothing reaches, from the C file just written.
//
// zc emits every method of every instance of a generic type and every function of the std
// modules a program imports. In a whole program (see builds_whole_program in the driver) only
// the generated C can call them, so the ones that nothing reaches from the text outside the
// functions (main, tests, vtables, drop glue, raw blocks, initializers) can go. The references
// are read from the C itself: it names every function it calls, including the calls codegen
// adds (drop, iterators, operators) and the ones inside raw blocks, which the AST does not show.
#include "codegen.h"
#include "../parser/parser.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    const char *name;
    int *calls; ///< Indices of the functions its definitions name.
    int call_count;
    int call_cap;
    int reachable;
} PrunedFunction;

typedef struct
{
    PrunedFunction *functions;
    int count;
} FunctionTable;

static int compare_names(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

// The index of the identifier text[0..len) in the table, or -1.
static int find_function(const FunctionTable *table, const char *text, size_t len)
{
    int lo = 0;
    int hi = table->count - 1;
    while (lo <= hi)
    {
        int mid = lo + (hi - lo) / 2;
        const char *name = table->functions[mid].name;
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

static void add_call(PrunedFunction *caller, int callee)
{
    for (int i = 0; i < caller->call_count; i++)
    {
        if (caller->calls[i] == callee)
        {
            return;
        }
    }
    if (caller->call_count == caller->call_cap)
    {
        caller->call_cap = caller->call_cap ? caller->call_cap * 2 : 8;
        caller->calls = xrealloc(caller->calls, sizeof(int) * (size_t)caller->call_cap);
    }
    caller->calls[caller->call_count++] = callee;
}

// Every function named in text[start, end): marked reachable (caller NULL), or recorded as a
// call of `caller`.
static void scan_names(FunctionTable *table, const char *text, long start, long end,
                       PrunedFunction *caller, int *stack, int *stack_len)
{
    long i = start;
    while (i < end)
    {
        if (!is_identifier_char(text[i]) || (i > start && is_identifier_char(text[i - 1])))
        {
            i++;
            continue;
        }
        long j = i;
        while (j < end && is_identifier_char(text[j]))
        {
            j++;
        }
        int index =
            isdigit((unsigned char)text[i]) ? -1 : find_function(table, text + i, (size_t)(j - i));
        if (index >= 0)
        {
            if (caller)
            {
                add_call(caller, index);
            }
            else if (!table->functions[index].reachable)
            {
                table->functions[index].reachable = 1;
                stack[(*stack_len)++] = index;
            }
        }
        i = j;
    }
}

static char *read_file(const char *path, long *size)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *text = xmalloc((size_t)*size + 1);
    size_t got = fread(text, 1, (size_t)*size, f);
    fclose(f);
    if ((long)got != *size)
    {
        return NULL;
    }
    text[*size] = '\0';
    return text;
}

int prune_unreachable_functions(ParserContext *ctx, const char *path)
{
    FunctionSpan *spans = ctx->cg.function_spans;
    int span_count = ctx->cg.function_span_count;
    if (span_count == 0)
    {
        return 0;
    }
    long size = 0;
    char *text = read_file(path, &size);
    if (!text)
    {
        return -1;
    }
    // The spans come in the order they were written; one that does not start after the previous
    // one ends means something was written out of order: leave the file as it is.
    for (int s = 0; s < span_count; s++)
    {
        if (spans[s].start > spans[s].end || spans[s].end > size ||
            (s > 0 && spans[s].start < spans[s - 1].end))
        {
            return -1;
        }
    }

    const char **names = xmalloc(sizeof(char *) * (size_t)span_count);
    for (int s = 0; s < span_count; s++)
    {
        names[s] = spans[s].name;
    }
    qsort(names, (size_t)span_count, sizeof(char *), compare_names);
    FunctionTable table = {xcalloc((size_t)span_count, sizeof(PrunedFunction)), 0};
    for (int s = 0; s < span_count; s++)
    {
        if (table.count == 0 || strcmp(table.functions[table.count - 1].name, names[s]) != 0)
        {
            table.functions[table.count++].name = names[s];
        }
    }

    int *stack = xmalloc(sizeof(int) * (size_t)table.count);
    int stack_len = 0;
    long outside = 0;
    for (int s = 0; s < span_count; s++)
    {
        scan_names(&table, text, outside, spans[s].start, NULL, stack, &stack_len);
        if (spans[s].is_definition)
        {
            int self = find_function(&table, spans[s].name, strlen(spans[s].name));
            scan_names(&table, text, spans[s].start, spans[s].end, &table.functions[self], stack,
                       &stack_len);
        }
        outside = spans[s].end;
    }
    scan_names(&table, text, outside, size, NULL, stack, &stack_len);

    while (stack_len > 0)
    {
        PrunedFunction *f = &table.functions[stack[--stack_len]];
        for (int c = 0; c < f->call_count; c++)
        {
            if (!table.functions[f->calls[c]].reachable)
            {
                table.functions[f->calls[c]].reachable = 1;
                stack[stack_len++] = f->calls[c];
            }
        }
    }

    FILE *out = fopen(path, "wb");
    if (!out)
    {
        return -1;
    }
    int removed = 0;
    long from = 0;
    for (int s = 0; s < span_count; s++)
    {
        int index = find_function(&table, spans[s].name, strlen(spans[s].name));
        if (table.functions[index].reachable)
        {
            continue;
        }
        fwrite(text + from, 1, (size_t)(spans[s].start - from), out);
        from = spans[s].end;
        removed += spans[s].is_definition;
    }
    fwrite(text + from, 1, (size_t)(size - from), out);
    fclose(out);
    return removed;
}
