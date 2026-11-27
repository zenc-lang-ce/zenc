// SPDX-License-Identifier: MIT
#include "plugins/plugin_manager.h"
#include "parser.h"
#include "utils/format_expr.h"
#include "utils/colors.h"
#include "utils/utils.h"
#include "constants.h"
#include "ast/primitives.h"
#include <ctype.h>
#include <limits.h>
#include "analysis/const_fold.h"

static char *local_key(const char *site, const char *name)
{
    size_t size = strlen(name) + 32;
    char *key = xmalloc(size);
    snprintf(key, size, "%p\x1f%s", (const void *)site, name);
    return key;
}

void record_local_c_name(ParserContext *ctx, const char *site, const char *name, const char *c_name)
{
    // Every ParserContext starts zeroed, wherever it is created: the map starts here.
    if (!ctx->local_c_names.hash_func)
    {
        ctx->local_c_names = zmap_init(CNameByDecl, zmap_hash_cstr, zmap_cmp_cstr);
    }
    zmap_put(&ctx->local_c_names, local_key(site, name), c_name);
}

const char *local_c_name_at(ParserContext *ctx, const char *site, const char *name)
{
    if (!ctx->local_c_names.hash_func || !site || !name)
    {
        return NULL;
    }
    char *key = local_key(site, name);
    const char **found = zmap_get(&ctx->local_c_names, key);
    zfree(key);
    return found ? *found : NULL;
}

void register_named_type(ParserContext *ctx, const char *name, ASTNode *node)
{
    // Every ParserContext starts zeroed, wherever it is created: the map starts here.
    if (!ctx->type_registry.hash_func)
    {
        ctx->type_registry = zmap_init(DeclByName, zmap_hash_cstr, zmap_cmp_cstr);
    }
    zmap_put(&ctx->type_registry, xstrdup(name), node);
}

void add_to_struct_list(ParserContext *ctx, ASTNode *node)
{
    StructRef *r = xmalloc(sizeof(StructRef));
    r->node = node;
    r->next = ctx->parsed_structs_list;
    ctx->parsed_structs_list = r;
    register_named_type(ctx, declared_type_c_name(node), node);
}

void register_type_alias(ParserContext *ctx, const char *alias, Type *type_info, int is_opaque,
                         const char *defined_in_file, Token tok, int is_export)
{
    if (ctx->config->mode_lsp)
    {
        TypeAlias *existing = find_type_alias_node(ctx, alias);
        if (existing)
        {
            existing->type_info = type_info;
            existing->is_opaque = is_opaque;
            existing->defined_in_file = defined_in_file ? xstrdup(defined_in_file) : NULL;
        }
    }

    TypeAlias *ta = NULL;
    if (ctx->config->mode_lsp)
    {
        ta = find_type_alias_node(ctx, alias);
    }

    if (!ta)
    {
        ta = xmalloc(sizeof(TypeAlias));
        ta->alias = xstrdup(alias);
        ta->next = ctx->type_aliases;
        ctx->type_aliases = ta;
    }

    ta->type_info = type_info;
    ta->is_opaque = is_opaque;
    ta->defined_in_file = defined_in_file ? xstrdup(defined_in_file) : NULL;

    audit_section_5(ctx, ctx->current_scope, alias, NULL, tok);
    ZenSymbol *sym = symbol_lookup_local(ctx->current_scope, alias);
    if (!sym)
    {
        sym = symbol_add(ctx->current_scope, alias, SYM_ALIAS);
    }
    else
    {
        sym->kind = SYM_ALIAS;
    }
    sym->decl_token = tok;
    sym->is_export = is_export;
    sym->type_info = type_info;
    register_in_all_symbols(ctx, sym);
}

Type *find_type_alias(ParserContext *ctx, const char *alias)
{
    ZenSymbol *sym = symbol_lookup_kind(ctx->current_scope, alias, SYM_ALIAS);
    if (sym)
    {
        return sym->type_info;
    }

    TypeAlias *ta = find_type_alias_node(ctx, alias);
    return ta ? ta->type_info : NULL;
}

TypeAlias *find_type_alias_node(ParserContext *ctx, const char *alias)
{
    TypeAlias *ta = ctx->type_aliases;
    while (ta)
    {
        if (ta->alias[0] == alias[0] && strcmp(ta->alias, alias) == 0)
        {
            return ta;
        }
        ta = ta->next;
    }
    return NULL;
}

void add_to_enum_list(ParserContext *ctx, ASTNode *node)
{
    StructRef *r = xmalloc(sizeof(StructRef));
    r->node = node;
    r->next = ctx->parsed_enums_list;
    ctx->parsed_enums_list = r;
    if (declared_type_c_name(node))
    {
        register_named_type(ctx, declared_type_c_name(node), node);
    }
}

void add_to_func_list(ParserContext *ctx, ASTNode *node)
{
    StructRef *curr = ctx->parsed_funcs_list;
    while (curr)
    {
        if (curr->node == node)
        {
            return;
        }
        curr = curr->next;
    }
    StructRef *r = xmalloc(sizeof(StructRef));
    r->node = node;
    r->next = ctx->parsed_funcs_list;
    ctx->parsed_funcs_list = r;
}

void add_to_impl_list(ParserContext *ctx, ASTNode *node)
{
    StructRef *curr = ctx->parsed_impls_list;
    while (curr)
    {
        if (curr->node == node)
        {
            return;
        }
        curr = curr->next;
    }
    StructRef *r = xmalloc(sizeof(StructRef));
    r->node = node;
    r->next = ctx->parsed_impls_list;
    ctx->parsed_impls_list = r;
}

