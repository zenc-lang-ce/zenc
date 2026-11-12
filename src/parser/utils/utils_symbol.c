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

void add_symbol(ParserContext *ctx, const char *n, Type *type_info, int is_export)
{
    add_symbol_with_token(ctx, n, type_info, TOKEN_UNKNOWN, is_export);
}

void add_symbol_with_token(ParserContext *ctx, const char *n, Type *type_info, Token tok,
                           int is_export)
{
    if (!ctx->current_scope)
    {
        if (!ctx->global_scope)
        {
            ctx->global_scope = symbol_scope_create(NULL, "Global");
        }
        ctx->current_scope = ctx->global_scope;
    }

    if (strcmp(n, "it") != 0 && strcmp(n, "self") != 0)
    {
        audit_section_5(ctx, ctx->current_scope, n, NULL, tok);
    }

    if (ctx->config->mode_lsp || ctx->config->misra_mode)
    {
        ZenSymbol *existing = symbol_lookup_local(ctx->current_scope, n);
        if (existing)
        {
            existing->type_info = type_info;
            existing->decl_token = tok;
            return;
        }
    }

    ZenSymbol *s = symbol_add(ctx->current_scope, n, SYM_VARIABLE);
    s->is_local = (ctx->current_scope != ctx->global_scope);
    s->type_info = type_info;
    s->decl_token = tok;
    s->is_export = is_export;

    register_in_all_symbols(ctx, s);
}

Type *find_symbol_type_info(ParserContext *ctx, const char *n)
{
    ZenSymbol *sym = symbol_lookup(ctx->current_scope, n);
    if (sym)
    {
        return sym->type_info;
    }

    EnumVariantReg *enum_variant = find_enum_variant(ctx, n);
    if (enum_variant)
    {
        return type_new_enum(enum_variant->enum_name);
    }

    return NULL;
}

ZenSymbol *find_symbol_entry(ParserContext *ctx, const char *n)
{
    return symbol_lookup(ctx->current_scope, n);
}

ZenSymbol *find_symbol_in_all(ParserContext *ctx, const char *n)
{
    ZenSymbol **last = zmap_get(&ctx->all_symbols_by_name, n);
    return last ? *last : NULL;
}

void init_builtins(void)
{
    static int init = 0;
    if (init)
    {
        return;
    }
    init = 1;
}

void register_func(ParserContext *ctx, Scope *scope, const char *name, int count,
                   ASTNode **default_values, Type **arg_types, Type *ret_type, int is_varargs,
                   int is_pure, const char *link_name, const char *own_name, Token decl_token,
                   int is_export)
{
    FuncSig *f = NULL;
    if (ctx->config->mode_lsp || ctx->config->misra_mode)
    {
        f = find_func(ctx, name);
    }

    if (!f)
    {
        f = xcalloc(1, sizeof(FuncSig));
        f->name = xstrdup(name);
        f->next = ctx->func_registry;
        ctx->func_registry = f;
        // Every ParserContext starts zeroed, wherever it is created: the map starts here.
        if (!ctx->func_registry_by_name.hash_func)
        {
            ctx->func_registry_by_name = zmap_init(FuncByName, zmap_hash_cstr, zmap_cmp_cstr);
        }
        zmap_put(&ctx->func_registry_by_name, f->name, f);
    }

    f->decl_token = decl_token;
    f->total_args = count;
    // One slot per argument, NULL where it has no default: callers that register a function
    // without defaults (enum variant constructors) get the empty array readers index into.
    f->default_values =
        (default_values || count <= 0) ? default_values : xcalloc((size_t)count, sizeof(ASTNode *));
    f->arg_types = arg_types;
    f->ret_type = ret_type;
    f->is_varargs = is_varargs;
    f->is_pure = is_pure;
    f->required = 0;
    f->own_name = own_name;

    Scope *target_scope = scope ? scope : ctx->current_scope;
    audit_section_5(ctx, target_scope ? target_scope : ctx->global_scope, name, link_name,
                    decl_token);

    ZenSymbol *sym = symbol_lookup_local(target_scope, name);
    if (!sym)
    {
        sym = symbol_add(scope ? scope : ctx->current_scope, name, SYM_FUNCTION);
    }
    else
    {
        sym->kind = SYM_FUNCTION;
    }
    sym->data.sig = f;
    sym->decl_token = decl_token;
    sym->is_export = is_export;
    if (link_name)
    {
        f->link_name = xstrdup(link_name);
        sym->link_name = f->link_name;
    }

    register_in_all_symbols(ctx, sym);

    Type *ft = type_new(TYPE_FUNCTION);
    ft->count = count;
    ft->args = arg_types;
    ft->inner = ret_type;
    ft->is_raw = 1;
    ft->traits.has_drop = 0;
    sym->type_info = ft;
}

