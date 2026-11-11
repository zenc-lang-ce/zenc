// SPDX-License-Identifier: MIT
#include "ast.h"
#include "primitives.h"
#include "../parser/parser.h"
#include "../constants.h"
#include "../arena.h"
#include "../utils/colors.h"
#include "../utils/utils.h"
#include <stdlib.h>
#include <string.h>

// Local trait registry — avoids dependency on g_parser_ctx.
static TraitReg *registered_traits_local = NULL;

void register_trait(const char *name)
{
    TraitReg *r = xmalloc(sizeof(TraitReg));
    r->name = xstrdup(name);
    r->next = registered_traits_local;
    registered_traits_local = r;
}

void clear_registered_traits(void)
{
    // The TraitReg nodes are arena-allocated, so the arena reset/restore in the
    // fuzz harness and the LSP reclaims them. The list head must be cleared
    // there too, otherwise it dangles into reused arena memory and is_trait()
    // walks a cyclic list forever.
    registered_traits_local = NULL;
}

int is_trait(const char *name)
{
    if (!name)
    {
        return 0;
    }

    // Strip trailing stars for pointer types (e.g., IAnimal*)
    char *base = xstrdup(name);

    // Strip "struct " or "union " if present
    if (strncmp(base, "struct ", 7) == 0)
    {
        char *nb = xstrdup(base + 7);
        zfree(base);
        base = nb;
    }
    else if (strncmp(base, "union ", 6) == 0)
    {
        char *nb = xstrdup(base + 6);
        zfree(base);
        base = nb;
    }

    char *p = (char *)strchr(base, '*');
    if (p)
    {
        *p = '\0';
    }

    TraitReg *r = registered_traits_local;
    while (r)
    {
        if (0 == strcmp(r->name, base))
        {
            zfree(base);
            return 1;
        }
        r = r->next;
    }
    zfree(base);
    return 0;
}

ASTNode *ast_create(NodeType type)
{
    ASTNode *node = xmalloc(sizeof(ASTNode));
    memset(node, 0, sizeof(ASTNode));
    node->kind = type;
    return node;
}

void ast_free(ASTNode *node)
{
    if (node->kind == NODE_AST_COMMENT)
    {
        if (node->comment.content)
        {
            zfree(node->comment.content);
        }
    }
    zfree(node);
}

static void add_child(ASTChildren *c, ASTNode **slot)
{
    c->slots[c->count] = slot;
    c->count++;
}

// Every NodeType must have a case: a new kind fails to compile until it says
// which children it has (with GCC and Clang; TCC does not know these pragmas).
#if defined(__GNUC__) && !defined(__TINYC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch-enum"
#endif
ASTChildren ast_children(ASTNode *node)
{
    ASTChildren c = {0};
    if (!node)
    {
        return c;
    }

    switch (node->kind)
    {
    // Declarations and blocks.
    case NODE_ROOT:
        add_child(&c, &node->root.children);
        break;
    case NODE_FUNCTION:
        add_child(&c, &node->func.body);
        break;
    case NODE_BLOCK:
        add_child(&c, &node->block.statements);
        break;
    case NODE_VAR_DECL:
    case NODE_CONST:
        add_child(&c, &node->var_decl.init_expr);
        break;
    case NODE_DESTRUCT_VAR:
        add_child(&c, &node->destruct.init_expr);
        add_child(&c, &node->destruct.else_block);
        break;
    case NODE_STRUCT:
        add_child(&c, &node->strct.fields);
        break;
    case NODE_ENUM:
        add_child(&c, &node->enm.variants);
        break;
    case NODE_TRAIT:
        add_child(&c, &node->trait.methods);
        break;
    case NODE_IMPL:
        add_child(&c, &node->impl.methods);
        break;
    case NODE_IMPL_TRAIT:
        add_child(&c, &node->impl_trait.methods);
        break;
    case NODE_TEST:
        add_child(&c, &node->test_stmt.body);
        break;
    case NODE_COMPTIME:
        add_child(&c, &node->comptime.body);
        add_child(&c, &node->comptime.generated);
        break;

    // Statements.
    case NODE_RETURN:
        add_child(&c, &node->ret.value);
        break;
    case NODE_IF:
        add_child(&c, &node->if_stmt.condition);
        add_child(&c, &node->if_stmt.then_body);
        add_child(&c, &node->if_stmt.else_body);
        break;
    case NODE_WHILE:
        add_child(&c, &node->while_stmt.condition);
        add_child(&c, &node->while_stmt.body);
        break;
    case NODE_FOR:
        add_child(&c, &node->for_stmt.init);
        add_child(&c, &node->for_stmt.condition);
        add_child(&c, &node->for_stmt.step);
        add_child(&c, &node->for_stmt.body);
        break;
    case NODE_FOR_RANGE:
        add_child(&c, &node->for_range.start);
        add_child(&c, &node->for_range.end);
        add_child(&c, &node->for_range.body);
        break;
    case NODE_LOOP:
        add_child(&c, &node->loop_stmt.body);
        break;
    case NODE_MATCH:
        add_child(&c, &node->match_stmt.expr);
        add_child(&c, &node->match_stmt.cases);
        break;
    case NODE_MATCH_CASE:
        add_child(&c, &node->match_case.guard);
        add_child(&c, &node->match_case.body);
        break;
    case NODE_DEFER:
        add_child(&c, &node->defer_stmt.stmt);
        break;
    case NODE_ASSERT:
    case NODE_EXPECT:
        add_child(&c, &node->assert_stmt.condition);
        break;
    case NODE_GOTO:
        add_child(&c, &node->goto_stmt.goto_expr);
        break;
    case NODE_REPL_PRINT:
        add_child(&c, &node->repl_print.expr);
        break;

    // Expressions.
    case NODE_EXPR_BINARY:
        add_child(&c, &node->binary.left);
        add_child(&c, &node->binary.right);
        break;
    case NODE_EXPR_UNARY:
        add_child(&c, &node->unary.operand);
        break;
    case NODE_EXPR_CALL:
        add_child(&c, &node->call.callee);
        add_child(&c, &node->call.args);
        break;
    case NODE_EXPR_MEMBER:
        add_child(&c, &node->member.target);
        break;
    case NODE_EXPR_INDEX:
        add_child(&c, &node->index.array);
        add_child(&c, &node->index.index);
        add_child(&c, &node->index.extra_indices);
        break;
    case NODE_EXPR_SLICE:
        add_child(&c, &node->slice.array);
        add_child(&c, &node->slice.start);
        add_child(&c, &node->slice.end);
        break;
    case NODE_EXPR_CAST:
        add_child(&c, &node->cast.expr);
        break;
    case NODE_EXPR_SIZEOF:
    case NODE_TYPEOF:
        add_child(&c, &node->size_of.expr);
        break;
    case NODE_EXPR_STRUCT_INIT:
        add_child(&c, &node->struct_init.fields);
        break;
    case NODE_FIELD_INIT:
        add_child(&c, &node->var_decl.init_expr);
        break;
    case NODE_EXPR_ARRAY_LITERAL:
        add_child(&c, &node->array_literal.elements);
        break;
    case NODE_EXPR_TUPLE_LITERAL:
        add_child(&c, &node->tuple_literal.elements);
        break;
    case NODE_TERNARY:
        add_child(&c, &node->ternary.cond);
        add_child(&c, &node->ternary.true_expr);
        add_child(&c, &node->ternary.false_expr);
        break;
    case NODE_LAMBDA:
        add_child(&c, &node->lambda.body);
        break;
    case NODE_TRY:
        add_child(&c, &node->try_stmt.expr);
        break;
    case NODE_CUDA_LAUNCH:
        add_child(&c, &node->cuda_launch.call);
        add_child(&c, &node->cuda_launch.grid);
        add_child(&c, &node->cuda_launch.block);
        add_child(&c, &node->cuda_launch.shared_mem);
        add_child(&c, &node->cuda_launch.stream);
        break;
    case NODE_VA_START:
        add_child(&c, &node->va_start_args.ap);
        add_child(&c, &node->va_start_args.last_arg);
        break;
    case NODE_VA_END:
        add_child(&c, &node->va_end_args.ap);
        break;
    case NODE_VA_COPY:
        add_child(&c, &node->va_copy_args.dest);
        add_child(&c, &node->va_copy_args.src);
        break;
    case NODE_VA_ARG:
        add_child(&c, &node->va_arg_val.ap);
        break;

    // No children. NODE_IMPORT and the f-string nodes do hold nodes, but not as
    // children: see ast_children() in ast.h.
    case NODE_EXPR_LITERAL:
    case NODE_EXPR_VAR:
    case NODE_BREAK:
    case NODE_CONTINUE:
    case NODE_LABEL:
    case NODE_TYPE_ALIAS:
    case NODE_FIELD:
    case NODE_ENUM_VARIANT:
    case NODE_INCLUDE:
    case NODE_LINK:
    case NODE_IMPORT:
    case NODE_RAW_STMT:
    case NODE_PREPROC_DIRECTIVE:
    case NODE_ASM:
    case NODE_PLUGIN:
    case NODE_REFLECTION:
    case NODE_AST_COMMENT:
    case NODE_EXPR_FSTRING:
    case NODE_FSTRING_PART:
    case NODE_ERRONEOUS:
        break;

    default:
        break;
    }
    return c;
}
#if defined(__GNUC__) && !defined(__TINYC__)
#pragma GCC diagnostic pop
#endif