void add_to_global_list(ParserContext *ctx, ASTNode *node)
{
    StructRef *curr = ctx->parsed_globals_list;
    while (curr)
    {
        if (curr->node == node)
        {
            return;
        }
        curr = curr->next;
    }
    StructRef *r = xmalloc(sizeof(StructRef));
    r->node = node;
    r->next = ctx->parsed_globals_list;
    ctx->parsed_globals_list = r;
}

Type *instantiate_slice(ParserContext *ctx, Type *elem, Token token)
{
    Type **args = xmalloc(sizeof(Type *));
    args[0] = elem;
    Type *slice = type_new_instance("Slice", args, 1);
    instantiate_type(ctx, slice, token);
    return slice;
}

#if FF_COMPATIBILITY_0_5
Type *compat_slice_from_name(ParserContext *ctx, const char *name, Token token)
{
    if (strncmp(name, "Slice_", 6) != 0 || name[6] == '_' || name[6] == '\0')
    {
        return NULL;
    }
    const char *elem_name = name + 6;
    TypeKind kind = find_primitive_kind(elem_name);
    Type *elem = NULL;
    if (kind != TYPE_UNKNOWN)
    {
        elem = type_new(kind);
    }
    else if (find_type_alias_node(ctx, elem_name) || find_struct_def(ctx, elem_name))
    {
        elem = type_from_name(ctx, elem_name);
    }
    return elem ? instantiate_slice(ctx, elem, token) : NULL;
}
#endif

void register_tuple(ParserContext *ctx, Type *tuple)
{
    char *struct_name = type_mangle_spelling(tuple);
    for (TupleType *c = ctx->used_tuples; c; c = c->next)
    {
        if (strcmp(c->name, struct_name) == 0)
        {
            zfree(struct_name);
            return;
        }
    }
    TupleType *n = xmalloc(sizeof(TupleType));
    n->type = tuple;
    n->name = struct_name;
    n->next = ctx->used_tuples;
    ctx->used_tuples = n;
}

void register_struct_def(ParserContext *ctx, const char *name, ASTNode *node)
{
    // The language server parses a file again: its struct takes the place of the old one.
    register_named_type(ctx, name, node);

    if (ctx->config->misra_mode)
    {
        ZenSymbol *all = ctx->all_symbols;
        while (all)
        {
            if ((all->kind == SYM_STRUCT || all->kind == SYM_ENUM) && all->name[0] == name[0] &&
                strcmp(all->name, name) == 0)
            {
                zerror_at(node ? node->token : TOKEN_UNKNOWN, "MISRA Rule 5.7");
                break;
            }
            all = all->next;
        }
    }

    ensure_global_scope(ctx);
    ZenSymbol *sym_existing = symbol_lookup_local(ctx->global_scope, name);
    ZenSymbol *sym = NULL;
    if (!sym_existing)
    {
        sym = symbol_add(ctx->global_scope, name,
                         (node && node->kind == NODE_ENUM) ? SYM_ENUM : SYM_STRUCT);
    }
    else
    {
        sym = sym_existing;
        sym->kind = (node && node->kind == NODE_ENUM) ? SYM_ENUM : SYM_STRUCT;
    }

    sym->data.node = node;
    sym->link_name = node ? node->link_name : NULL;
    if (node)
    {
        sym->decl_token = node->token;
        if (node->kind == NODE_STRUCT)
        {
            sym->is_export = node->strct.is_export;
        }
        else if (node->kind == NODE_ENUM)
        {
            sym->is_export = node->enm.is_export;
        }
    }
    sym->type_info = node ? node->type_info : NULL;

    audit_section_5(ctx, ctx->global_scope, name, sym->link_name, sym->decl_token);

    register_in_all_symbols(ctx, sym);
}

FuncSig *index_method(ParserContext *ctx, Type *t, const char *method)
{
    t = resolve_alias(t);
    if (!t || t->kind != TYPE_STRUCT || !t->name || slice_element(t))
    {
        return NULL;
    }
    const char *name = type_def_name(t);
    char *mangled = mangle_method_symbol(name, NULL, method);
    FuncSig *sig = find_func(ctx, mangled);
    if (sig || !strchr(name, '<'))
    {
        return sig;
    }

    // While the body of a generic template is parsed (`Vec<T>`), its own method, with the
    // template's types.
    size_t len = strcspn(name, "<");
    char *base = xmalloc(len + 1);
    memcpy(base, name, len);
    base[len] = 0;
    char *template_name = mangle_method_symbol(base, NULL, method);
    for (GenericImplTemplate *it = ctx->impl_templates; it; it = it->next)
    {
        if (strcmp(it->struct_name, base) != 0)
        {
            continue;
        }
        for (ASTNode *m = it->impl_node->impl.methods; m; m = m->next)
        {
            if (m->func.name && strcmp(function_c_name(m), template_name) == 0)
            {
                sig = xcalloc(1, sizeof(FuncSig));
                sig->ret_type = m->func.ret_type_info;
                sig->arg_types = m->func.arg_types;
                sig->total_args = m->func.count;
                return sig;
            }
        }
    }
    return NULL;
}

Type *vector_type_named(ParserContext *ctx, const char *name)
{
    ASTNode *def = name ? find_struct_def(ctx, name) : NULL;
    if (!def || def->kind != NODE_STRUCT || !def->type_info || def->type_info->kind != TYPE_VECTOR)
    {
        return NULL;
    }
    return def->type_info;
}

