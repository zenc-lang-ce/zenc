// SPDX-License-Identifier: MIT
#include "plugins/plugin_manager.h"
#include "parser.h"
#include "utils/format_expr.h"
#include "utils/colors.h"
#include "utils/utils.h"
#include "constants.h"
#include "ast/primitives.h"
#include <ctype.h>
#include "analysis/const_fold.h"

void register_type_usage(ParserContext *ctx, const char *name, Token t)
{
    if (ctx->is_speculative)
    {
        return;
    }

    TypeUsage *u = xmalloc(sizeof(TypeUsage));
    u->name = xstrdup(name);
    u->location = t;
    u->next = ctx->pending_type_validations;
    ctx->pending_type_validations = u;
}

int is_known_type_name(ParserContext *ctx, const char *file, const char *name)
{
    if (find_struct_def(ctx, name) || find_type_alias(ctx, name) || imports_name(ctx, file, name) ||
        is_extern_symbol(ctx, name) || is_trait(ctx, name) ||
        find_primitive_kind(name) != TYPE_UNKNOWN)
    {
        return 1;
    }
    for (char **listed = ctx->config->c_type_whitelist; listed && *listed; listed++)
    {
        if (strcmp(name, *listed) == 0)
        {
            return 1;
        }
    }
    return 0;
}