Type *type_new(TypeKind kind)
{
    Type *t = xmalloc(sizeof(Type));
    memset(t, 0, sizeof(Type));
    t->kind = kind;
    if (kind == TYPE_FUNCTION)
    {
        t->traits.has_drop = 1;
    }
    t->name = NULL;
    t->inner = NULL;
    t->args = NULL;
    t->count = 0;
    t->is_const = 0;
    t->is_explicit_struct = 0;
    t->is_raw = 0;
    t->array_size = 0;
    return t;
}

Type *type_new_ptr(Type *inner)
{
    Type *t = type_new(TYPE_POINTER);
    t->inner = inner;
    return t;
}

Type *type_new_array(Type *inner, int size)
{
    Type *t = type_new(TYPE_ARRAY);
    t->inner = inner;
    t->array_size = size;
    return t;
}

// Whether a type still names a generic parameter (`T`, `Vec<T>*`).
static int type_names_generic(Type *t)
{
    if (!t)
    {
        return 0;
    }
    if (t->kind == TYPE_GENERIC || type_names_generic(t->inner))
    {
        return 1;
    }
    for (int i = 0; t->args && i < t->count; i++)
    {
        if (type_names_generic(t->args[i]))
        {
            return 1;
        }
    }
    return 0;
}

int type_is_generic_param(Type *t, const char *param)
{
    // Parsed outside its template's body, a parameter is still a struct of its name.
    return t && param && (t->kind == TYPE_GENERIC || t->kind == TYPE_STRUCT) && t->name &&
           strcmp(t->name, param) == 0;
}

int type_names_param(Type *t, const char *param)
{
    if (!t)
    {
        return 0;
    }
    if (type_is_generic_param(t, param) || type_names_param(t->inner, param))
    {
        return 1;
    }
    for (int i = 0; t->args && i < t->count; i++)
    {
        if (type_names_param(t->args[i], param))
        {
            return 1;
        }
    }
    return 0;
}

