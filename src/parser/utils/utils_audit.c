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

void audit_section_5(ParserContext *ctx, Scope *scope, const char *name, const char *link_name,
                     Token tok)
{
    if (!scope || !name)
    {
        return;
    }
    if (strcmp(name, "it") == 0 || strcmp(name, "self") == 0)
    {
        return;
    }

    if (!ctx->config->misra_mode)
    {
        // Only shadowing is reported: each symbol of this name in the scopes around this one.
        if (!ctx->silent_warnings)
        {
            for (Scope *p = scope->parent; p; p = p->parent)
            {
                for (ZenSymbol *sh = symbol_lookup_local(p, name); sh; sh = sh->next_same_name)
                {
                    warn_shadowing(tok, name);
                }
            }
        }
        return;
    }

    if (ctx->hook_check_standard_macro_name)
    {
        ctx->hook_check_standard_macro_name(tok, name);
    }

    // MISRA also compares the name with every symbol in scope, so it walks them all.
    Scope *p = scope;
    int limit = (p == ctx->global_scope) ? 31 : 63;
    const char *actual_name = link_name ? link_name : name;

    while (p)
    {
        ZenSymbol *sh = p->symbols;
        while (sh)
        {
            if (p != scope && strcmp(sh->name, name) == 0 && !ctx->silent_warnings)
            {
                zerror_at(tok, "MISRA Rule 5.3");
            }

            const char *sh_actual_name = sh->link_name ? sh->link_name : sh->name;
            if (strcmp(sh_actual_name, actual_name) != 0 && ctx->hook_check_identifier_collision)
            {
                ctx->hook_check_identifier_collision(tok, sh_actual_name, actual_name, limit);
            }

            sh = sh->next;
        }
        p = p->parent;
    }
}