const char *callee_own_name(ParserContext *ctx, ASTNode *callee)
{
    if (callee->kind != NODE_EXPR_VAR)
    {
        return NULL;
    }
    if (callee->var_ref.variant.member)
    {
        return callee->var_ref.variant.member;
    }
    FuncSig *sig = callee->var_ref.name ? find_func(ctx, callee->var_ref.name) : NULL;
    if (sig && sig->own_name)
    {
        return sig->own_name;
    }
    return callee->var_ref.instance.name;
}

void register_func_template(ParserContext *ctx, const char *name, const char *param, ASTNode *node)
{
    GenericFuncTemplate *t = xcalloc(1, sizeof(GenericFuncTemplate));
    t->name = xstrdup(name);
    t->generic_param = xstrdup(param);
    t->func_node = node;
    t->next = ctx->func_templates;
    ctx->func_templates = t;
}

void register_deprecated_func(ParserContext *ctx, const char *name, const char *reason)
{
    DeprecatedFunc *d = xmalloc(sizeof(DeprecatedFunc));
    d->name = xstrdup(name);
    d->reason = reason ? xstrdup(reason) : NULL;
    d->next = ctx->deprecated_funcs;
    ctx->deprecated_funcs = d;
}

GenericFuncTemplate *find_func_template(ParserContext *ctx, const char *name)
{
    GenericFuncTemplate *t = ctx->func_templates;
    while (t)
    {
        if (strcmp(t->name, name) == 0)
        {
            return t;
        }
        t = t->next;
    }
    return NULL;
}

static GenericParameter *find_known_generic(ParserContext *ctx, const char *name, size_t len)
{
    for (int i = 0; i < ctx->known_generics_count; i++)
    {
        const char *known = ctx->known_generics[i].name;
        if (strncmp(known, name, len) == 0 && known[len] == 0)
        {
            return &ctx->known_generics[i];
        }
    }
    return NULL;
}

static void add_generic_parameter_use(ParserContext *ctx, Token at, Token declaration)
{
    if (ctx->is_speculative)
    {
        return;
    }
    GenericParameterUse *use = xmalloc(sizeof(GenericParameterUse));
    use->at = at;
    use->declaration = declaration;
    use->next = ctx->generic_parameter_uses;
    ctx->generic_parameter_uses = use;
}

void register_generic(ParserContext *ctx, Token name)
{
    if (find_known_generic(ctx, name.start, name.len))
    {
        return;
    }
    GenericParameter *param = &ctx->known_generics[ctx->known_generics_count++];
    param->name = token_strdup(name);
    param->token = name;
    add_generic_parameter_use(ctx, name, name);
}

void record_generic_parameter_use(ParserContext *ctx, Token t)
{
    GenericParameter *param = find_known_generic(ctx, t.start, t.len);
    if (param)
    {
        add_generic_parameter_use(ctx, t, param->token);
    }
}

int is_known_generic(ParserContext *ctx, char *name)
{
    return find_known_generic(ctx, name, strlen(name)) != NULL;
}

int type_is_generic_dependent(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return 0;
    }
    if (t->kind == TYPE_GENERIC || (t->name && t->count == 0 && is_known_generic(ctx, t->name)))
    {
        return 1;
    }
    if (type_is_generic_dependent(ctx, t->inner))
    {
        return 1;
    }
    for (int i = 0; t->args && i < t->count; i++)
    {
        if (type_is_generic_dependent(ctx, t->args[i]))
        {
            return 1;
        }
    }
    return 0;
}

void register_impl_template(ParserContext *ctx, const char *sname, const char *param, ASTNode *node)
{
    GenericImplTemplate *t = xmalloc(sizeof(GenericImplTemplate));
    t->struct_name = xstrdup(sname);
    t->generic_param = xstrdup(param);
    t->impl_node = node;
    t->next = ctx->impl_templates;
    ctx->impl_templates = t;

    Instantiation *inst = ctx->instantiations;
    while (inst)
    {
        if (inst->template_name && strcmp(inst->template_name, sname) == 0)
        {
            instantiate_methods(ctx, t, inst->name, inst->args, inst->count);
        }
        inst = inst->next;
    }
}