char *type_def_name(Type *t)
{
    // An instance has a definition of its own; inside its template (`Vec<T>`), the template's.
    if (t && (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->count > 0 && t->args &&
        !type_names_generic(t))
    {
        return type_to_string(t);
    }
    return t ? t->name : NULL;
}

int type_is_instance_of(Type *t, const char *tpl)
{
    return t && (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM) && t->count > 0 && t->args &&
           t->name && strcmp(t->name, tpl) == 0;
}

char *type_impl_name(Type *t)
{
    if (t && (t->kind == TYPE_STRUCT || t->kind == TYPE_ENUM || t->kind == TYPE_ALIAS))
    {
        return type_def_name(t);
    }
    return t ? type_mangle_spelling(t) : NULL;
}

// Keywords of C (C23) and of C++ (C++23, with the alternative tokens), sorted for bsearch;
// not `true`, `false` and `nullptr`, which programs use as values.
static const char *const C_KEYWORDS[] = {"_Alignas",       "_Alignof",
                                         "_Atomic",        "_BitInt",
                                         "_Bool",          "_Complex",
                                         "_Decimal128",    "_Decimal32",
                                         "_Decimal64",     "_Generic",
                                         "_Imaginary",     "_Noreturn",
                                         "_Static_assert", "_Thread_local",
                                         "alignas",        "alignof",
                                         "auto",           "bool",
                                         "break",          "case",
                                         "char",           "const",
                                         "constexpr",      "continue",
                                         "default",        "do",
                                         "double",         "else",
                                         "enum",           "extern",
                                         "float",          "for",
                                         "goto",           "if",
                                         "inline",         "int",
                                         "long",           "register",
                                         "restrict",       "return",
                                         "short",          "signed",
                                         "sizeof",         "static",
                                         "static_assert",  "struct",
                                         "switch",         "thread_local",
                                         "typedef",        "typeof",
                                         "typeof_unqual",  "union",
                                         "unsigned",       "void",
                                         "volatile",       "while"};

static const char *const CPP_KEYWORDS[] = {"alignas",
                                           "alignof",
                                           "and",
                                           "and_eq",
                                           "asm",
                                           "auto",
                                           "bitand",
                                           "bitor",
                                           "bool",
                                           "break",
                                           "case",
                                           "catch",
                                           "char",
                                           "char16_t",
                                           "char32_t",
                                           "char8_t",
                                           "class",
                                           "co_await",
                                           "co_return",
                                           "co_yield",
                                           "compl",
                                           "concept",
                                           "const",
                                           "const_cast",
                                           "consteval",
                                           "constexpr",
                                           "constinit",
                                           "continue",
                                           "decltype",
                                           "default",
                                           "delete",
                                           "do",
                                           "double",
                                           "dynamic_cast",
                                           "else",
                                           "enum",
                                           "explicit",
                                           "export",
                                           "extern",
                                           "float",
                                           "for",
                                           "friend",
                                           "goto",
                                           "if",
                                           "inline",
                                           "int",
                                           "long",
                                           "mutable",
                                           "namespace",
                                           "new",
                                           "noexcept",
                                           "not",
                                           "not_eq",
                                           "operator",
                                           "or",
                                           "or_eq",
                                           "private",
                                           "protected",
                                           "public",
                                           "register",
                                           "reinterpret_cast",
                                           "requires",
                                           "return",
                                           "short",
                                           "signed",
                                           "sizeof",
                                           "static",
                                           "static_assert",
                                           "static_cast",
                                           "struct",
                                           "switch",
                                           "template",
                                           "this",
                                           "thread_local",
                                           "throw",
                                           "try",
                                           "typedef",
                                           "typeid",
                                           "typename",
                                           "union",
                                           "unsigned",
                                           "using",
                                           "virtual",
                                           "void",
                                           "volatile",
                                           "wchar_t",
                                           "while",
                                           "xor",
                                           "xor_eq"};

static int compare_keyword(const void *key, const void *entry)
{
    return strcmp((const char *)key, *(const char *const *)entry);
}

static int is_keyword_in(const char *name, const char *const *list, size_t count)
{
    return bsearch(name, list, count, sizeof(list[0]), compare_keyword) != NULL;
}

const char *c_safe_name(const char *name, int cpp)
{
    if (!name || !(is_keyword_in(name, C_KEYWORDS, sizeof(C_KEYWORDS) / sizeof(C_KEYWORDS[0])) ||
                   (cpp && is_keyword_in(name, CPP_KEYWORDS,
                                         sizeof(CPP_KEYWORDS) / sizeof(CPP_KEYWORDS[0])))))
    {
        return name;
    }
    size_t len = strlen(name) + sizeof("__zc");
    char *safe = xmalloc(len);
    snprintf(safe, len, "%s__zc", name);
    return safe;
}

Type *type_new_enum(const char *name)
{
    Type *t = type_new(TYPE_ENUM);
    t->name = xstrdup(name);
    return t;
}

Type *type_new_instance(const char *tpl, Type **args, int count)
{
    Type *t = type_new(TYPE_STRUCT);
    t->name = xstrdup(tpl);
    t->args = args;
    t->count = count;
    return t;
}

Type *type_new_tuple(Type **elements, int count)
{
    Type *t = type_new(TYPE_TUPLE);
    t->args = elements;
    t->count = count;
    return t;
}

Type *slice_element(Type *t)
{
    if (t && t->kind == TYPE_STRUCT && t->count == 1 && t->args && t->name &&
        strcmp(t->name, "Slice") == 0)
    {
        return t->args[0];
    }
    return NULL;
}

int is_owning_closure(Type *t)
{
    return t && t->kind == TYPE_FUNCTION && t->traits.has_drop && !t->is_raw;
}

Type *tuple_of(Type *t)
{
    if (t && t->kind == TYPE_POINTER)
    {
        t = t->inner;
    }
    return t && t->kind == TYPE_TUPLE ? t : NULL;
}

Type *tuple_element_type(Type *tuple, const char *field)
{
    // The program writes `t.0`; C names the field `v0`, and code may write that too.
    const char *digits = field[0] == 'v' ? field + 1 : field;
    if (!*digits)
    {
        return NULL;
    }
    int index = 0;
    for (const char *p = digits; *p; p++)
    {
        if (*p < '0' || *p > '9' || index > tuple->count)
        {
            return NULL;
        }
        index = index * 10 + (*p - '0');
    }
    return index < tuple->count ? tuple->args[index] : NULL;
}

Type *type_clone(Type *t)
{
    if (!t)
    {
        return NULL;
    }
    Type *clone = xmalloc(sizeof(Type));
    memcpy(clone, t, sizeof(Type));

    // In Zen C, we only clone the top-level Type struct to isolate usage-site metadata
    // like lifetime_depth. However, we SHIELD the recursive structures (inner, args)
    // by sharing their pointers. This ensures that type inference (which may happen
    // at a usage site) correctly propagates back to the canonical Type objects
    // that the codegen and other passes depend on.
    // clone->inner = t->inner; // Already done by memcpy
    // clone->args = t->args;   // Already done by memcpy

    // Note: Strings like name are shared (shallow copy)
    // but names are usually static or managed by pctx string pool.
    return clone;
}

static int is_char_ptr(Type *t)
{
    if (!t)
    {
        return 0;
    }
    // Handle both primitive char* and legacy struct char*.
    if (TYPE_POINTER == t->kind && t->inner && TYPE_CHAR == t->inner->kind)
    {
        return 1;
    }
    if (TYPE_POINTER == t->kind && t->inner && TYPE_STRUCT == t->inner->kind && t->inner->name &&
        0 == strcmp(t->inner->name, "char"))
    {
        return 1;
    }
    return 0;
}

int is_integer_type(Type *t)
{
    if (!t)
    {
        return 0;
    }

    if (t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        return is_integer_type(t->inner);
    }

    int res =
        (t->kind == TYPE_INT || t->kind == TYPE_CHAR || t->kind == TYPE_BOOL ||
         t->kind == TYPE_I8 || t->kind == TYPE_U8 || t->kind == TYPE_I16 || t->kind == TYPE_U16 ||
         t->kind == TYPE_I32 || t->kind == TYPE_U32 || t->kind == TYPE_I64 || t->kind == TYPE_U64 ||
         t->kind == TYPE_USIZE || t->kind == TYPE_ISIZE || t->kind == TYPE_BYTE ||
         t->kind == TYPE_RUNE || t->kind == TYPE_UINT || t->kind == TYPE_I128 ||
         t->kind == TYPE_U128 || t->kind == TYPE_BITINT || t->kind == TYPE_UBITINT ||
         t->kind == TYPE_C_INT || t->kind == TYPE_C_UINT || t->kind == TYPE_C_LONG ||
         t->kind == TYPE_C_ULONG || t->kind == TYPE_C_LONGLONG || t->kind == TYPE_C_ULONGLONG ||
         t->kind == TYPE_C_SHORT || t->kind == TYPE_C_USHORT || t->kind == TYPE_C_CHAR ||
         t->kind == TYPE_C_UCHAR ||
         (t->kind == TYPE_STRUCT && t->name &&
          (0 == strcmp(t->name, "int8_t") || 0 == strcmp(t->name, "uint8_t") ||
           0 == strcmp(t->name, "int16_t") || 0 == strcmp(t->name, "uint16_t") ||
           0 == strcmp(t->name, "int32_t") || 0 == strcmp(t->name, "uint32_t") ||
           0 == strcmp(t->name, "int64_t") || 0 == strcmp(t->name, "uint64_t") ||
           0 == strcmp(t->name, "size_t") || 0 == strcmp(t->name, "ssize_t") ||
           0 == strcmp(t->name, "ptrdiff_t"))));
    return res;
}

int is_unsigned_type(Type *t)
{
    if (!t)
    {
        return 0;
    }

    if (t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        return is_unsigned_type(t->inner);
    }

    int res =
        (t->kind == TYPE_U8 || t->kind == TYPE_U16 || t->kind == TYPE_U32 || t->kind == TYPE_U64 ||
         t->kind == TYPE_USIZE || t->kind == TYPE_UINT || t->kind == TYPE_U128 ||
         t->kind == TYPE_UBITINT || t->kind == TYPE_C_UINT || t->kind == TYPE_C_ULONG ||
         t->kind == TYPE_C_ULONGLONG || t->kind == TYPE_C_USHORT || t->kind == TYPE_C_UCHAR ||
         (t->kind == TYPE_STRUCT && t->name &&
          (0 == strcmp(t->name, "uint8_t") || 0 == strcmp(t->name, "uint16_t") ||
           0 == strcmp(t->name, "uint32_t") || 0 == strcmp(t->name, "uint64_t") ||
           0 == strcmp(t->name, "size_t"))));
    return res;
}

// The type an alias stands for, through every alias of an alias.
int is_char_type(Type *t)
{
    if (!t)
    {
        return 0;
    }
    Type *res = resolve_alias(t);
    return (res->kind == TYPE_CHAR || res->kind == TYPE_C_CHAR || res->kind == TYPE_C_UCHAR);
}

Type *resolve_alias(Type *t)
{
    while (t && t->kind == TYPE_ALIAS && t->inner)
    {
        t = t->inner;
    }
    return t;
}

int integer_type_width(Type *t)
{
    if (!t)
    {
        return 0;
    }
    switch (t->kind)
    {
    case TYPE_I8:
    case TYPE_U8:
    case TYPE_BYTE:
    case TYPE_C_CHAR:
    case TYPE_C_UCHAR:
        return 8;
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_C_SHORT:
    case TYPE_C_USHORT:
        return 16;
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_INT:
    case TYPE_UINT:
    case TYPE_RUNE:
    case TYPE_C_INT:
    case TYPE_C_UINT:
        return 32;
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_ISIZE:
    case TYPE_USIZE:
    case TYPE_C_LONG:
    case TYPE_C_ULONG:
    case TYPE_C_LONGLONG:
    case TYPE_C_ULONGLONG:
        return 64;
    case TYPE_I128:
    case TYPE_U128:
        return 128;
    default:
        return 0;
    }
}

// The type C gives an operation on two integers, so the one the generated code computes in:
// the wider of the two, and the unsigned one when they are as wide (`8` and a usize give a
// usize). Either may be an alias: the one chosen is returned as it is.
Type *common_integer_type(Type *a, Type *b)
{
    int width_a = integer_type_width(resolve_alias(a));
    int width_b = integer_type_width(resolve_alias(b));
    if (width_a != width_b)
    {
        return width_a > width_b ? a : b;
    }
    return (is_unsigned_type(resolve_alias(b)) && !is_unsigned_type(resolve_alias(a))) ? b : a;
}

// An integer literal written without a suffix (`15`), whose type adapts to the other operand.
static int is_plain_int_literal(ASTNode *n)
{
    return n && n->kind == NODE_EXPR_LITERAL && n->literal.kind == LITERAL_INT && n->type_info &&
           n->type_info->kind == TYPE_INT;
}

Type *integer_operation_type(ASTNode *left, ASTNode *right)
{
    if (is_plain_int_literal(left) && !is_plain_int_literal(right))
    {
        return right->type_info;
    }
    if (is_plain_int_literal(right))
    {
        return left->type_info;
    }
    return common_integer_type(left->type_info, right->type_info);
}

int is_signed_type(Type *t)
{
    if (!t)
    {
        return 0;
    }

    if (t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        return is_signed_type(t->inner);
    }

    int res =
        (t->kind == TYPE_I8 || t->kind == TYPE_I16 || t->kind == TYPE_I32 || t->kind == TYPE_I64 ||
         t->kind == TYPE_ISIZE || t->kind == TYPE_INT || t->kind == TYPE_I128 ||
         t->kind == TYPE_BITINT || t->kind == TYPE_C_INT || t->kind == TYPE_C_LONG ||
         t->kind == TYPE_C_LONGLONG || t->kind == TYPE_C_SHORT || t->kind == TYPE_C_CHAR ||
         (t->kind == TYPE_STRUCT && t->name &&
          (0 == strcmp(t->name, "int8_t") || 0 == strcmp(t->name, "int16_t") ||
           0 == strcmp(t->name, "int32_t") || 0 == strcmp(t->name, "int64_t") ||
           0 == strcmp(t->name, "ssize_t") || 0 == strcmp(t->name, "ptrdiff_t"))));
    return res;
}

int is_boolean_type(Type *t)
{
    if (!t)
    {
        return 0;
    }

    if (t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        return is_boolean_type(t->inner);
    }

    return (t->kind == TYPE_BOOL);
}

int is_float_type(Type *t)
{
    if (!t)
    {
        return 0;
    }

    if (t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        return is_float_type(t->inner);
    }

    int res = (t->kind == TYPE_FLOAT || t->kind == TYPE_F32 || t->kind == TYPE_F64);
    return res;
}

int is_incomplete_type(struct ParserContext *ctx, Type *t)
{
    if (!t || t->kind != TYPE_STRUCT || !t->name)
    {
        return 0;
    }
    ASTNode *def = find_struct_def(ctx, type_def_name(t));
    if (!def)
    {
        return 1;
    }
    return def->strct.is_incomplete;
}

int is_composite_expression(ASTNode *node)
{
    if (!node)
    {
        return 0;
    }

    switch (node->kind)
    {
    case NODE_EXPR_BINARY:
        return 1;
    case NODE_EXPR_UNARY:
        return 1;
    case NODE_TERNARY:
        return 1;
    default:
        return 0;
    }
}

int type_eq(Type *a, Type *b)
{
    if (!a || !b)
    {
        return 0;
    }

    if (a == b)
    {
        return 1;
    }

    if (a->kind == TYPE_UNKNOWN || b->kind == TYPE_UNKNOWN)
    {
        return 1;
    }

    // Lax integer matching (bool == int, char == i8, etc.).
    if (is_integer_type(a) && is_integer_type(b))
    {
        return 1;
    }

    // Lax float matching.
    if (is_float_type(a) && is_float_type(b))
    {
        return 1;
    }

    // String Literal vs char*
    if (a->kind == TYPE_STRING && is_char_ptr(b))
    {
        return 1;
    }

    if (b->kind == TYPE_STRING && is_char_ptr(a))
    {
        return 1;
    }

    if (a->kind != b->kind)
    {
        if (!((a->kind == TYPE_STRUCT && b->kind == TYPE_ENUM) ||
              (a->kind == TYPE_ENUM && b->kind == TYPE_STRUCT)))
        {
            return 0;
        }
    }

    if (a->kind == TYPE_STRUCT || a->kind == TYPE_GENERIC || a->kind == TYPE_ENUM)
    {
        // The same definition: for instances of a generic, the same template and arguments.
        if (!a->name || !b->name)
        {
            return 0;
        }
        return 0 == strcmp(type_def_name(a), type_def_name(b));
    }
    if (a->kind == TYPE_ALIAS)
    {
        if (a->alias.is_opaque_alias)
        {
            if (b->kind != TYPE_ALIAS || !b->alias.is_opaque_alias)
            {
                return 0;
            }
            return 0 == strcmp(a->name, b->name);
        }
        return type_eq(a->inner, b);
    }
    if (a->kind == TYPE_POINTER)
    {
        return type_eq(a->inner, b->inner);
    }
    if (a->kind == TYPE_FUNCTION)
    {
        if (a->is_raw != b->is_raw)
        {
            return 0;
        }
        if (a->count != b->count)
        {
            return 0;
        }
        if (!type_eq(a->inner, b->inner))
        {
            return 0;
        }
        for (int i = 0; i < a->count; i++)
        {
            if (!type_eq(a->args[i], b->args[i]))
            {
                return 0;
            }
        }
        return 1;
    }
    if (a->kind == TYPE_ARRAY || a->kind == TYPE_VECTOR)
    {
        return a->array_size == b->array_size && type_eq(a->inner, b->inner);
    }
    if (a->kind == TYPE_TUPLE)
    {
        // Two tuples are one C struct only if their elements are spelt alike: (int, bool) is not
        // (u8, bool), though type_eq lets an int stand for a u8.
        char *name_a = type_to_string(a);
        char *name_b = type_to_string(b);
        int same = strcmp(name_a, name_b) == 0;
        zfree(name_a);
        zfree(name_b);
        return same;
    }

    return 1;
}

static char *type_spelling(Type *t, int c_names);

// The name of the C struct a tuple is: `Tuple__` and its elements' names, as `(int, bool*)`
// gives `Tuple__int__boolPtr`.
static char *tuple_struct_name(Type *t)
{
    size_t length = 0;
    char **elements = xmalloc(sizeof(char *) * (size_t)(t->count > 0 ? t->count : 1));
    for (int i = 0; i < t->count; i++)
    {
        elements[i] = type_spelling(t->args[i], 1);
        length += strlen(elements[i]) + 2;
    }
    char *signature = xmalloc(length + 1);
    signature[0] = 0;
    for (int i = 0; i < t->count; i++)
    {
        if (i > 0)
        {
            strcat(signature, "__");
        }
        strcat(signature, elements[i]);
        zfree(elements[i]);
    }
    zfree(elements);
    char *clean = sanitize_mangled_name(signature);
    zfree(signature);
    char *name = xmalloc(strlen(clean) + sizeof("Tuple__"));
    sprintf(name, "Tuple__%s", clean); /* safe */
    zfree(clean);
    return name;
}

static char *type_spelling_impl(Type *t, int c_names);

// A type as type_to_string spells it, or, with `c_names`, as the C names built from it spell
// it (type_mangle_spelling).
static char *type_spelling(Type *t, int c_names)
{
    if (!t)
    {
        return xstrdup("void");
    }
    char *res = type_spelling_impl(t, c_names);
    if (t->is_const)
    {
        char *final = xmalloc(strlen(res) + 7);
        sprintf(final, "const %s", res); /* safe */
        zfree(res);
        return final;
    }
    return res;
}

char *type_to_string(Type *t)
{
    return type_spelling(t, 0);
}

char *type_mangle_spelling(Type *t)
{
    return type_spelling(t, 1);
}

static char *type_spelling_impl(Type *t, int c_names)
{
    if (!t)
    {
        return xstrdup("void");
    }

    // People read a fixed-width primitive by the name the program writes (`i32`); a C name
    // spells it as C does (`Vec__int32_t`).
    const char *zen_name = c_names ? NULL : primitive_zen_name(t->kind);
    if (zen_name)
    {
        return xstrdup(zen_name);
    }

    switch (t->kind)
    {
    case TYPE_VOID:
        return xstrdup("void");
    case TYPE_BOOL:
        return xstrdup("bool");
    case TYPE_STRING:
        return xstrdup("string");
    case TYPE_CHAR:
        return xstrdup("char");
    case TYPE_I8:
        return xstrdup("int8_t");
    case TYPE_U8:
        return xstrdup("uint8_t");
    case TYPE_I16:
        return xstrdup("int16_t");
    case TYPE_U16:
        return xstrdup("uint16_t");
    case TYPE_I32:
        return xstrdup("int32_t");
    case TYPE_U32:
        return xstrdup("uint32_t");
    case TYPE_I64:
        return xstrdup("int64_t");
    case TYPE_U64:
        return xstrdup("uint64_t");
    case TYPE_F32:
        return xstrdup("float");
    case TYPE_F64:
        return xstrdup("double");
    case TYPE_USIZE:
        return xstrdup("size_t");
    case TYPE_ISIZE:
        return xstrdup("ptrdiff_t");
    case TYPE_BYTE:
        return xstrdup("uint8_t");
    case TYPE_I128:
        return xstrdup("__int128");
    case TYPE_U128:
        return xstrdup("unsigned __int128");
    case TYPE_RUNE:
        // Not `int32_t`, which `int` also is: an instance of `rune` is not one of `int`.
        return xstrdup("rune");
    case TYPE_UINT:
        return xstrdup("unsigned int");

    // Portable C Types
    case TYPE_C_INT:
        return xstrdup("c_int");
    case TYPE_C_UINT:
        return xstrdup("c_uint");
    case TYPE_C_LONG:
        return xstrdup("c_long");
    case TYPE_C_ULONG:
        return xstrdup("c_ulong");
    case TYPE_C_LONGLONG:
        return xstrdup("c_longlong");
    case TYPE_C_ULONGLONG:
        return xstrdup("c_ulonglong");
    case TYPE_C_SHORT:
        return xstrdup("c_short");
    case TYPE_C_USHORT:
        return xstrdup("c_ushort");
    case TYPE_C_CHAR:
        return xstrdup("c_char");
    case TYPE_C_UCHAR:
        return xstrdup("c_uchar");

    case TYPE_INT:
        // `int` is `i32`: its C names are i32's (`Vec__int32_t`), so both make one instance.
        return xstrdup(c_names ? "int32_t" : "int");
    case TYPE_FLOAT:
        return xstrdup("float");
    case TYPE_BITINT:
    {
        char *res = xmalloc(32);
        sprintf(res, "i%d", t->array_size); /* safe */
        return res;
    }
    case TYPE_UBITINT:
    {
        char *res = xmalloc(32);
        sprintf(res, "u%d", t->array_size); /* safe */
        return res;
    }

    case TYPE_VECTOR:
    {
        if (t->name)
        {
            return xstrdup(t->name);
        }
        char *inner = type_spelling(t->inner, c_names);
        char *res = xmalloc(strlen(inner) + 20);
        sprintf(res, "%sx%d", inner, t->array_size); /* safe */
        zfree(inner);
        return res;
    }

    case TYPE_POINTER:
    {
        char *inner = type_spelling(t->inner, c_names);
        if (t->is_restrict)
        {
            char *res = xmalloc(strlen(inner) + 16);
            sprintf(res, "%s* __restrict", inner); /* safe */
            return res;
        }
        else
        {
            char *res = xmalloc(strlen(inner) + 2);
            sprintf(res, "%s*", inner); /* safe */
            return res;
        }
    }

    case TYPE_ARRAY:
    {
        Type *base = t;
        int *dims = NULL;
        int dims_cap = 0;
        int dims_count = 0;

        while (base->kind == TYPE_ARRAY)
        {
            if (dims_count == dims_cap)
            {
                dims_cap = dims_cap == 0 ? 4 : dims_cap * 2;
                dims = xrealloc(dims, sizeof(int) * (size_t)(dims_cap));
            }
            dims[dims_count++] = base->array_size;
            base = base->inner;
        }

        char *inner = type_spelling(base, c_names);
        size_t total_len = strlen(inner) + 1;
        for (int i = 0; i < dims_count; i++)
        {
            total_len += 20;
        }

        char *res = xmalloc((size_t)(total_len));
        strcpy(res, inner);
        zfree(inner);

        char *p = res + strlen(res);
        for (int i = 0; i < dims_count; i++)
        {
            snprintf(p, 20, "[%d]", dims[i]);
            p += strlen(p);
        }

        if (dims)
        {
            zfree(dims);
        }
        return res;
    }

    case TYPE_FUNCTION:
    {
        if (t->is_raw)
        {
            // fn*(Args)->Ret
            char *ret = type_spelling(t->inner, c_names);
            char *res = xmalloc(strlen(ret) + 64);
            snprintf(res, strlen(ret) + 64, "fn*(");

            for (int i = 0; i < t->count; i++)
            {
                if (i > 0)
                {
                    char *tmp = xmalloc(strlen(res) + 3);
                    snprintf(tmp, strlen(res) + 3, "%s, ", res);
                    zfree(res);
                    res = tmp;
                }
                char *arg = type_spelling(t->args[i], c_names);
                char *tmp = xmalloc(strlen(res) + strlen(arg) + 1);
                sprintf(tmp, "%s%s", res, arg); /* safe */
                zfree(res);
                res = tmp;
                zfree(arg);
            }
            if (t->is_varargs)
            {
                char *tmp = xmalloc(strlen(res) + 6);
                sprintf(tmp, "%s, ...", res); /* safe */
                zfree(res);
                res = tmp;
            }
            char *tmp = xmalloc(strlen(res) + strlen(ret) + 6); // ) -> Ret
            sprintf(tmp, "%s) -> %s", res, ret);                /* safe */
            zfree(res);
            res = tmp;
            zfree(ret);
            return res;
        }

        // fn(Args) -> Ret
        char *ret = type_spelling(t->inner, c_names);
        char *res = xmalloc(strlen(ret) + 64);
        snprintf(res, strlen(ret) + 64, "fn(");

        for (int i = 0; i < t->count; i++)
        {
            if (i > 0)
            {
                char *tmp = xmalloc(strlen(res) + 3);
                snprintf(tmp, strlen(res) + 3, "%s, ", res);
                zfree(res);
                res = tmp;
            }
            char *arg = type_spelling(t->args[i], c_names);
            char *tmp = xmalloc(strlen(res) + strlen(arg) + 1);
            sprintf(tmp, "%s%s", res, arg); /* safe */
            zfree(res);
            res = tmp;
            zfree(arg);
        }
        char *tmp = xmalloc(strlen(res) + strlen(ret) + 6); // ) -> Ret
        sprintf(tmp, "%s) -> %s", res, ret);                /* safe */
        zfree(res);
        res = tmp;
        zfree(ret);
        return res;
    }

    case TYPE_STRUCT:
    case TYPE_ENUM:
    case TYPE_GENERIC:
    {
        // An instance of a generic is named after its template and its arguments.
        if (t->count > 0 && t->args && t->name)
        {
            char **args = xmalloc((size_t)t->count * sizeof(char *));
            for (int i = 0; i < t->count; i++)
            {
                args[i] = type_spelling(t->args[i], 1);
            }
            return mangle_generic_name(t->name, args, t->count);
        }
        return xstrdup(t->name);
    }
    case TYPE_TUPLE:
        return tuple_struct_name(t);
    case TYPE_ALIAS:
        // The name the program uses: a @link_name only shows in C (type_to_c_string).
        return xstrdup(t->name);

    default:
        return xstrdup("unknown");
    }
}

// C-compatible type stringifier.
// Strictly uses 'struct T' for explicit structs to support external types.
// Does NOT mangle pointers to 'Ptr'.
static char *type_to_c_string_impl(Type *t, int misra_mode, CNameLookup lookup, void *context);

char *type_to_c_string(Type *t, int misra_mode)
{
    return type_to_c_string_with(t, misra_mode, NULL, NULL);
}

// The C name a struct or enum declares for itself with @link_name, or NULL.
static const char *link_name_of_type(Type *t, CNameLookup lookup, void *context)
{
    const char *name = type_def_name(t);
    return (lookup && name) ? lookup(context, name) : NULL;
}

// The C name of a struct or enum: its @link_name, else the name its definition has.
static const char *c_name_of(Type *t, CNameLookup lookup, void *context)
{
    const char *link_name = link_name_of_type(t, lookup, context);
    return link_name ? link_name : type_def_name(t);
}

char *type_to_c_string_with(Type *t, int misra_mode, CNameLookup lookup, void *context)
{
    if (!t)
    {
        return xstrdup("void");
    }
    char *res = type_to_c_string_impl(t, misra_mode, lookup, context);
    if (t->is_const)
    {
        char *final = xmalloc(strlen(res) + 7);
        sprintf(final, "const %s", res); /* safe */
        zfree(res);
        return final;
    }
    return res;
}

static char *type_to_c_string_impl(Type *t, int misra_mode, CNameLookup lookup, void *context)
{
    if (!t)
    {
        return xstrdup("void");
    }
    if (t->c_typedef_name)
    {
        return xstrdup(t->c_typedef_name);
    }

    switch (t->kind)
    {
    case TYPE_VOID:
        return xstrdup("void");
    case TYPE_STRUCT:
    {
        const char *c_name = c_name_of(t, lookup, context);
        // A struct with a C name of its own is written by it alone; otherwise "struct Foo" only
        // when written so (else a typedef).
        if (!c_name || link_name_of_type(t, lookup, context) || !t->is_explicit_struct)
        {
            return xstrdup(c_name);
        }
        const char *keyword = t->is_union ? "union" : "struct";
        char *res = xmalloc(strlen(keyword) + strlen(c_name) + 2);
        sprintf(res, "%s %s", keyword, c_name); /* safe */
        return res;
    }
    case TYPE_TUPLE:
        return tuple_struct_name(t);
    case TYPE_BOOL:
        return xstrdup("bool");
    case TYPE_STRING:
        return xstrdup("string");
    case TYPE_CHAR:
        return xstrdup("char");
    case TYPE_I8:
        return xstrdup("int8_t");
    case TYPE_U8:
        return xstrdup("uint8_t");
    case TYPE_I16:
        return xstrdup("int16_t");
    case TYPE_U16:
        return xstrdup("uint16_t");
    case TYPE_I32:
        return xstrdup("int32_t");
    case TYPE_U32:
        return xstrdup("uint32_t");
    case TYPE_I64:
        return xstrdup("int64_t");
    case TYPE_U64:
        return xstrdup("uint64_t");
    case TYPE_F32:
        return xstrdup("float");
    case TYPE_F64:
        return xstrdup("double");
    case TYPE_USIZE:
        return xstrdup("size_t");
    case TYPE_ISIZE:
        return xstrdup("ptrdiff_t");
    case TYPE_BYTE:
        return xstrdup("uint8_t");
    case TYPE_I128:
        return xstrdup("__int128");
    case TYPE_U128:
        return xstrdup("unsigned __int128");
    case TYPE_RUNE:
        return xstrdup("int32_t");
    case TYPE_UINT:
        return xstrdup("unsigned int");

    // Portable C Types (Map directly to C types)
    case TYPE_C_INT:
        return xstrdup(misra_mode ? "int32_t" : "int");
    case TYPE_C_UINT:
        return xstrdup(misra_mode ? "uint32_t" : "unsigned int");
    case TYPE_C_LONG:
        return xstrdup(misra_mode ? "int64_t" : "long");
    case TYPE_C_ULONG:
        return xstrdup(misra_mode ? "uint64_t" : "unsigned long");
    case TYPE_C_LONGLONG:
        return xstrdup(misra_mode ? "int64_t" : "long long");
    case TYPE_C_ULONGLONG:
        return xstrdup(misra_mode ? "uint64_t" : "unsigned long long");
    case TYPE_C_SHORT:
        return xstrdup(misra_mode ? "int16_t" : "short");
    case TYPE_C_USHORT:
        return xstrdup(misra_mode ? "uint16_t" : "unsigned short");
    case TYPE_C_CHAR:
        return xstrdup(misra_mode ? "int8_t" : "char");
    case TYPE_C_UCHAR:
        return xstrdup(misra_mode ? "uint8_t" : "unsigned char");

    case TYPE_INT:
        // 'int' in Zen C maps to 'i32' now for portability.
        // FFI should use c_int.
        return xstrdup("int32_t");
    case TYPE_FLOAT:
        return xstrdup("float");
    case TYPE_BITINT:
    {
        char *res = xmalloc(32);
        sprintf(res, "_BitInt(%d)", t->array_size); /* safe */
        return res;
    }
    case TYPE_UBITINT:
    {
        char *res = xmalloc(40);
        sprintf(res, "unsigned _BitInt(%d)", t->array_size); /* safe */
        return res;
    }

    case TYPE_VECTOR:
    {
        if (t->name)
        {
            return xstrdup(t->name);
        }
        char *inner = type_to_c_string_with(t->inner, misra_mode, lookup, context);
        char *res = xmalloc(strlen(inner) + 32);
        sprintf(res, "ZC_SIMD(%s, %d)", inner, t->array_size); /* safe */
        zfree(inner);
        return res;
    }

    case TYPE_POINTER:
    {
        char *inner = type_to_c_string_with(t->inner, misra_mode, lookup, context);
        char *ptr_token = (char *)strstr(inner, "(*");
        if (ptr_token)
        {
            ptrdiff_t prefix_len = ptr_token - inner + 2; // "void (*"
            char *res = xmalloc(strlen(inner) + 2);
            strncpy(res, inner, (size_t)(prefix_len));
            res[prefix_len] = 0;
            strcat(res, "*");
            strcat(res, ptr_token + 2);
            zfree(inner);
            return res;
        }

        if (t->is_restrict)
        {
            char *res = xmalloc(strlen(inner) + 16);
            sprintf(res, "%s* __restrict", inner); /* safe */
            return res;
        }
        else
        {
            char *res = xmalloc(strlen(inner) + 2);
            sprintf(res, "%s*", inner); /* safe */
            return res;
        }
    }

    case TYPE_ARRAY:
    {
        Type *base = t;
        int *dims = NULL;
        int dims_cap = 0;
        int dims_count = 0;

        while (base->kind == TYPE_ARRAY)
        {
            if (dims_count == dims_cap)
            {
                dims_cap = dims_cap == 0 ? 4 : dims_cap * 2;
                dims = xrealloc(dims, sizeof(int) * (size_t)(dims_cap));
            }
            dims[dims_count++] = base->array_size;
            base = base->inner;
        }

        char *inner = type_to_c_string_with(base, misra_mode, lookup, context);
        size_t total_len = strlen(inner) + 1;
        for (int i = 0; i < dims_count; i++)
        {
            total_len += 20;
        }

        char *res = xmalloc((size_t)(total_len));
        strcpy(res, inner);
        zfree(inner);

        char *p = res + strlen(res);
        for (int i = 0; i < dims_count; i++)
        {
            snprintf(p, 20, "[%d]", dims[i]);
            p += strlen(p);
        }

        if (dims)
        {
            zfree(dims);
        }
        return res;
    }

    case TYPE_FUNCTION:
        if (t->is_raw)
        {
            char *ret = type_to_c_string_with(t->inner, misra_mode, lookup, context);
            char *res = xmalloc(strlen(ret) + 64); // heuristic start buffer
            snprintf(res, strlen(ret) + 64, "%s (*)(", ret);

            for (int i = 0; i < t->count; i++)
            {
                if (i > 0)
                {
                    char *tmp = xmalloc(strlen(res) + 3);
                    snprintf(tmp, strlen(res) + 3, "%s, ", res);
                    zfree(res);
                    res = tmp;
                }
                char *arg = type_to_c_string_with(t->args[i], misra_mode, lookup, context);
                char *tmp = xmalloc(strlen(res) + strlen(arg) + 1);
                sprintf(tmp, "%s%s", res, arg); /* safe */
                zfree(res);
                res = tmp;
                zfree(arg);
            }
            if (t->is_varargs)
            {
                if (t->count > 0)
                {
                    char *tmp = xmalloc(strlen(res) + 6);
                    sprintf(tmp, "%s, ...", res); /* safe */
                    zfree(res);
                    res = tmp;
                }
                else
                {
                    char *tmp = xmalloc(strlen(res) + 4);
                    sprintf(tmp, "%s...", res); /* safe */
                    zfree(res);
                    res = tmp;
                }
            }
            char *tmp = xmalloc(strlen(res) + 2);
            sprintf(tmp, "%s)", res); /* safe */
            zfree(res);
            res = tmp;
            zfree(ret);
            return res;
        }
        if (t->inner)
        {
            zfree(type_to_c_string_with(t->inner, misra_mode, lookup, context));
        }
        return xstrdup("z_closure_T");

    case TYPE_GENERIC:
        // Use type_to_string to get the mangled name (e.g. Option_int) instead of raw C string
        // composition This ensures consistency with struct definitions.
        {
            char *s = type_to_string(t);
            return s;
        }

    case TYPE_ALIAS:
        if (t->alias.is_opaque_alias)
        {
            return xstrdup(t->name);
        }
        return type_to_c_string_with(t->inner, misra_mode, lookup, context);

    case TYPE_ENUM:
        return xstrdup(c_name_of(t, lookup, context));

    case TYPE_UNSAFE_ANY:
        return xstrdup("any");
    default:
        return xstrdup("unknown");
    }
}

Type *get_inner_type(Type *t)
{
    while (t && t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        t = t->inner;
    }
    return t;
}

ASTNode *deref_to_one_pointer(ASTNode *expr)
{
    Type *t = get_inner_type(expr->type_info);
    Type *pointee = t && t->kind == TYPE_POINTER ? get_inner_type(t->inner) : NULL;
    while (pointee && pointee->kind == TYPE_POINTER)
    {
        ASTNode *deref = ast_create(NODE_EXPR_UNARY);
        deref->unary.op = xstrdup("*");
        deref->unary.operand = expr;
        deref->type_info = t->inner;
        deref->token = expr->token;
        expr = deref;
        t = pointee;
        pointee = get_inner_type(t->inner);
    }
    return expr;
}

// Type inference — resolve an expression node to its C type string.
// (Moved from codegen_utils.c this is part of core, not codegen)

char *infer_type(ParserContext *ctx, ASTNode *node)
{
    if (!node || !ctx)
    {
        return NULL;
    }

    if (node->type_info && node->type_info->kind != TYPE_UNKNOWN)
    {
        char *t =
            type_to_c_string_with(node->type_info, ctx->config->misra_mode, link_name_of, ctx);
        return t;
    }

    if (node->kind == NODE_EXPR_LITERAL)
    {
        if (node->type_info)
        {
            return type_to_c_string_with(node->type_info, ctx->config->misra_mode, link_name_of,
                                         ctx);
        }
        return NULL;
    }

    if (node->kind == NODE_EXPR_VAR)
    {
        ZenSymbol *sym = find_symbol_entry(ctx, node->var_ref.name);
        if (sym && sym->type_info)
        {
            return type_to_c_string_with(sym->type_info, ctx->config->misra_mode, link_name_of,
                                         ctx);
        }
    }

    if (node->kind == NODE_EXPR_CALL)
    {
        if (node->call.callee->kind == NODE_EXPR_VAR)
        {
            FuncSig *sig = find_func(ctx, node->call.callee->var_ref.name);
            if (sig)
            {
                if (sig->ret_type)
                {
                    return type_to_c_string_with(sig->ret_type, ctx->config->misra_mode,
                                                 link_name_of, ctx);
                }
            }

            // Fallback for known stdlib memory/file functions.
            if (strcmp(node->call.callee->var_ref.name, "malloc") == 0 ||
                strcmp(node->call.callee->var_ref.name, "calloc") == 0 ||
                strcmp(node->call.callee->var_ref.name, "realloc") == 0 ||
                strcmp(node->call.callee->var_ref.name, "fopen") == 0 ||
                strcmp(node->call.callee->var_ref.name, "popen") == 0 ||
                strcmp(node->call.callee->var_ref.name, "fdopen") == 0)
            {
                return "void*";
            }
            ASTNode *sdef = find_struct_def(ctx, node->call.callee->var_ref.name);
            if (sdef)
            {
                return node->call.callee->var_ref.name;
            }

            // Check for enum variants (constructors)
            TypeMember *variant = &node->call.callee->var_ref.variant;
            EnumVariantReg *ev =
                variant->member
                    ? find_enum_variant_of(ctx, type_def_name(variant->owner), variant->member)
                    : find_enum_variant(ctx, node->call.callee->var_ref.name);
            if (ev)
            {
                return ev->enum_name;
            }
        }
        // Method call: target.method() - look up Type_method signature.
        if (node->call.callee->kind == NODE_EXPR_MEMBER)
        {
            char *target_type = infer_type(ctx, node->call.callee->member.target);
            if (target_type)
            {
                char clean_type[MAX_TYPE_NAME_LEN];
                snprintf(clean_type, sizeof(clean_type), "%s", target_type);

                // Robustly strip all pointer levels for method lookup
                char *ptr = (char *)strchr(clean_type, '*');
                if (ptr)
                {
                    *ptr = 0;
                }

                char *base = clean_type;
                if (strncmp(base, "struct ", 7) == 0)
                {
                    base += 7;
                }

                char *func_name = mangle_method_symbol(base, NULL, node->call.callee->member.field);

                FuncSig *sig = find_func(ctx, func_name);

                if (sig && sig->ret_type)
                {
                    char *ret = type_to_c_string_with(sig->ret_type, ctx->config->misra_mode,
                                                      link_name_of, ctx);
                    zfree(func_name);
                    return ret;
                }
                zfree(func_name);
            }
        }

        if (node->call.callee->kind == NODE_EXPR_VAR)
        {
            ZenSymbol *sym = find_symbol_entry(ctx, node->call.callee->var_ref.name);
            if (sym && sym->type_info && sym->type_info->kind == TYPE_FUNCTION &&
                sym->type_info->inner)
            {
                return type_to_c_string_with(sym->type_info->inner, ctx->config->misra_mode,
                                             link_name_of, ctx);
            }
        }
    }

    if (node->kind == NODE_TRY)
    {
        char *inner_type = infer_type(ctx, node->try_stmt.expr);
        if (inner_type)
        {
            // Extract T from Result<T> or Option<T>
            char *start = (char *)strchr(inner_type, '<');
            if (start)
            {
                start++; // Skip <
                char *end = (char *)strrchr(inner_type, '>');
                if (end && end > start)
                {
                    ptrdiff_t len = end - start;
                    char *extracted = xmalloc((size_t)(len + 1));
                    strncpy(extracted, start, (size_t)(len));
                    extracted[len] = 0;
                    return extracted;
                }
            }

            // Find the struct/enum definition and look for "Ok" or "val"
            char *search_name = inner_type;
            if (strncmp(search_name, "struct ", 7) == 0)
            {
                search_name += 7;
            }

            ASTNode *def = find_struct_def(ctx, search_name);
            if (!def)
            {
                // check enums list explicitly if not found in instantiated list
                StructRef *er = ctx->parsed_enums_list;
                while (er)
                {
                    if (er->node && er->node->kind == NODE_ENUM &&
                        strcmp(er->node->enm.name, search_name) == 0)
                    {
                        def = er->node;
                        break;
                    }
                    er = er->next;
                }
            }

            if (def)
            {
                if (def->kind == NODE_ENUM)
                {
                    // Look for "Ok" variant
                    ASTNode *var = def->enm.variants;
                    while (var)
                    {
                        if (var->variant.name && strcmp(var->variant.name, "Ok") == 0)
                        {
                            if (var->variant.payload)
                            {
                                return type_to_c_string_with(var->variant.payload,
                                                             ctx->config->misra_mode, link_name_of,
                                                             ctx);
                            }
                            // Ok with no payload? Then it's void/u0.
                            return "void";
                        }
                        var = var->next;
                    }
                }
                else if (def->kind == NODE_STRUCT)
                {
                    // Look for "val" field
                    ASTNode *field = def->strct.fields;
                    while (field)
                    {
                        if (field->field.name && strcmp(field->field.name, "val") == 0)
                        {
                            return type_to_string(field->type_info);
                        }
                        field = field->next;
                    }
                }
            }
        }
    }

    if (node->kind == NODE_EXPR_MEMBER)
    {
        char *parent_type = infer_type(ctx, node->member.target);
        if (!parent_type)
        {
            return NULL;
        }

        char clean_name[MAX_TYPE_NAME_LEN];
        snprintf(clean_name, sizeof(clean_name), "%s", parent_type);
        char *ptr = (char *)strchr(clean_name, '*');
        if (ptr)
        {
            *ptr = 0;
        }

        return get_field_type_str(ctx, clean_name, node->member.field);
    }

    if (node->kind == NODE_EXPR_BINARY)
    {
        if (strcmp(node->binary.op, "??") == 0)
        {
            return infer_type(ctx, node->binary.left);
        }

        const char *op = node->binary.op;
        char *left_type = infer_type(ctx, node->binary.left);
        char *right_type = infer_type(ctx, node->binary.right);

        int is_logical = (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0 || strcmp(op, "==") == 0 ||
                          strcmp(op, "!=") == 0 || strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
                          strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0);

        if (is_logical)
        {
            return xstrdup("int");
        }

        if (left_type && strcmp(left_type, "usize") == 0)
        {
            return "usize";
        }
        if (right_type && strcmp(right_type, "usize") == 0)
        {
            return "usize";
        }
        if (left_type && strcmp(left_type, "double") == 0)
        {
            return "double";
        }

        return left_type ? left_type : right_type;
    }

    if (node->kind == NODE_MATCH)
    {
        ASTNode *case_node = node->match_stmt.cases;
        while (case_node)
        {
            char *type = infer_type(ctx, case_node->match_case.body);
            if (type && strcmp(type, "void") != 0 && strcmp(type, "unknown") != 0)
            {
                return type;
            }
            case_node = case_node->next;
        }
        return NULL;
    }

    if (node->kind == NODE_EXPR_INDEX)
    {
        char *array_type = infer_type(ctx, node->index.array);
        if (array_type)
        {
            // If T*, returns T. If T[], returns T.
            char *ptr = (char *)strrchr(array_type, '*');
            if (ptr)
            {
                ptrdiff_t len = ptr - array_type;
                char *buf = xmalloc((size_t)(len + 1));
                strncpy(buf, array_type, (size_t)(len));
                buf[len] = 0;
                return buf;
            }

            char *search_name = array_type;
            if (strncmp(search_name, "struct ", 7) == 0)
            {
                search_name += 7;
            }

            ASTNode *def = find_struct_def(ctx, search_name);
            if (def && def->type_info && def->type_info->kind == TYPE_VECTOR &&
                def->type_info->inner)
            {
                return type_to_c_string_with(def->type_info->inner, ctx->config->misra_mode,
                                             link_name_of, ctx);
            }
        }
        return "int";
    }

    if (node->kind == NODE_EXPR_UNARY)
    {
        if (strcmp(node->unary.op, "&") == 0)
        {
            char *inner = infer_type(ctx, node->unary.operand);
            if (inner)
            {
                char *buf = xmalloc(strlen(inner) + 2);
                sprintf(buf, "%s*", inner); /* safe */
                return buf;
            }
        }
        if (strcmp(node->unary.op, "*") == 0)
        {
            char *inner = infer_type(ctx, node->unary.operand);
            if (inner)
            {
                if (strcmp(inner, "string") == 0)
                {
                    return xstrdup("char");
                }
                char *ptr = (char *)strchr(inner, '*');
                if (ptr)
                {
                    // Return base type (naive)
                    ptrdiff_t len = ptr - inner;
                    char *dup = xmalloc((size_t)(len + 1));
                    strncpy(dup, inner, (size_t)(len));
                    dup[len] = 0;
                    return dup;
                }
            }
        }
        return infer_type(ctx, node->unary.operand);
    }

    if (node->kind == NODE_EXPR_CAST)
    {
        return type_to_c_string_with(node->type_info, ctx->config->misra_mode, link_name_of, ctx);
    }

    if (node->kind == NODE_EXPR_STRUCT_INIT)
    {
        return node->struct_init.struct_name;
    }

    if (node->kind == NODE_EXPR_ARRAY_LITERAL)
    {
        if (node->type_info)
        {
            return type_to_c_string_with(node->type_info, ctx->config->misra_mode, link_name_of,
                                         ctx);
        }
        return NULL;
    }

    if (node->kind == NODE_EXPR_LITERAL)
    {
        if (node->literal.kind == LITERAL_STRING)
        {
            return xstrdup("string");
        }
        if (node->literal.kind == LITERAL_CHAR)
        {
            return xstrdup("char");
        }
        if (node->literal.kind == LITERAL_FLOAT)
        {
            return "double";
        }
        return "int";
    }

    return NULL;
}

// Extract variable names from argument string.

// Parse original method name from mangled name.

// Replace string type in arguments.

// Helper to emit auto type or fallback.

// Field type lookup — get the type string of a struct field.
// (Moved from codegen_utils.c)

char *get_field_type_str(ParserContext *ctx, const char *struct_name, const char *field_name)
{
    char clean_name[MAX_TYPE_NAME_LEN];
    strncpy(clean_name, struct_name, sizeof(clean_name) - 1);
    clean_name[sizeof(clean_name) - 1] = 0;

    char *ptr = (char *)strchr(clean_name, '<');
    if (ptr)
    {
        *ptr = 0;
    }

    ASTNode *def = find_struct_def(ctx, clean_name);
    if (!def)
    {
        return NULL;
    }

    ASTNode *f = def->strct.fields;
    while (f)
    {
        if (strcmp(f->field.name, field_name) == 0)
        {
            return type_to_string(f->type_info);
        }
        f = f->next;
    }
    return NULL;
}

// Type inference.
