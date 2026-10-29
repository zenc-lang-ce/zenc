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

// `not`, as a word or as `!`, in a @cfg condition.
static int is_cfg_not(Token t)
{
    return t.kind == TOK_NOT ||
           (t.kind == TOK_IDENT && t.len == 3 && strncmp(t.start, "not", 3) == 0);
}

// The C test for the define `name`: `defined(ZC_CFG_name)`, or its negation.
static char *cfg_defined(Token name, int negated)
{
    char *cond = xmalloc((size_t)name.len + 32);
    sprintf(cond, "%sdefined(ZC_CFG_%.*s)", negated ? "!" : "", (int)name.len,
            name.start); /* safe */
    return cond;
}

// `cond` joined to `acc` by `op` (" && ", " || "); just `cond` when `acc` is still empty.
static char *cfg_join(char *acc, const char *op, const char *cond)
{
    if (!acc)
    {
        return xstrdup(cond);
    }
    char *joined = xmalloc(strlen(acc) + strlen(op) + strlen(cond) + 1);
    sprintf(joined, "%s%s%s", acc, op, cond); /* safe */
    zfree(acc);
    return joined;
}

// The `(NAME, not(NAME), ...)` of @cfg(any(...)) or @cfg(all(...)), its terms joined by `op`.
// *out is NULL for an empty list. Returns 0 after reporting an error.
static int parse_cfg_list(Lexer *l, const char *list, const char *op, char **out)
{
    *out = NULL;
    if (lexer_peek(l).kind != TOK_LPAREN)
    {
        zpanic_at(lexer_peek(l), "Expected ( after %s in @cfg(%s(...))", list, list);
        return 0;
    }
    lexer_next(l);
    char *cond = NULL;
    while (1)
    {
        Token t = lexer_next(l);
        if (t.kind == TOK_EOF)
        {
            break;
        }
        if (is_cfg_not(t))
        {
            if (lexer_next(l).kind != TOK_LPAREN)
            {
                zpanic_at(lexer_peek(l), "Expected ( after not");
                return 0;
            }
            Token name = lexer_next(l);
            if (name.kind != TOK_IDENT)
            {
                zpanic_at(name, "Expected define name");
                return 0;
            }
            cond = cfg_join(cond, op, cfg_defined(name, 1));
            if (lexer_next(l).kind != TOK_RPAREN)
            {
                zpanic_at(lexer_peek(l), "Expected )");
                return 0;
            }
        }
        else if (t.kind == TOK_IDENT)
        {
            cond = cfg_join(cond, op, cfg_defined(t, 0));
        }
        else
        {
            zpanic_at(t, "Expected define name in @cfg(%s(...))", list);
            return 0;
        }
        if (lexer_peek(l).kind != TOK_COMMA)
        {
            break;
        }
        lexer_next(l);
    }
    if (lexer_next(l).kind != TOK_RPAREN)
    {
        zpanic_at(lexer_peek(l), "Expected ) after %s(...)", list);
        return 0;
    }
    *out = cond;
    return 1;
}