void add_instantiated_func(ParserContext *ctx, ASTNode *fn)
{
    fn->next = ctx->instantiated_funcs;
    ctx->instantiated_funcs = fn;
}

void register_enum_variant(ParserContext *ctx, const char *vname, const char *ename, int tag)
{
    if (ctx->config->mode_lsp)
    {
        EnumVariantReg *existing = find_enum_variant_of(ctx, ename, vname);
        if (existing)
        {
            existing->tag_id = tag;
            return;
        }
    }

    audit_section_5(ctx, ctx->global_scope, vname, NULL, TOKEN_UNKNOWN);

    EnumVariantReg *r = xcalloc(1, sizeof(EnumVariantReg));
    r->enum_name = ename ? xstrdup(ename) : NULL;
    r->variant_name = vname ? xstrdup(vname) : NULL;
    r->tag_id = tag;
    r->next = ctx->enum_variants;
    ctx->enum_variants = r;
}

static int variant_reg_is_of(EnumVariantReg *r, const char *enum_name)
{
    return r->enum_name && strcmp(r->enum_name, enum_name) == 0;
}

// A variant written without its enum (`Red`): the first variant with that name. A qualified
// one (`Color::Red`) is resolved where it is parsed, from the node's `var_ref.variant`.
EnumVariantReg *find_enum_variant(ParserContext *ctx, const char *name)
{
    for (EnumVariantReg *r = ctx->enum_variants; r; r = r->next)
    {
        if (r->variant_name && strcmp(r->variant_name, name) == 0)
        {
            return r;
        }
    }
    return NULL;
}

char *definition_link_name(ParserContext *ctx, const char *link_name, const char *name)
{
    if (link_name)
    {
        return xstrdup(link_name);
    }
    const char *safe = c_safe_name(name, ctx->config->use_cpp);
    return safe != name ? (char *)safe : NULL;
}

const char *link_name_of(void *ctx, const char *name)
{
    ASTNode *def = find_struct_def((ParserContext *)ctx, name);
    return def ? def->link_name : NULL;
}

// Several enums may share a variant name (`Slot::None`, `Attribute::None`), so
// a lookup that knows the enum must filter by it instead of taking the first
// variant with that name.
EnumVariantReg *find_enum_variant_of(ParserContext *ctx, const char *enum_name,
                                     const char *variant_name)
{
    if (!enum_name || !variant_name)
    {
        return NULL;
    }
    EnumVariantReg *r = ctx->enum_variants;
    while (r)
    {
        if (r->variant_name && strcmp(r->variant_name, variant_name) == 0 &&
            variant_reg_is_of(r, enum_name))
        {
            return r;
        }
        r = r->next;
    }
    return NULL;
}

// Whether a case without a guard names `variant` in one of its alternatives.
static int match_covers(ASTNode *match, const char *variant)
{
    for (ASTNode *c = match->match_stmt.cases; c; c = c->next)
    {
        if (c->match_case.guard)
        {
            continue;
        }
        for (MatchPattern *p = c->match_case.patterns; p; p = p->next)
        {
            if (p->kind == PATTERN_NAME && strcmp(p->name.name, variant) == 0)
            {
                return 1;
            }
        }
    }
    return 0;
}

Type *match_subject_type(Type *scrutinee)
{
    Type *t = get_inner_type(scrutinee);
    while (t && t->kind == TYPE_POINTER)
    {
        t = get_inner_type(t->inner);
    }
    return t;
}

