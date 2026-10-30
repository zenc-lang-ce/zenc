// SPDX-License-Identifier: MIT
// Unit tests of src/codegen/codegen_decl_emit.c.
#include "unit_parse.h"
#include "codegen/codegen.h"
#include <stdio.h>
#include <string.h>
#include "unit_test.h"

// emit_impl_vtables used to keep the emitted (trait, struct) pairs in a fixed array of
// 2048 entries and aborted past it.
UTEST(emit_impl_vtables, emits_more_than_2048_vtables)
{
    const int count = 2100;
    size_t cap = (size_t)count * 128 + 64;
    char *src = xmalloc(cap);
    size_t len = (size_t)snprintf(src, cap, "trait Shape { fn area(self) -> int; }\n");
    for (int i = 0; i < count; i++)
    {
        len += (size_t)snprintf(src + len, cap - len,
                                "struct S%d { x: int; }\n"
                                "impl Shape for S%d { fn area(self) -> int { return self.x; } }\n",
                                i, i);
    }
    ASSERT_LT(len, cap);

    parse_source(src);
    ParserContext *ctx = parse_source_context();
    emitter_init_buffer(&ctx->cg.emitter);
    emit_impl_vtables(ctx);
    char *out = emitter_take_string(&ctx->cg.emitter);

    ASSERT_TRUE(strstr(out, "Shape_VTable S0__Shape__VTable") != NULL);
    ASSERT_TRUE(strstr(out, "Shape_VTable S2099__Shape__VTable") != NULL);
}