DeclarationAttributes parse_attributes(ParserContext *ctx, Lexer *l)
{
    (void)ctx;
    DeclarationAttributes res;
    memset(&res, 0, sizeof(res));
    res.derived_traits = xmalloc(sizeof(char *) * 32);

    // The attributes that only set a flag.
    struct
    {
        const char *name;
        int *flag;
    } flags[] = {
        {"required", &res.is_required},
        {"inline", &res.is_inline},
        {"noinline", &res.is_noinline},
        {"noreturn", &res.is_noreturn},
        {"cold", &res.is_cold},
        {"hot", &res.is_hot},
        {"constructor", &res.is_constructor},
        {"destructor", &res.is_destructor},
        {"unused", &res.is_unused},
        {"weak", &res.is_weak},
        {"export", &res.is_export},
        {"thread_local", &res.is_thread_local},
        {"comptime", &res.is_comptime},
        {"packed", &res.is_packed},
        {"pure", &res.is_pure},
        {"global", &res.cuda_global},
        {"device", &res.cuda_device},
        {"host", &res.cuda_host},
    };

    Token t = lexer_peek(l);
    while (t.kind == TOK_AT)
    {
        lexer_next(l);
        Token attr = lexer_next(l);
        if (attr.kind != TOK_IDENT && attr.kind != TOK_COMPTIME && attr.kind != TOK_ALIAS)
        {
            zpanic_at(attr, "Expected attribute name");
            return (DeclarationAttributes){0};
        }

        int *flag = NULL;
        for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
        {
            if (strlen(flags[i].name) == attr.len &&
                strncmp(attr.start, flags[i].name, attr.len) == 0)
            {
                flag = flags[i].flag;
            }
        }

        if (flag)
        {
            *flag = 1;
        }
        else if ((0 == strncmp(attr.start, "vector_size", 11) && 11 == attr.len) ||
                 (0 == strncmp(attr.start, "vector", 6) && 6 == attr.len))
        {
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Token num = lexer_next(l);
                if (num.kind == TOK_INT)
                {
                    char *tmp = token_strdup(num);
                    res.vector_size = (int)strtol(tmp, NULL, 10);
                    zfree(tmp);
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after vector size");
                    return (DeclarationAttributes){0};
                }
            }
        }
        else if (0 == strncmp(attr.start, "deprecated", 10) && 10 == attr.len)
        {
            res.is_deprecated = 1;
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Token msg = lexer_next(l);
                if (msg.kind == TOK_STRING)
                {
                    res.deprecated_msg = xmalloc(msg.len - 1);
                    strncpy(res.deprecated_msg, msg.start + 1, msg.len - 2);
                    res.deprecated_msg[msg.len - 2] = 0;
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after deprecated message");
                    return (DeclarationAttributes){0};
                }
            }
        }
        else if (0 == strncmp(attr.start, "section", 7) && 7 == attr.len)
        {
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Token sec = lexer_next(l);
                if (sec.kind == TOK_STRING)
                {
                    res.section = xmalloc(sec.len - 1);
                    strncpy(res.section, sec.start + 1, sec.len - 2);
                    res.section[sec.len - 2] = 0;
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after section name");
                    return (DeclarationAttributes){0};
                }
            }
            else
            {
                zpanic_at(lexer_peek(l), "@section requires a name: @section(\"name\")");
                return (DeclarationAttributes){0};
            }
        }
        else if (0 == strncmp(attr.start, "crepr", 5) && 5 == attr.len)
        {
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Token ct = lexer_next(l);
                if (ct.kind == TOK_STRING)
                {
                    res.crepr_c_type = xmalloc(ct.len - 1);
                    strncpy(res.crepr_c_type, ct.start + 1, ct.len - 2);
                    res.crepr_c_type[ct.len - 2] = 0;
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after crepr type name");
                    return (DeclarationAttributes){0};
                }
            }
            else
            {
                zpanic_at(lexer_peek(l), "@crepr requires a value: @crepr(\"C.kind\")");
                return (DeclarationAttributes){0};
            }
        }
        else if (0 == strncmp(attr.start, "align", 5) && 5 == attr.len)
        {
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Token num = lexer_next(l);
                if (num.kind == TOK_INT)
                {
                    res.align = (int)strtol(num.start, NULL, 10);
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after align value");
                    return (DeclarationAttributes){0};
                }
            }
            else
            {
                zpanic_at(lexer_peek(l), "@align requires a value: @align(N)");
                return (DeclarationAttributes){0};
            }
        }
        else if (0 == strncmp(attr.start, "cfg", 3) && 3 == attr.len)
        {
            if (lexer_peek(l).kind != TOK_LPAREN)
            {
                zpanic_at(lexer_peek(l), "@cfg requires a condition: @cfg(NAME)");
                return (DeclarationAttributes){0};
            }
            lexer_next(l);
            Token cfg_tok = lexer_next(l);
            int is_any = cfg_tok.kind == TOK_IDENT && cfg_tok.len == 3 &&
                         strncmp(cfg_tok.start, "any", 3) == 0;
            int is_all = cfg_tok.kind == TOK_IDENT && cfg_tok.len == 3 &&
                         strncmp(cfg_tok.start, "all", 3) == 0;
            if (is_cfg_not(cfg_tok))
            {
                if (lexer_peek(l).kind != TOK_LPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ( after not in @cfg(not(...))");
                    return (DeclarationAttributes){0};
                }
                lexer_next(l);
                Token name_tok = lexer_next(l);
                if (name_tok.kind != TOK_IDENT)
                {
                    zpanic_at(name_tok, "Expected define name in @cfg(not(NAME))");
                    return (DeclarationAttributes){0};
                }
                res.cfg_condition = cfg_join(res.cfg_condition, " && ", cfg_defined(name_tok, 1));
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after name in @cfg(not(NAME))");
                    return (DeclarationAttributes){0};
                }
            }
            else if (is_any || is_all)
            {
                char *list_cond = NULL;
                if (!parse_cfg_list(l, is_any ? "any" : "all", is_any ? " || " : " && ",
                                    &list_cond))
                {
                    return (DeclarationAttributes){0};
                }
                if (list_cond)
                {
                    char *grouped = xmalloc(strlen(list_cond) + 3);
                    sprintf(grouped, "(%s)", list_cond); /* safe */
                    res.cfg_condition = cfg_join(res.cfg_condition, " && ", grouped);
                }
            }
            else if (cfg_tok.kind == TOK_IDENT)
            {
                res.cfg_condition = cfg_join(res.cfg_condition, " && ", cfg_defined(cfg_tok, 0));
            }
            else
            {
                zpanic_at(cfg_tok, "Expected define name in @cfg(NAME)");
                return (DeclarationAttributes){0};
            }
            if (lexer_next(l).kind != TOK_RPAREN)
            {
                zpanic_at(lexer_peek(l), "Expected ) after @cfg(...)");
                return (DeclarationAttributes){0};
            }
        }
        else if (0 == strncmp(attr.start, "link_name", 9) && 9 == attr.len)
        {
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Token name_tok = lexer_next(l);
                if (name_tok.kind == TOK_STRING)
                {
                    res.link_name = xmalloc(name_tok.len - 1);
                    strncpy(res.link_name, name_tok.start + 1, name_tok.len - 2);
                    res.link_name[name_tok.len - 2] = 0;
                }
                else
                {
                    zpanic_at(name_tok, "Expected string literal in @link_name");
                    return (DeclarationAttributes){0};
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after @link_name value");
                    return (DeclarationAttributes){0};
                }
            }
            else
            {
                zpanic_at(lexer_peek(l), "@link_name requires a value: @link_name(\"name\")");
                return (DeclarationAttributes){0};
            }
        }
        else if (0 == strncmp(attr.start, "link", 4) && 4 == attr.len)
        {
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                Token path_tok = lexer_next(l);
                if (path_tok.kind == TOK_STRING)
                {
                    res.link_path = xmalloc(path_tok.len - 1);
                    strncpy(res.link_path, path_tok.start + 1, path_tok.len - 2);
                    res.link_path[path_tok.len - 2] = 0;
                }
                else
                {
                    zpanic_at(path_tok, "Expected string literal in @link");
                    return (DeclarationAttributes){0};
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected ) after @link path");
                    return (DeclarationAttributes){0};
                }
            }
            else
            {
                zpanic_at(lexer_peek(l), "@link requires a path: @link(\"path/to/file.c\")");
                return (DeclarationAttributes){0};
            }
        }
        else if (0 == strncmp(attr.start, "derive", 6) && 6 == attr.len)
        {
            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                while (1)
                {
                    Token inner_t = lexer_next(l);
                    if (inner_t.kind != TOK_IDENT)
                    {
                        zpanic_at(inner_t, "Expected trait name in @derive");
                        return (DeclarationAttributes){0};
                    }
                    char *trait = token_strdup(inner_t);
                    check_derivable_trait(inner_t, trait);
                    if (res.derived_count < 32)
                    {
                        res.derived_traits[res.derived_count++] = trait;
                    }
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
                    zpanic_at(lexer_peek(l), "Expected ) after derive traits");
                    return (DeclarationAttributes){0};
                }
            }
            else
            {
                zpanic_at(lexer_peek(l), "@derive requires traits: @derive(Debug, Clone)");
                return (DeclarationAttributes){0};
            }
        }
        else
        {
            Attribute *new_attr = xmalloc(sizeof(Attribute));
            new_attr->name = token_strdup(attr);
            new_attr->args = NULL;
            new_attr->count = 0;
            new_attr->next = res.custom_attributes; // Prepend
            res.custom_attributes = new_attr;

            if (lexer_peek(l).kind == TOK_LPAREN)
            {
                lexer_next(l);
                while (1)
                {
                    Token inner_t = lexer_next(l);
                    new_attr->args =
                        realloc(new_attr->args, sizeof(char *) * (size_t)(new_attr->count + 1));

                    if (inner_t.kind == TOK_STRING)
                    {
                        new_attr->args[new_attr->count++] = token_strdup(inner_t);
                    }
                    else
                    {
                        new_attr->args[new_attr->count++] = token_strdup(inner_t);
                    }

                    if (lexer_peek(l).kind == TOK_EOF)
                    {
                        zpanic_at(lexer_peek(l), "Unexpected end of file in attribute args");
                        break;
                    }
                    if (lexer_peek(l).kind == TOK_COMMA)
                    {
                        lexer_next(l);
                    }
                    else if (lexer_peek(l).kind == TOK_RPAREN)
                    {
                        break;
                    }
                    else
                    {
                        zpanic_at(lexer_peek(l), "Expected , or ) in attribute args");
                        return (DeclarationAttributes){0};
                    }
                }
                if (lexer_next(l).kind != TOK_RPAREN)
                {
                    zpanic_at(lexer_peek(l), "Expected )");
                    return (DeclarationAttributes){0};
                }
            }
        }

        t = lexer_peek(l);
    }
    return res;
}