const char *match_missing_variant(ParserContext *ctx, ASTNode *match, Type *scrutinee)
{
    Type *subject = match_subject_type(scrutinee);
    const char *subject_type = subject ? type_def_name(subject) : NULL;
    // A default case without a guard covers everything.
    for (ASTNode *c = match->match_stmt.cases; c; c = c->next)
    {
        for (MatchPattern *p = c->match_case.patterns; p && !c->match_case.guard; p = p->next)
        {
            if (p->kind == PATTERN_WILDCARD)
            {
                return NULL;
            }
        }
    }

    // Option and Result are structs in the standard library, with two variants each.
    const char *pair[2] = {NULL, NULL};
    if (type_is_instance_of(subject, "Option"))
    {
        pair[0] = "Some";
        pair[1] = "None";
    }
    else if (type_is_instance_of(subject, "Result"))
    {
        pair[0] = "Ok";
        pair[1] = "Err";
    }
    if (pair[0])
    {
        for (int i = 0; i < 2; i++)
        {
            if (!match_covers(match, pair[i]))
            {
                return pair[i];
            }
        }
        return NULL;
    }

    // An enum: the subject's type if it is one, else the enum of the first variant named.
    const char *enum_name = NULL;
    for (EnumVariantReg *r = ctx->enum_variants; r && subject_type && !enum_name; r = r->next)
    {
        if (strcmp(r->enum_name, subject_type) == 0)
        {
            enum_name = r->enum_name;
        }
    }
    for (ASTNode *c = match->match_stmt.cases; c && !enum_name; c = c->next)
    {
        for (MatchPattern *p = c->match_case.patterns; p && !enum_name; p = p->next)
        {
            EnumVariantReg *reg = NULL;
            if (p->kind == PATTERN_NAME)
            {
                reg = p->name.scope ? find_enum_variant_of(ctx, p->name.scope, p->name.name)
                                    : find_enum_variant(ctx, p->name.name);
            }
            enum_name = reg ? reg->enum_name : NULL;
        }
    }
    if (!enum_name)
    {
        return "_";
    }

    for (EnumVariantReg *r = ctx->enum_variants; r; r = r->next)
    {
        if (strcmp(r->enum_name, enum_name) == 0 && !match_covers(match, r->variant_name))
        {
            return r->variant_name;
        }
    }
    return NULL;
}

// The expression has no address of its own (a call result, a literal, an enum
// variant...). Taking its address for a `self` or `T*` parameter must go
// through a temporary (`&_rval`), or the C compiler rejects `&(rvalue)`.
int expr_is_rvalue(ParserContext *ctx, ASTNode *e)
{
    if (!e)
    {
        return 0;
    }
    switch (e->kind)
    {
    case NODE_EXPR_CALL:
    case NODE_EXPR_BINARY:
    case NODE_EXPR_STRUCT_INIT:
    case NODE_EXPR_CAST:
    case NODE_EXPR_LITERAL:
    case NODE_MATCH:
        return 1;
    case NODE_EXPR_MEMBER:
        // A field of a temporary is a temporary too (`pairs.get(0).id`), unless
        // it is reached through a pointer (`ptr->field` has an address).
        return !e->member.is_pointer_access && expr_is_rvalue(ctx, e->member.target);
    case NODE_EXPR_VAR:
        // `Shape::Circle` written without (): the variant constructor's result.
        return e->var_ref.variant.member &&
               find_enum_variant_of(ctx, type_def_name(e->var_ref.variant.owner),
                                    e->var_ref.variant.member);
    default:
        return 0;
    }
}

void register_lambda(ParserContext *ctx, ASTNode *node)
{
    LambdaRef *ref = xmalloc(sizeof(LambdaRef));
    ref->node = node;
    ref->next = NULL;

    // Keep the creation order, which is the order they are emitted in: a lambda defined inside
    // another one is created first, and the outer body uses its context and its function.
    LambdaRef **tail = &ctx->global_lambdas;
    while (*tail)
    {
        tail = &(*tail)->next;
    }
    *tail = ref;
}

void register_extern_symbol(ParserContext *ctx, const char *name)
{
    for (int i = 0; i < ctx->extern_symbol_count; i++)
    {
        if (strcmp(ctx->extern_symbols[i], name) == 0)
        {
            return;
        }
    }

    if (ctx->extern_symbol_count == 0)
    {
        ctx->extern_symbols = xmalloc(sizeof(char *) * 64);
    }
    else if (ctx->extern_symbol_count % 64 == 0)
    {
        ctx->extern_symbols =
            xrealloc(ctx->extern_symbols, sizeof(char *) * (size_t)(ctx->extern_symbol_count + 64));
    }

    ctx->extern_symbols[ctx->extern_symbol_count++] = xstrdup(name);
}