Type *vector_type_of(ParserContext *ctx, Type *t)
{
    t = resolve_alias(t);
    if (!t || !t->name || (t->kind != TYPE_STRUCT && t->kind != TYPE_VECTOR))
    {
        return NULL;
    }
    return vector_type_named(ctx, type_def_name(t));
}

ASTNode *named_type_def(ParserContext *ctx, Type *t)
{
    return (t && (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->name)
               ? find_struct_def(ctx, type_def_name(t))
               : NULL;
}

ASTNode *find_struct_def(ParserContext *ctx, const char *name)
{
    ASTNode **registered =
        ctx && ctx->type_registry.hash_func ? zmap_get(&ctx->type_registry, name) : NULL;
    return registered ? *registered : NULL;
}

int is_simple_enum(ParserContext *ctx, const char *enum_name)
{
    if (!ctx || !enum_name)
    {
        return 0;
    }
    const char *clean = enum_name;
    if (strncmp(clean, "struct ", 7) == 0)
    {
        clean += 7;
    }
    ASTNode *def = find_struct_def(ctx, clean);
    if (!def || def->kind != NODE_ENUM)
    {
        return 0;
    }
    ASTNode *v = def->enm.variants;
    while (v)
    {
        if (v->variant.payload)
        {
            return 0;
        }
        v = v->next;
    }
    return 1;
}

ASTNode *find_trait_def(ParserContext *ctx, const char *name)
{
    if (!ctx || !name)
    {
        return NULL;
    }

    StructRef *r = ctx->parsed_globals_list;
    while (r)
    {
        if (r->node && r->node->kind == NODE_TRAIT && r->node->trait.name[0] == name[0] &&
            strcmp(r->node->trait.name, name) == 0)
        {
            return r->node;
        }
        r = r->next;
    }
    return NULL;
}

int is_trait(ParserContext *ctx, const char *name)
{
    if (!name)
    {
        return 0;
    }
    if (strncmp(name, "struct ", 7) == 0)
    {
        name += 7;
    }
    else if (strncmp(name, "union ", 6) == 0)
    {
        name += 6;
    }
    char base[MAX_MANGLED_NAME_LEN];
    size_t length = strcspn(name, "*");
    if (length >= sizeof(base))
    {
        return 0; // Longer than any name the program declares
    }
    memcpy(base, name, length);
    base[length] = 0;
    return find_trait_def(ctx, base) != NULL;
}

ASTNode *find_concrete_struct_def(ParserContext *ctx, const char *name)
{
    ASTNode *def = find_struct_def(ctx, name);
    if (!def || (def->kind == NODE_STRUCT && def->strct.is_template) ||
        (def->kind == NODE_ENUM && def->enm.is_template))
    {
        return NULL;
    }
    return def;
}

// The key of `name` in `file`'s part of a registry: the two, joined by the unit separator, which
// neither a path nor a name has.
static char *file_key(const char *file, const char *name)
{
    size_t size = strlen(file) + strlen(name) + 2;
    char *key = xmalloc(size);
    snprintf(key, size, "%s\x1f%s", file, name);
    return key;
}

Module *imported_module(ParserContext *ctx, const char *scope)
{
    // The module this file imports by that alias.
    Module **found = NULL;
    if (ctx->current_filename)
    {
        char *key = file_key(ctx->current_filename, scope);
        found = zmap_get(&ctx->imports.file_modules, key);
        zfree(key);
    }
#if FF_COMPATIBILITY_0_5
    // In 0.5 an alias imported by any file is seen by every file.
    if (!found)
    {
        found = zmap_get(&ctx->imports.modules, scope);
    }
#endif
    return found ? *found : NULL;
}

// The C name of the top-level declaration `decl`: its name, which carries the module prefix.
static const char *declaration_c_name(ASTNode *decl)
{
    switch (decl->kind)
    {
    case NODE_FUNCTION:
        return function_c_name(decl);
    case NODE_STRUCT:
        return declared_type_c_name(decl);
    case NODE_ENUM:
        return declared_type_c_name(decl);
    case NODE_TRAIT:
        return decl->trait.name;
    case NODE_TYPE_ALIAS:
        return decl->type_alias.alias;
    case NODE_CONST:
    case NODE_VAR_DECL:
        return decl->var_decl.name;
    default:
        return NULL;
    }
}

// The import, one by one or with `as *`, that brings `name` into `file`; NULL when none. In 0.5
// (FF_COMPATIBILITY_0_5) the program's table, where `as *` registers each name of the module.
// From 0.6 the file's own imports, then the modules it imports with `as *`.
static SelectiveImport *imported_by_name(ParserContext *ctx, const char *file, const char *name)
{
#if FF_COMPATIBILITY_0_5
    (void)file;
    return find_selective_import(ctx, name);
#else
    if (!file)
    {
        return NULL;
    }
    char *key = file_key(file, name);
    SelectiveImport **found = zmap_get(&ctx->imports.file_selective_imports, key);
    zfree(key);
    if (found)
    {
        return *found;
    }
    size_t file_len = strlen(file);
    zmap_iter_FileSet it = zmap_iter_init(FileSet, &ctx->imports.file_wildcard_imports);
    const char *wildcard_key;
    const char *path;
    while (zmap_iter_next(&it, &wildcard_key, &path))
    {
        if (strncmp(wildcard_key, file, file_len) == 0 && wildcard_key[file_len] == '\x1f' &&
            find_file_declaration(ctx, path, name))
        {
            SelectiveImport *imported = xcalloc(1, sizeof(SelectiveImport));
            imported->symbol = xstrdup(name);
            imported->source_module = xstrdup(wildcard_key + file_len + 1);
            imported->source_path = xstrdup(path);
            return imported;
        }
    }
    return NULL;
#endif
}

char *module_prefixed_name(ParserContext *ctx, const char *prefix, const char *name)
{
    if (!prefix || is_extern_symbol(ctx, name))
    {
        return NULL;
    }
    return mangle_module_symbol(prefix, name);
}

// The C name `name` has in the module `prefix`: the module's prefix and the name, or the name
// alone when it keeps it (module_prefixed_name).
static char *module_c_name(ParserContext *ctx, const char *prefix, const char *name)
{
    char *prefixed = module_prefixed_name(ctx, prefix, name);
    return prefixed ? prefixed : xstrdup(name);
}

// The C name of what `imported` names: its module's declaration of it, or, for one the module
// does not declare, the name the module would give it.
static char *imported_c_name(ParserContext *ctx, SelectiveImport *imported)
{
    ASTNode *decl = imported->source_path
                        ? find_file_declaration(ctx, imported->source_path, imported->symbol)
                        : NULL;
    const char *c_name = decl ? declaration_c_name(decl) : NULL;
    return c_name ? xstrdup(c_name) : module_c_name(ctx, imported->source_module, imported->symbol);
}

char *module_written_name(ParserContext *ctx, const char *name)
{
    const char *alias = ctx->imports.current_module_alias;
    if (!alias)
    {
        // Imported one by one: by the name the import gives it (`GeoVec` of
        // `import { Vector as GeoVec }`), registered before the module is parsed.
        zmap_iter_SelMap it = zmap_iter_init(SelMap, &ctx->imports.selective_imports);
        const char *local_name;
        SelectiveImport *imported;
        while (zmap_iter_next(&it, &local_name, &imported))
        {
            if (ctx->imports.current_module_prefix && imported->source_module &&
                strcmp(imported->source_module, ctx->imports.current_module_prefix) == 0 &&
                strcmp(imported->symbol, name) == 0)
            {
                return xstrdup(local_name);
            }
        }
        return xstrdup(name);
    }
    size_t size = strlen(alias) + strlen(name) + 3;
    char *written = xmalloc(size);
    snprintf(written, size, "%s::%s", alias, name);
    return written;
}

// The C name of the module being parsed's own declaration of `name` (registered by the name it
// writes, find_file_declaration), when `is_kind` accepts it; NULL outside a module parsed with a
// prefix, or when the module has none above.
static char *module_own_name(ParserContext *ctx, const char *name, int (*is_kind)(ASTNode *))
{
    if (!ctx->imports.current_module_prefix)
    {
        return NULL;
    }
    ASTNode *decl = find_file_declaration(ctx, ctx->current_filename, name);
    const char *c_name = decl && is_kind(decl) ? declaration_c_name(decl) : NULL;
    return c_name ? xstrdup(c_name) : NULL;
}

static int is_type_declaration(ASTNode *decl)
{
    return decl->kind == NODE_STRUCT || decl->kind == NODE_ENUM;
}

// A function the program calls by its name: not a generic one, which is a template.
static int is_function_declaration(ASTNode *decl)
{
    return decl->kind == NODE_FUNCTION && decl->func.generic_param_count == 0;
}

char *module_type_name(ParserContext *ctx, const char *name)
{
    // Names no module declares: a generic parameter, a primitive, `Self`, a C type.
    if (is_known_generic(ctx, name) || is_primitive_type_name(name) ||
        strcasecmp(name, "Self") == 0 || is_extern_symbol(ctx, name))
    {
        return NULL;
    }
    char *own = module_own_name(ctx, name, is_type_declaration);
    if (own || !ctx->imports.current_module_prefix)
    {
        return own;
    }
    if (find_struct_def(ctx, name) || find_type_alias(ctx, name))
    {
        return NULL;
    }
    // Not declared yet anywhere: a type the module declares further down (`next: Node*`).
    return module_prefixed_name(ctx, ctx->imports.current_module_prefix, name);
}

int type_name_declared(ParserContext *ctx, const char *name)
{
    if (is_known_generic(ctx, name) || is_primitive_type_name(name) ||
        strcasecmp(name, "Self") == 0 || is_extern_symbol(ctx, name) ||
        find_struct_def(ctx, name) || find_type_alias_node(ctx, name) ||
        find_generic_template(ctx, name) || find_trait_def(ctx, name))
    {
        return 1;
    }
    if (!ctx->imports.current_module_prefix && imported_by_name(ctx, ctx->current_filename, name))
    {
        return 1;
    }
    if (module_own_name(ctx, name, is_type_declaration))
    {
        return 1;
    }
    char *prefixed = module_prefixed_name(ctx, ctx->imports.current_module_prefix, name);
    return prefixed && find_struct_def(ctx, prefixed);
}

// The pending name whose type is `t`, among `list`, or NULL.
static PendingTypeName *pending_type_of(PendingTypeName *list, Type *t)
{
    for (PendingTypeName *p = list; p; p = p->next)
    {
        if (p->type == t)
        {
            return p;
        }
    }
    return NULL;
}

static void resolve_pending_type_name(ParserContext *ctx, PendingTypeName *list,
                                      PendingTypeName *p);

// Resolves the pending names inside `t` (an alias's type, before it is copied).
static void resolve_pending_inside(ParserContext *ctx, PendingTypeName *list, Type *t)
{
    if (!t)
    {
        return;
    }
    PendingTypeName *p = pending_type_of(list, t);
    if (p)
    {
        resolve_pending_type_name(ctx, list, p);
    }
    resolve_pending_inside(ctx, list, t->inner);
    for (int i = 0; t->args && i < t->count; i++)
    {
        resolve_pending_inside(ctx, list, t->args[i]);
    }
}

static void resolve_pending_type_name(ParserContext *ctx, PendingTypeName *list, PendingTypeName *p)
{
    if (p->state != PENDING_TYPE_WAITING)
    {
        return; // Resolved, or being resolved (an alias that names itself).
    }
    p->state = PENDING_TYPE_RESOLVING;
    char *saved_prefix = ctx->imports.current_module_prefix;
    const char *saved_filename = ctx->current_filename;
    ctx->imports.current_module_prefix = p->module_prefix;
    ctx->current_filename = p->filename;
    Type *t = p->type;
    // A copy of a known alias, or a name (with no arguments) that turned out to be one.
    TypeAlias *alias = p->alias;
    if (!alias && !p->scope && t->kind == TYPE_STRUCT && t->count == 0)
    {
        alias = find_type_alias_node(ctx, p->name);
    }
    if (p->scope)
    {
        // `m::Name`, now that `m` is imported.
        t->name = qualified_type_name(ctx, p->scope, p->name);
        register_type_usage(ctx, t->name, p->token);
    }
    else if (alias)
    {
        // The type the alias names, resolved first (it may name another alias declared further
        // down), in place: what shares the type sees it.
        resolve_pending_inside(ctx, list, alias->type_info);
        int is_const = t->is_const;
        *t = *alias_type(alias);
        t->is_const = t->is_const || is_const;
        register_type_usage(ctx, p->name, p->token);
    }
    else
    {
#if FF_COMPATIBILITY_0_5
        // `Slice_Point`, read before `Point` was registered.
        Type *compat_slice = t->kind == TYPE_STRUCT && t->count == 0
                                 ? compat_slice_from_name(ctx, p->name, p->token)
                                 : NULL;
        if (compat_slice)
        {
            int is_const = t->is_const;
            *t = *compat_slice;
            t->is_const = t->is_const || is_const;
        }
        else
#endif
        {
            char *c_name = type_declaration_name(ctx, p->name);
            if (c_name)
            {
                t->name = c_name;
            }
            register_type_usage(ctx, t->name, p->token);
        }
    }
    ctx->imports.current_module_prefix = saved_prefix;
    ctx->current_filename = saved_filename;
    p->state = PENDING_TYPE_DONE;
}

// Whether the size of the array `t` waits for a constant.
static int size_pending(ParserContext *ctx, Type *t)
{
    for (PendingConstant *p = ctx->pending_constants; p; p = p->next)
    {
        if (p->array == t && !p->done)
        {
            return 1;
        }
    }
    return 0;
}

int type_names_pending(ParserContext *ctx, Type *t)
{
    if (!t)
    {
        return 0;
    }
    if (pending_type_of(ctx->pending_type_names, t) || size_pending(ctx, t))
    {
        return 1;
    }
    for (int i = 0; t->args && i < t->count; i++)
    {
        if (type_names_pending(ctx, t->args[i]))
        {
            return 1;
        }
    }
    return type_names_pending(ctx, t->inner);
}

int array_size_now(ParserContext *ctx, ASTNode *size_expr, int *size)
{
    long long value = 0;
    if (size_expr && eval_const_int_expr(size_expr, ctx, &value))
    {
        *size = (int)value;
        return 1;
    }
    *size = 0;
    return size_expr && !ctx->declarations_read ? 0 : -1;
}

PendingConstant *wait_for_constant(ParserContext *ctx, ASTNode *expr, Type *array, ZenSymbol *def,
                                   Token token, Lexer *start)
{
    PendingConstant *p = xcalloc(1, sizeof(PendingConstant));
    p->expr = expr;
    p->array = array;
    p->def = def;
    p->token = token;
    p->module_prefix = ctx->imports.current_module_prefix;
    p->filename = ctx->current_filename;
    if (start)
    {
        p->start = *start;
        p->read_again = 1;
    }
    p->next = ctx->pending_constants;
    ctx->pending_constants = p;
    return p;
}

// Gives the variant of `p` the value `value`, and the variants that count on from it theirs.
static void set_variant_values(ParserContext *ctx, PendingConstant *p, long long value)
{
    if (value < INT_MIN || value + p->followers > INT_MAX)
    {
        zpanic_at(p->token, "Enum variant value %lld does not fit in an 'int'", value);
        return;
    }
    ASTNode *variant = p->variant;
    for (int i = 0; i <= p->followers && variant; i++, variant = variant->next)
    {
        variant->variant.tag_id = (int)value + i;
        EnumVariantReg *reg = find_enum_variant_of(ctx, p->enum_name, variant->variant.name);
        if (reg)
        {
            reg->tag_id = variant->variant.tag_id;
            reg->tag_pending = 0;
        }
    }
}

void resolve_pending_constants(ParserContext *ctx)
{
    char *saved_prefix = ctx->imports.current_module_prefix;
    const char *saved_filename = ctx->current_filename;
    // What was read before the declarations were registered did not see them: `Code::First`
    // was not a variant yet.
    for (PendingConstant *p = ctx->pending_constants; p; p = p->next)
    {
        if (p->read_again)
        {
            ctx->imports.current_module_prefix = p->module_prefix;
            ctx->current_filename = p->filename;
            Lexer l = p->start;
            p->expr = parse_constant_expression(ctx, &l);
        }
    }
    for (int progress = 1; progress;)
    {
        progress = 0;
        for (PendingConstant *p = ctx->pending_constants; p; p = p->next)
        {
            long long value = 0;
            ctx->imports.current_module_prefix = p->module_prefix;
            ctx->current_filename = p->filename;
            if (p->done || !p->expr || !eval_const_int_expr(p->expr, ctx, &value))
            {
                continue;
            }
            if (p->array)
            {
                p->array->array_size = (int)value;
            }
            else if (p->variant)
            {
                set_variant_values(ctx, p, value);
            }
            else if (p->is_macro)
            {
                define_macro_constant(ctx, p->token, value);
            }
            else
            {
                set_def_value(p->def, value);
                // The value named a `def` that had no type yet: it has its own now.
                if (!p->expr->type_info || p->expr->type_info->kind == TYPE_UNKNOWN)
                {
                    p->expr->type_info = p->def->type_info;
                }
            }
            p->done = 1;
            progress = 1;
        }
    }
    // A `def` or a `#define` with another value (a float, a string) has no integer one; an array
    // size and the value of a variant must.
    for (PendingConstant *p = ctx->pending_constants; p; p = p->next)
    {
        if (!p->done && p->array)
        {
            zpanic_at(p->token, ctx->config->misra_mode
                                    ? "MISRA Rule 18.8"
                                    : "Array size must be a known compile-time constant integer");
        }
        else if (!p->done && p->variant)
        {
            zpanic_at(p->token, "Enum variant value must be a constant integer");
        }
    }
    ctx->pending_constants = NULL;
    ctx->imports.current_module_prefix = saved_prefix;
    ctx->current_filename = saved_filename;
}

void resolve_pending_type_names(ParserContext *ctx)
{
    // In the order read, so the names are checked (validate_types) in the order of the source.
    PendingTypeName *in_order = NULL;
    while (ctx->pending_type_names)
    {
        PendingTypeName *p = ctx->pending_type_names;
        ctx->pending_type_names = p->next;
        p->next = in_order;
        in_order = p;
    }
    for (PendingTypeName *p = in_order; p; p = p->next)
    {
        resolve_pending_type_name(ctx, in_order, p);
    }
}

char *type_declaration_name(ParserContext *ctx, const char *name)
{
    // Imported one by one, or with `as *`: the module's type, by the name the import gives it.
    if (!ctx->imports.current_module_prefix)
    {
        // Imported one by one, or with `as *`: the module's declaration of it.
        SelectiveImport *imported = imported_by_name(ctx, ctx->current_filename, name);
        if (imported)
        {
            return imported_c_name(ctx, imported);
        }
    }
    return module_type_name(ctx, name);
}

static char *module_member_name(ParserContext *ctx, Module *mod, const char *name)
{
    // A C header's names are C's own.
    if (mod->is_c_header)
    {
        register_extern_symbol(ctx, name);
        return xstrdup(name);
    }
    return module_c_name(ctx, mod->base_name, name);
}

char *qualified_type_name(ParserContext *ctx, const char *scope, const char *name)
{
    Module *mod = imported_module(ctx, scope);
    if (!mod)
    {
        return module_c_name(ctx, scope, name);
    }
    // The module's own declaration of the name, by the name it writes: its C name. A name it
    // does not declare (a C header's, one declared further down) keeps today's rule.
    ASTNode *decl = mod->is_c_header ? NULL : find_file_declaration(ctx, mod->path, name);
    const char *c_name = decl ? declaration_c_name(decl) : NULL;
    return c_name ? xstrdup(c_name) : module_member_name(ctx, mod, name);
}

char *module_function_name(ParserContext *ctx, const char *name)
{
    ZenSymbol *local = find_symbol_entry(ctx, name);
    if (local && local->is_local)
    {
        return NULL;
    }
    // Imported one by one, or with `as *`: the module's function, as type_declaration_name
    // finds a type.
    if (!ctx->imports.current_module_prefix)
    {
        SelectiveImport *imported = imported_by_name(ctx, ctx->current_filename, name);
        ASTNode *decl = imported && imported->source_path
                            ? find_file_declaration(ctx, imported->source_path, imported->symbol)
                            : NULL;
        if (decl && is_function_declaration(decl))
        {
            return imported_c_name(ctx, imported);
        }
    }
    return module_own_name(ctx, name, is_function_declaration);
}

// The name programs write for the struct or enum `name`, kept when its C name got a module
// prefix; NULL for one written as it is named.
static const char *written_type_name(void *context, const char *name)
{
    ASTNode *def = find_struct_def((ParserContext *)context, name);
    if (def && def->kind == NODE_STRUCT)
    {
        return def->strct.written_name;
    }
    if (def && def->kind == NODE_ENUM)
    {
        return def->enm.written_name;
    }
    return NULL;
}

char *zen_c_type_spelling(ParserContext *ctx, Type *t)
{
    return type_to_zen_c_string_with(t, written_type_name, ctx);
}

void register_selective_import(ParserContext *ctx, const char *symbol, const char *alias,
                               const char *source_module, const char *source_path)
{
    SelectiveImport *si = xmalloc(sizeof(SelectiveImport));
    si->symbol = xstrdup(symbol);
    si->alias = alias ? xstrdup(alias) : NULL;
    si->source_module = xstrdup(source_module);
    si->source_path = source_path ? xstrdup(source_path) : NULL;
    const char *key = alias ? alias : symbol;
    zmap_put(&ctx->imports.selective_imports, key, si);
}

int imports_name(ParserContext *ctx, const char *file, const char *name)
{
    return imported_by_name(ctx, file, name) != NULL;
}

SelectiveImport *find_selective_import(ParserContext *ctx, const char *name)
{
    SelectiveImport **si = zmap_get(&ctx->imports.selective_imports, name);
    return si ? *si : NULL;
}

void re_export_propagated(ParserContext *ctx, const char *alias, const char *parent_prefix,
                          const char *base_name)
{
    if (!parent_prefix || !alias)
    {
        return;
    }
    char *combined = mangle_module_symbol(parent_prefix, alias);

    if (!zmap_get(&ctx->imports.modules, combined))
    {
        Module *new_mod = xmalloc(sizeof(Module));
        new_mod->alias = xstrdup(combined);
        new_mod->path = NULL;
        new_mod->base_name = xstrdup(base_name ? base_name : alias);
        new_mod->is_c_header = 0;
        new_mod->is_re_export = 1;
        zmap_put(&ctx->imports.modules, xstrdup(combined), new_mod);
    }
}

void re_export_wildcard_symbols(ParserContext *ctx, const char *module_base,
                                const char *module_path)
{
    if (!ctx->global_scope || !module_base)
    {
        return;
    }
    size_t prefix_len = strlen(module_base) + 2;
    char *prefix = xmalloc(prefix_len + 1);
    snprintf(prefix, prefix_len + 1, "%s__", module_base);

    ZenSymbol *sym = ctx->global_scope->symbols;
    while (sym)
    {
        if (sym->name && strncmp(sym->name, prefix, (size_t)(prefix_len)) == 0)
        {
            const char *bare_name = sym->name + prefix_len;
            if (!*bare_name)
            {
                sym = sym->next;
                continue;
            }

            if (zmap_get(&ctx->imports.selective_imports, bare_name))
            {
                sym = sym->next;
                continue;
            }

            register_selective_import(ctx, bare_name, NULL, module_base, module_path);
        }
        sym = sym->next;
    }
    zfree(prefix);
}

// Both separators, whatever the platform, as extract_module_name accepts them.
static int is_path_sep(char c)
{
    return c == '/' || c == '\\';
}

// The symbol prefix of a module imported with `as`, selectively or as `*`.
// Normally its file name (`helpers.zc` → `helpers`); when another file already
// owns that prefix (`net/helpers.zc` and `fs/helpers.zc`), the parent
// directory is prepended (`fs_helpers`), then a counter if still taken.
// The same path always gets the same prefix within a build.
char *module_prefix_for(ParserContext *ctx, const char *path)
{
    const char **known = zmap_get(&ctx->imports.module_prefixes, path);
    if (known)
    {
        return xstrdup(*known);
    }

    char *prefix = extract_module_name(path);
    const char **owner = zmap_get(&ctx->imports.prefix_owners, prefix);
    if (owner && strcmp(*owner, path) != 0)
    {
        // `<parent dir>_<file>`: `end` is the last separator, `start` the one before it.
        const char *end = NULL;
        for (const char *c = path; *c; c++)
        {
            if (is_path_sep(*c))
            {
                end = c;
            }
        }
        const char *start = end ? end : path;
        while (start > path && !is_path_sep(start[-1]))
        {
            start--;
        }
        size_t dir_len = end ? (size_t)(end - start) : 0;
        size_t cap = dir_len + strlen(prefix) + 16;
        char *qualified = xmalloc(cap);
        snprintf(qualified, cap, "%.*s_%s", (int)dir_len, start, prefix);
        for (char *c = qualified; *c; c++)
        {
            if (!isalnum((unsigned char)*c))
            {
                *c = '_';
            }
        }
        int counter = 2;
        char *candidate = xstrdup(qualified);
        while ((owner = zmap_get(&ctx->imports.prefix_owners, candidate)) &&
               strcmp(*owner, path) != 0)
        {
            zfree(candidate);
            candidate = xmalloc(cap);
            snprintf(candidate, cap, "%s%d", qualified, counter++);
        }
        zfree(qualified);
        zfree(prefix);
        prefix = candidate;
    }

    char *stored_path = xstrdup(path);
    char *stored_prefix = xstrdup(prefix);
    zmap_put(&ctx->imports.module_prefixes, stored_path, stored_prefix);
    zmap_put(&ctx->imports.prefix_owners, stored_prefix, stored_path);
    return prefix;
}

char *extract_module_name(const char *path)
{
    const char *slash = (char *)strrchr(path, '/');
    const char *backslash = (char *)strrchr(path, '\\');
    if (backslash && (!slash || backslash > slash))
    {
        slash = backslash;
    }

    const char *base = slash ? slash + 1 : path;
    const char *dot = (char *)strrchr(base, '.');
    int len = dot ? (int)(dot - base) : (int)strlen(base);
    char *name = xmalloc((size_t)(len + 1));
    strncpy(name, base, (size_t)(len));
    name[len] = 0;

    for (int i = 0; i < len; i++)
    {
        if (!isalnum(name[i]))
        {
            name[i] = '_';
        }
    }

    return name;
}

int is_ident_char(char c)
{
    return isalnum(c) || c == '_';
}

int is_file_imported(ParserContext *ctx, const char *path)
{
    return zmap_get(&ctx->imports.imported_files, path) != NULL;
}

void record_file_dependency(ParserContext *ctx, const char *importer, const char *imported)
{
    if (!importer || !imported || strcmp(importer, imported) == 0)
    {
        return;
    }
    for (FileDependency *d = ctx->file_dependencies; d; d = d->next)
    {
        if (strcmp(d->importer, importer) == 0 && strcmp(d->imported, imported) == 0)
        {
            return;
        }
    }
    FileDependency *d = xmalloc(sizeof(FileDependency));
    d->importer = xstrdup(importer);
    d->imported = xstrdup(imported);
    d->next = ctx->file_dependencies;
    ctx->file_dependencies = d;
}

// Whether `from` reaches `to` through the imports, without going through a file in `seen`.
static int file_reaches(ParserContext *ctx, const char *from, const char *to, zvec_Text *seen)
{
    if (strcmp(from, to) == 0)
    {
        return 1;
    }
    for (size_t i = 0; i < seen->length; i++)
    {
        if (strcmp(seen->data[i], from) == 0)
        {
            return 0;
        }
    }
    zvec_push_Text(seen, (char *)from);
    for (FileDependency *d = ctx->file_dependencies; d; d = d->next)
    {
        if (strcmp(d->importer, from) == 0 && file_reaches(ctx, d->imported, to, seen))
        {
            return 1;
        }
    }
    return 0;
}

int file_sees(ParserContext *ctx, const char *from, const char *to)
{
    zvec_Text seen = {0};
    return file_reaches(ctx, from, to, &seen);
}

void mark_file_imported(ParserContext *ctx, const char *path)
{
    // Duplicate into the arena so the map key outlives callers that free the
    // original (e.g. driver.c stores realpath() results which it libc_frees).
    zmap_put(&ctx->imports.imported_files, xstrdup(path), xstrdup(path));
}

void register_impl(ParserContext *ctx, const char *trait, const char *strct)
{
    ImplReg *r = xmalloc(sizeof(ImplReg));
    r->trait = xstrdup(trait);
    r->strct = xstrdup(strct);
    r->next = ctx->registered_impls;
    ctx->registered_impls = r;
}

int check_impl(ParserContext *ctx, const char *trait, const char *strct)
{
    for (ImplReg *r = ctx->registered_impls; r; r = r->next)
    {
        if (r->trait[0] == trait[0] && strcmp(r->trait, trait) == 0 && r->strct[0] == strct[0] &&
            strcmp(r->strct, strct) == 0)
        {
            return 1;
        }
    }
    return 0;
}

FuncSig *find_func(ParserContext *ctx, const char *name)
{
    FuncSig **registered = zmap_get(&ctx->func_registry, name);
    if (registered)
    {
        return *registered;
    }

    // A method of the generic impl being parsed, which is a template and registers none.
    if (ctx->current_impl_methods)
    {
        ASTNode *n = ctx->current_impl_methods;
        while (n)
        {
            if (n->kind == NODE_FUNCTION && n->func.name && strcmp(function_c_name(n), name) == 0)
            {
                FuncSig *sig = xcalloc(1, sizeof(FuncSig));
                sig->name = function_c_name(n);
                sig->own_name = n->func.is_method ? n->func.name : NULL;
                sig->decl_token = n->token;
                sig->total_args = n->func.count;
                sig->default_values = n->func.default_values;
                sig->arg_types = n->func.arg_types;
                sig->ret_type = n->func.ret_type_info;
                sig->is_varargs = n->func.is_varargs;
                return sig;
            }
            n = n->next;
        }
    }

    return NULL;
}

// Whether `node` is a named top-level declaration (not an impl, an import or a test).
static int is_named_declaration(ASTNode *node)
{
    switch (node->kind)
    {
    case NODE_FUNCTION:
    case NODE_STRUCT:
    case NODE_ENUM:
    case NODE_TRAIT:
    case NODE_TYPE_ALIAS:
    case NODE_CONST:
    case NODE_VAR_DECL:
        return 1;
    default:
        return 0;
    }
}

void register_declaration(ParserContext *ctx, ASTNode *node)
{
    // Its name as its own file writes it: the token of the name, which the node keeps. Not the
    // C name (`mod__Point`), nor the importer's (written_name, `geo::Point`).
    if (!node || !is_named_declaration(node) || !node->token.file || node->token.len == 0)
    {
        return;
    }
    char *name = token_strdup(node->token);
    if (!ctx->declarations.hash_func)
    {
        ctx->declarations = zmap_init(DeclByName, zmap_hash_cstr, zmap_cmp_cstr);
    }
    zmap_put(&ctx->declarations, file_key(node->token.file, name), node);
}

ASTNode *find_file_declaration(ParserContext *ctx, const char *file, const char *name)
{
    if (!file || !name || !ctx->declarations.hash_func)
    {
        return NULL;
    }
    char *key = file_key(file, name);
    ASTNode **found = zmap_get(&ctx->declarations, key);
    zfree(key);
    return found ? *found : NULL;
}

void record_file_import(ParserContext *ctx, const char *alias, const char *path, Module *module)
{
    if (!ctx->current_filename || !alias)
    {
        return;
    }
    Module *own = xmalloc(sizeof(Module));
    *own = *module;
    own->alias = xstrdup(alias);
    own->path = xstrdup(path);
    zmap_put(&ctx->imports.file_modules, file_key(ctx->current_filename, alias), own);
}

void record_file_selective_import(ParserContext *ctx, const char *symbol, const char *alias,
                                  const char *source_module, const char *source_path)
{
    if (!ctx->current_filename)
    {
        return;
    }
    SelectiveImport *si = xmalloc(sizeof(SelectiveImport));
    si->symbol = xstrdup(symbol);
    si->alias = alias ? xstrdup(alias) : NULL;
    si->source_module = xstrdup(source_module);
    si->source_path = source_path ? xstrdup(source_path) : NULL;
    zmap_put(&ctx->imports.file_selective_imports,
             file_key(ctx->current_filename, alias ? alias : symbol), si);
}

void record_file_wildcard_import(ParserContext *ctx, const char *module_base, const char *path)
{
    if (!ctx->current_filename || !module_base || !path)
    {
        return;
    }
    zmap_put(&ctx->imports.file_wildcard_imports, file_key(ctx->current_filename, module_base),
             xstrdup(path));
}