int is_extern_symbol(ParserContext *ctx, const char *name)
{
    for (int i = 0; i < ctx->extern_symbol_count; i++)
    {
        if (strcmp(ctx->extern_symbols[i], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// The functions (and function-like macros) that the headers of the hosted preamble declare:
// <stdio.h>, <stdlib.h>, <string.h>, <stdarg.h>, <unistd.h> and <fcntl.h> (emit_preamble).
static const char *const PREAMBLE_C_FUNCTIONS[] = {
    // <stdio.h>
    "clearerr", "dprintf", "fclose", "fdopen", "feof", "ferror", "fflush", "fgetc", "fgetpos",
    "fgets", "fileno", "fopen", "fprintf", "fputc", "fputs", "fread", "freopen", "fscanf", "fseek",
    "fsetpos", "ftell", "fwrite", "getc", "getchar", "getdelim", "getline", "pclose", "perror",
    "popen", "printf", "putc", "putchar", "puts", "remove", "rename", "rewind", "scanf", "setbuf",
    "setvbuf", "snprintf", "sprintf", "sscanf", "tmpfile", "tmpnam", "ungetc", "vfprintf",
    "vfscanf", "vprintf", "vscanf", "vsnprintf", "vsprintf", "vsscanf",
    // <stdlib.h>
    "_Exit", "abort", "abs", "aligned_alloc", "atexit", "atof", "atoi", "atol", "atoll", "bsearch",
    "calloc", "div", "exit", "free", "getenv", "labs", "ldiv", "llabs", "lldiv", "malloc",
    "mkstemp", "posix_memalign", "qsort", "rand", "realloc", "realpath", "setenv", "srand",
    "strtod", "strtof", "strtol", "strtold", "strtoll", "strtoul", "strtoull", "system", "unsetenv",
    // <string.h>
    "memchr", "memcmp", "memcpy", "memmove", "memset", "strcat", "strchr", "strcmp", "strcoll",
    "strcpy", "strcspn", "strdup", "strerror", "strlen", "strncat", "strncmp", "strncpy", "strndup",
    "strnlen", "strpbrk", "strrchr", "strspn", "strstr", "strtok", "strtok_r", "strxfrm",
    // <stdarg.h>
    "va_arg", "va_copy", "va_end", "va_start",
    // <unistd.h> and <fcntl.h>
    "access", "chdir", "close", "creat", "dup", "dup2", "execv", "execve", "execvp", "fcntl",
    "fork", "getcwd", "getgid", "getpid", "getppid", "getuid", "isatty", "lseek", "open", "pipe",
    "read", "rmdir", "sleep", "sysconf", "unlink", "usleep", "write", NULL};

int is_preamble_c_function(ParserContext *ctx, const char *name)
{
    // The MISRA preamble includes none of these headers.
    if (ctx->config->misra_mode)
    {
        return 0;
    }
    for (int i = 0; PREAMBLE_C_FUNCTIONS[i]; i++)
    {
        if (strcmp(PREAMBLE_C_FUNCTIONS[i], name) == 0)
        {
            // The freestanding preamble only includes <stdarg.h>.
            return !ctx->config->is_freestanding || strncmp(name, "va_", 3) == 0;
        }
    }
    return 0;
}

// The variables the headers of the hosted preamble declare, which a program uses undeclared.
static const char *const PREAMBLE_C_VARIABLES[] = {"stdin", "stdout", "stderr", NULL};

static int is_preamble_c_variable(ParserContext *ctx, const char *name)
{
    if (ctx->config->misra_mode || ctx->config->is_freestanding)
    {
        return 0;
    }
    for (int i = 0; PREAMBLE_C_VARIABLES[i]; i++)
    {
        if (strcmp(PREAMBLE_C_VARIABLES[i], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int should_suppress_undef_warning(ParserContext *ctx, const char *name)
{
    if (strcmp(name, "struct") == 0 || strcmp(name, "tv") == 0)
    {
        return 1;
    }

    // The runtime's helpers, and panic(), which codegen turns into __zenc_panic.
    if (strncmp(name, "_z_", 3) == 0 || strncmp(name, "_Z", 2) == 0 || strcmp(name, "panic") == 0)
    {
        return 1;
    }

    if (is_extern_symbol(ctx, name) || is_preamble_c_function(ctx, name) ||
        is_preamble_c_variable(ctx, name))
    {
        return 1;
    }

    // Functions the project declares as C in its zenc.json.
    for (char **w = ctx->config->c_function_whitelist; w && *w; w++)
    {
        if (strcmp(*w, name) == 0)
        {
            return 1;
        }
    }

    int is_all_caps = 1;
    for (const char *p = name; *p; p++)
    {
        if (islower((unsigned char)*p))
        {
            is_all_caps = 0;
            break;
        }
    }
    if (is_all_caps && name[0] != '\0')
    {
        return 1;
    }

    // C headers bring functions the compiler does not see.
    if (ctx->has_external_includes)
    {
        return 1;
    }
    zmap_iter_ModMap it = zmap_iter_init(ModMap, &ctx->imports.modules);
    const char *alias;
    Module *mod;
    while (zmap_iter_next(&it, &alias, &mod))
    {
        if (mod->is_c_header)
        {
            return 1;
        }
    }

    return 0;
}

// Picks the instance for `Template::method` from the type of the `let` being
// initialized: in `let w: Wrap<int> = Wrap::make(3);` the call is to
// `Wrap__int32_t__make`. Returns that function's name, or NULL when the `let`
// is not declared as an instance of the template or that instance has no such
// method.
char *expected_instance_method(ParserContext *ctx, const char *template_name,
                               const char *method_name)
{
    Type *expected = ctx->cg.expected_init_type;
    if (!type_is_instance_of(expected, template_name))
    {
        return NULL;
    }
    char *name = mangle_method_symbol(type_mangle_spelling(expected), NULL, method_name);
    return find_func(ctx, name) ? name : NULL;
}

int find_method_call_target(ParserContext *ctx, const char *type_name, const char *method,
                            const char **owner_out, char **method_out)
{
    char *direct = mangle_method_symbol(type_name, NULL, method);
    int found = find_func(ctx, direct) != NULL;
    zfree(direct);
    if (found)
    {
        *owner_out = type_name;
        *method_out = xstrdup(method);
        return 1;
    }

    // A method of a trait the type implements: `Type__Trait__method`.
    for (StructRef *ref = ctx->parsed_impls_list; ref; ref = ref->next)
    {
        if (ref->node && ref->node->kind == NODE_IMPL_TRAIT &&
            strcmp(type_impl_name(ref->node->impl_trait.target), type_name) == 0)
        {
            const char *trait = ref->node->impl_trait.trait_name;
            char *trait_mangled = mangle_method_symbol(type_name, trait, method);
            found = find_func(ctx, trait_mangled) != NULL;
            zfree(trait_mangled);
            if (found)
            {
                *owner_out = type_name;
                *method_out = mangle_method_symbol(trait, NULL, method);
                return 1;
            }
        }
    }
    for (GenericImplTemplate *it = ctx->impl_templates; it; it = it->next)
    {
        if (it->impl_node && it->impl_node->kind == NODE_IMPL_TRAIT)
        {
            const char *trait = it->impl_node->impl_trait.trait_name;
            char *trait_mangled = mangle_method_symbol(type_name, trait, method);
            found = find_func(ctx, trait_mangled) != NULL;
            zfree(trait_mangled);
            if (found)
            {
                *owner_out = type_name;
                *method_out = mangle_method_symbol(trait, NULL, method);
                return 1;
            }
        }
    }

    // A method of a struct the type mixes in with `use`.
    ASTNode *def = find_struct_def(ctx, type_name);
    if (def && def->kind == NODE_STRUCT && def->strct.used_structs)
    {
        for (int k = 0; k < def->strct.used_struct_count; k++)
        {
            char *mixin = mangle_method_symbol(def->strct.used_structs[k], NULL, method);
            found = find_func(ctx, mixin) != NULL;
            zfree(mixin);
            if (found)
            {
                *owner_out = def->strct.used_structs[k];
                *method_out = xstrdup(method);
                return 1;
            }
        }
    }
    return 0;
}

Token template_method_token(ParserContext *ctx, const char *template_name, const char *method_name)
{
    for (GenericImplTemplate *it = ctx->impl_templates; it; it = it->next)
    {
        if (!it->struct_name || strcmp(it->struct_name, template_name) != 0)
        {
            continue;
        }
        ASTNode *impl = it->impl_node;
        ASTNode *methods = impl->kind == NODE_IMPL ? impl->impl.methods : impl->impl_trait.methods;
        for (ASTNode *m = methods; m; m = m->next)
        {
            if (m->kind == NODE_FUNCTION && m->func.method_name &&
                strcmp(m->func.method_name, method_name) == 0)
            {
                return m->token;
            }
        }
    }
    return TOKEN_UNKNOWN;
}
