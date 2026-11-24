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
        add_child(&c, &node->for_range.step);
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
    case NODE_IMPLICIT_CONVERSION:
        add_child(&c, &node->conversion.expr);
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
    case NODE_EMBED:
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

ZEN_CONST
const char *ast_kind_name(NodeType kind)
{
    switch (kind)
    {
    case NODE_ROOT:
        return "ROOT";
    case NODE_FUNCTION:
        return "FUNCTION";
    case NODE_BLOCK:
        return "BLOCK";
    case NODE_RETURN:
        return "RETURN";
    case NODE_VAR_DECL:
        return "VAR_DECL";
    case NODE_FIELD_INIT:
        return "FIELD_INIT";
    case NODE_CONST:
        return "CONST";
    case NODE_TYPE_ALIAS:
        return "TYPE_ALIAS";
    case NODE_IF:
        return "IF";
    case NODE_WHILE:
        return "WHILE";
    case NODE_FOR:
        return "FOR";
    case NODE_FOR_RANGE:
        return "FOR_RANGE";
    case NODE_LOOP:
        return "LOOP";
    case NODE_BREAK:
        return "BREAK";
    case NODE_CONTINUE:
        return "CONTINUE";
    case NODE_MATCH:
        return "MATCH";
    case NODE_MATCH_CASE:
        return "MATCH_CASE";
    case NODE_EXPR_BINARY:
        return "BINARY";
    case NODE_EXPR_UNARY:
        return "UNARY";
    case NODE_EXPR_LITERAL:
        return "LITERAL";
    case NODE_EXPR_VAR:
        return "VAR";
    case NODE_EXPR_CALL:
        return "CALL";
    case NODE_EXPR_MEMBER:
        return "MEMBER";
    case NODE_EXPR_INDEX:
        return "INDEX";
    case NODE_EXPR_CAST:
        return "CAST";
    case NODE_IMPLICIT_CONVERSION:
        return "CONVERSION";
    case NODE_EMBED:
        return "EMBED";
    case NODE_EXPR_SIZEOF:
        return "SIZEOF";
    case NODE_EXPR_STRUCT_INIT:
        return "STRUCT_INIT";
    case NODE_EXPR_ARRAY_LITERAL:
        return "ARRAY_LIT";
    case NODE_EXPR_TUPLE_LITERAL:
        return "TUPLE";
    case NODE_EXPR_SLICE:
        return "SLICE";
    case NODE_STRUCT:
        return "STRUCT";
    case NODE_FIELD:
        return "FIELD";
    case NODE_ENUM:
        return "ENUM";
    case NODE_ENUM_VARIANT:
        return "ENUM_VARIANT";
    case NODE_TRAIT:
        return "TRAIT";
    case NODE_IMPL:
        return "IMPL";
    case NODE_IMPL_TRAIT:
        return "IMPL_TRAIT";
    case NODE_INCLUDE:
        return "INCLUDE";
    case NODE_IMPORT:
        return "IMPORT";
    case NODE_RAW_STMT:
        return "RAW";
    case NODE_EXPR_FSTRING:
        return "FSTRING";
    case NODE_FSTRING_PART:
        return "FSTRING_PART";
    case NODE_GOTO:
        return "GOTO";
    case NODE_LABEL:
        return "LABEL";
    case NODE_DEFER:
        return "DEFER";
    case NODE_TEST:
        return "TEST";
    case NODE_ASSERT:
        return "ASSERT";
    case NODE_EXPECT:
        return "EXPECT";
    case NODE_DESTRUCT_VAR:
        return "DESTRUCT_VAR";
    case NODE_LAMBDA:
        return "LAMBDA";
    case NODE_PLUGIN:
        return "PLUGIN";
    case NODE_TERNARY:
        return "TERNARY";
    case NODE_TYPEOF:
        return "TYPEOF";
    case NODE_TRY:
        return "TRY";
    case NODE_REFLECTION:
        return "REFLECTION";
    case NODE_REPL_PRINT:
        return "REPL_PRINT";
    case NODE_CUDA_LAUNCH:
        return "CUDA_LAUNCH";
    case NODE_VA_START:
        return "VA_START";
    case NODE_VA_END:
        return "VA_END";
    case NODE_VA_COPY:
        return "VA_COPY";
    case NODE_VA_ARG:
        return "VA_ARG";
    case NODE_LINK:
        return "LINK";
    case NODE_ASM:
        return "ASM";
    case NODE_PREPROC_DIRECTIVE:
        return "PREPROC_DIRECTIVE";
    case NODE_COMPTIME:
        return "COMPTIME";
    case NODE_AST_COMMENT:
        return "AST_COMMENT";
    case NODE_ERRONEOUS:
        return "ERRONEOUS";
    default:
        return "UNKNOWN";
    }
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
    // Handle both primitive char* and legacy struct char*. `c_char` is C's char too.
    if (TYPE_POINTER == t->kind && t->inner &&
        (TYPE_CHAR == t->inner->kind || TYPE_C_CHAR == t->inner->kind))
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

// The integer facts of `t` (integer_kind): through an alias that is not opaque, and for an
// integer named as a struct (`int32_t`, `size_t`, `u8`), by its name. NULL for any other type.
static const IntegerKind *integer_facts(Type *t)
{
    while (t && t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        t = t->inner;
    }
    if (!t)
    {
        return NULL;
    }
    if (t->kind == TYPE_STRUCT && t->name)
    {
        return integer_kind(find_primitive_kind(t->name));
    }
    return integer_kind(t->kind);
}

int is_integer_type(Type *t)
{
    return integer_facts(t) != NULL;
}

int is_unsigned_type(Type *t)
{
    const IntegerKind *facts = integer_facts(t);
    return facts && facts->sign == INTEGER_UNSIGNED;
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

Type *type_seen_from(Type *t, const char *file)
{
    while (t && t->kind == TYPE_ALIAS && t->inner)
    {
        if (t->alias.is_opaque_alias && (!file || !t->alias.alias_defined_in_file ||
                                         strcmp(t->alias.alias_defined_in_file, file) != 0))
        {
            break;
        }
        t = t->inner;
    }
    return t;
}

int integer_type_width(Type *t)
{
    const IntegerKind *facts = integer_facts(t);
    return facts ? facts->width : 0;
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

// An integer literal written without a suffix (`15`) whose value fits in an i32, its default
// type: its type adapts to the other operand. A larger one (`3000000000`) is an i64 and does not.
static int is_plain_int_literal(ASTNode *n)
{
    return n && n->kind == NODE_EXPR_LITERAL && n->literal.kind == LITERAL_INT &&
           n->literal.is_untyped && n->literal.int_val <= (unsigned long long)INT32_MAX;
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
    const IntegerKind *facts = integer_facts(t);
    return facts && facts->sign == INTEGER_SIGNED;
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

    int res = (t->kind == TYPE_F32 || t->kind == TYPE_F64);
    return res;
}

int float_significand_bits(Type *t)
{
    return resolve_alias(t)->kind == TYPE_F32 ? 24 : 53;
}

int float_holds_integer_type(Type *float_type, Type *integer_type)
{
    int width = integer_type_width(resolve_alias(integer_type));
    if (width <= 0)
    {
        return 1;
    }
    int magnitude = is_unsigned_type(integer_type) ? width : width - 1;
    return magnitude <= float_significand_bits(float_type);
}

int float_holds_integer(Type *float_type, long long value)
{
    long long limit = 1LL << float_significand_bits(float_type);
    return value >= -limit && value <= limit;
}

int is_incomplete_type(struct ParserContext *ctx, Type *t)
{
    if (!t || t->kind != TYPE_STRUCT || !t->name)
    {
        return 0;
    }
    ASTNode *def = named_type_def(ctx, t);
    if (!def)
    {
        return 1;
    }
    return def->strct.is_incomplete;
}

int is_discard(ASTNode *node)
{
    return node && node->kind == NODE_VAR_DECL && node->var_decl.name &&
           strcmp(node->var_decl.name, "_") == 0;
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

// How a type is spelled.
typedef enum
{
    SPELLING_READABLE, // type_to_string: primitives by their Zen C name, instances by C name.
    SPELLING_C,        // type_mangle_spelling: as the C names built from the type spell it.
    SPELLING_ZEN_C,    // type_to_zen_c_string_with: as the program writes it, for people.
} TypeSpelling;

static char *type_spelling(Type *t, TypeSpelling spelling, WrittenNameLookup lookup, void *context);

// The name of the C struct a tuple is: `Tuple__` and its elements' names, as `(int, bool*)`
// gives `Tuple__int__boolPtr`.
static char *tuple_struct_name(Type *t)
{
    size_t length = 0;
    char **elements = xmalloc(sizeof(char *) * (size_t)(t->count > 0 ? t->count : 1));
    for (int i = 0; i < t->count; i++)
    {
        elements[i] = type_spelling(t->args[i], SPELLING_C, NULL, NULL);
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

static char *type_spelling_impl(Type *t, TypeSpelling spelling, WrittenNameLookup lookup,
                                void *context);

// The types of a list (a generic's arguments, a tuple's elements) as `a, b`.
static char *type_spelling_list(Type **types, int count, TypeSpelling spelling,
                                WrittenNameLookup lookup, void *context)
{
    char **parts = xmalloc(sizeof(char *) * (size_t)(count > 0 ? count : 1));
    size_t size = 1;
    for (int i = 0; i < count; i++)
    {
        parts[i] = type_spelling(types[i], spelling, lookup, context);
        size += strlen(parts[i]) + 2;
    }
    char *out = xmalloc(size);
    out[0] = 0;
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            strcat(out, ", ");
        }
        strcat(out, parts[i]);
    }
    return out;
}

static char *type_spelling(Type *t, TypeSpelling spelling, WrittenNameLookup lookup, void *context)
{
    if (!t)
    {
        return xstrdup("void");
    }
    char *res = type_spelling_impl(t, spelling, lookup, context);
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
    return type_spelling(t, SPELLING_READABLE, NULL, NULL);
}

char *type_mangle_spelling(Type *t)
{
    return type_spelling(t, SPELLING_C, NULL, NULL);
}

char *type_to_zen_c_string_with(Type *t, WrittenNameLookup lookup, void *context)
{
    return type_spelling(t, SPELLING_ZEN_C, lookup, context);
}

static char *type_spelling_impl(Type *t, TypeSpelling spelling, WrittenNameLookup lookup,
                                void *context)
{
    if (!t)
    {
        return xstrdup("void");
    }

    // People read a fixed-width primitive by the name the program writes (`i32`); a C name
    // spells it as C does (`Vec__int32_t`).
    const char *zen_name = spelling == SPELLING_C ? NULL : primitive_zen_name(t->kind);
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
        char *inner = type_spelling(t->inner, spelling, lookup, context);
        char *res = xmalloc(strlen(inner) + 20);
        sprintf(res, "%sx%d", inner, t->array_size); /* safe */
        zfree(inner);
        return res;
    }

    case TYPE_POINTER:
    {
        char *inner = type_spelling(t->inner, spelling, lookup, context);
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

        char *inner = type_spelling(base, spelling, lookup, context);
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
            char *ret = type_spelling(t->inner, spelling, lookup, context);
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
                char *arg = type_spelling(t->args[i], spelling, lookup, context);
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
        char *ret = type_spelling(t->inner, spelling, lookup, context);
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
            char *arg = type_spelling(t->args[i], spelling, lookup, context);
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
        // An instance of a generic is named after its template and its arguments: as the
        // program writes it (`Vec<Vec<f64>>`), or by the C name of the instance.
        if (t->count > 0 && t->args && t->name)
        {
            if (spelling == SPELLING_ZEN_C)
            {
                char *args = type_spelling_list(t->args, t->count, spelling, lookup, context);
                char *res = xmalloc(strlen(t->name) + strlen(args) + 3);
                sprintf(res, "%s<%s>", t->name, args); /* safe */
                return res;
            }
            char **args = xmalloc((size_t)t->count * sizeof(char *));
            for (int i = 0; i < t->count; i++)
            {
                args[i] = type_spelling(t->args[i], SPELLING_C, NULL, NULL);
            }
            return mangle_generic_name(t->name, args, t->count);
        }
        // A struct or enum of a module, as its importer writes it (`math::Vector`).
        const char *written =
            (spelling == SPELLING_ZEN_C && lookup && t->name) ? lookup(context, t->name) : NULL;
        return xstrdup(written ? written : t->name);
    }
    case TYPE_TUPLE:
        if (spelling == SPELLING_ZEN_C)
        {
            char *elements = type_spelling_list(t->args, t->count, spelling, lookup, context);
            char *res = xmalloc(strlen(elements) + 3);
            sprintf(res, "(%s)", elements); /* safe */
            return res;
        }
        return tuple_struct_name(t);
    case TYPE_ALIAS:
        // The name the program uses: a @link_name only shows in C (type_to_c_string).
        return xstrdup(t->name);

    default:
        return xstrdup("unknown");
    }
}

// C writes a type around what it declares: `int32_t x`, `int32_t* p`, `int32_t a[3]`,
// `int32_t (*f)(int32_t)`. type_to_c_declaration_with builds that text from the Type, from the
// outside in: a pointer puts `*` before the declarator (in parentheses when it points to an
// array), an array puts `[N]` after it, a function pointer wraps it in `(*...)(parameters)`, and
// what is left is the type's name before it. Any nesting comes out right (a pointer to an array of
// function pointers, a function pointer that returns one), and the type alone is the same with an
// empty declarator, for casts and `sizeof`.
static char *c_declaration(Type *t, const char *declarator, int misra_mode, CNameLookup lookup,
                           void *context);

char *type_to_c_string(Type *t, int misra_mode)
{
    return type_to_c_string_with(t, misra_mode, NULL, NULL);
}

char *type_to_c_string_with(Type *t, int misra_mode, CNameLookup lookup, void *context)
{
    return type_to_c_declaration_with(t, "", misra_mode, lookup, context);
}

// Whether `t` is a function pointer, through the aliases that name one.
static int is_function_pointer(Type *t)
{
    while (t && !t->c_typedef_name && t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        t = t->inner;
    }
    return t && !t->c_typedef_name && t->kind == TYPE_FUNCTION && t->is_raw;
}

char *type_to_c_declaration_with(Type *t, const char *declarator, int misra_mode,
                                 CNameLookup lookup, void *context)
{
    if (t && t->is_const && is_function_pointer(t))
    {
        // A function cannot be const: a const function pointer is the pointer,
        // `int32_t (*const f)(int32_t)`.
        size_t size = strlen(declarator) + 8;
        char *const_declarator = xmalloc(size);
        snprintf(const_declarator, size, "const%s%s", declarator[0] ? " " : "", declarator);
        char *res = c_declaration(t, const_declarator, misra_mode, lookup, context);
        zfree(const_declarator);
        return res;
    }
    char *res = c_declaration(t, declarator, misra_mode, lookup, context);
    if (t && t->is_const)
    {
        char *final = xmalloc(strlen(res) + 7);
        sprintf(final, "const %s", res); /* safe */
        zfree(res);
        return final;
    }
    return res;
}

// The C name a struct or enum declares for itself with @link_name, or NULL.
static const char *link_name_of_type(Type *t, CNameLookup lookup, void *context)
{
    const char *name = type_def_name(t);
    return (lookup && name) ? lookup(context, name) : NULL;
}

// The C name of a struct, enum or alias: its @link_name or the name that keeps it from being a
// keyword (definition_link_name), else the name its definition has.
static const char *c_name_of(Type *t, CNameLookup lookup, void *context)
{
    const char *link_name = link_name_of_type(t, lookup, context);
    return link_name ? link_name : type_def_name(t);
}

// `name` declaring `declarator`. A pointer's stars stay with the name (`int32_t* p`, `int32_t*`),
// anything else is written after a space (`int32_t x`, `int32_t (*f)(int32_t)`), and brackets
// follow the type alone directly (`int32_t[3]`).
static char *join_declaration(const char *name, const char *declarator)
{
    size_t stars = strspn(declarator, "*");
    const char *rest = declarator + stars;
    int space = rest[0] && rest[0] != '[' && rest[0] != ' ';
    size_t size = strlen(name) + strlen(declarator) + 2;
    char *res = xmalloc(size);
    snprintf(res, size, "%s%.*s%s%s", name, (int)stars, declarator, space ? " " : "", rest);
    return res;
}

// Whether the C declarator of `t` follows it (`[N]`): a pointer to it needs parentheses,
// `int32_t (*p)[3]`, since `int32_t *p[3]` is an array of pointers.
static int declarator_follows(Type *t)
{
    while (t && !t->c_typedef_name && t->kind == TYPE_ALIAS && !t->alias.is_opaque_alias)
    {
        t = t->inner;
    }
    return t && !t->c_typedef_name && t->kind == TYPE_ARRAY;
}

// The parameters of a function pointer type: `(int32_t, char*)`, `(void)`-less `()` when it has
// none.
static char *c_parameters(Type *t, int misra_mode, CNameLookup lookup, void *context)
{
    size_t size = 3;
    char **args = xmalloc(sizeof(char *) * (size_t)(t->count + 1));
    for (int i = 0; i < t->count; i++)
    {
        args[i] = type_to_c_string_with(t->args[i], misra_mode, lookup, context);
        size += strlen(args[i]) + 2;
    }
    size += 5; // ", ..."
    char *res = xmalloc(size);
    char *p = res;
    *p++ = '(';
    for (int i = 0; i < t->count; i++)
    {
        p += sprintf(p, "%s%s", i > 0 ? ", " : "", args[i]); /* safe */
        zfree(args[i]);
    }
    if (t->is_varargs)
    {
        p += sprintf(p, "%s...", t->count > 0 ? ", " : ""); /* safe */
    }
    strcpy(p, ")");
    zfree(args);
    return res;
}

// The name C knows a type by, for a type that does not wrap a declarator.
static char *c_type_name(Type *t, int misra_mode, CNameLookup lookup, void *context)
{
    // A primitive, by the one table of C names.
    const char *primitive = primitive_c_name(t->kind, misra_mode);
    if (primitive)
    {
        return xstrdup(primitive);
    }

    switch (t->kind)
    {
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

    case TYPE_FUNCTION: // A closure: a function pointer wraps a declarator (c_declaration).
        return xstrdup("z_closure_T");

    case TYPE_GENERIC:
        // Use type_to_string to get the mangled name (e.g. Option_int) instead of raw C string
        // composition This ensures consistency with struct definitions.
        return type_to_string(t);

    case TYPE_ALIAS: // An opaque one: a plain alias is the type it names (c_declaration).
        return xstrdup(c_name_of(t, lookup, context));

    case TYPE_ENUM:
        return xstrdup(c_name_of(t, lookup, context));

    case TYPE_UNSAFE_ANY:
        return xstrdup("any");
    default:
        return xstrdup("unknown");
    }
}

static char *c_declaration(Type *t, const char *declarator, int misra_mode, CNameLookup lookup,
                           void *context)
{
    if (!t)
    {
        return join_declaration("void", declarator);
    }
    char *outer = NULL; // The declarator this type wraps around `declarator`.
    Type *inner = NULL;
    if (!t->c_typedef_name)
    {
        switch (t->kind)
        {
        case TYPE_POINTER:
        {
            size_t size = strlen(declarator) + 16;
            outer = xmalloc(size);
            if (t->is_restrict)
            {
                snprintf(outer, size, "* __restrict%s%s", declarator[0] ? " " : "", declarator);
            }
            else
            {
                snprintf(outer, size, declarator_follows(t->inner) ? "(*%s)" : "*%s", declarator);
            }
            inner = t->inner;
            break;
        }
        case TYPE_ARRAY:
        {
            size_t size = strlen(declarator) + 24;
            outer = xmalloc(size);
            snprintf(outer, size, "%s[%d]", declarator, t->array_size);
            inner = t->inner;
            break;
        }
        case TYPE_FUNCTION:
            if (t->is_raw)
            {
                char *params = c_parameters(t, misra_mode, lookup, context);
                size_t size = strlen(declarator) + strlen(params) + 4;
                outer = xmalloc(size);
                snprintf(outer, size, "(*%s)%s", declarator, params);
                zfree(params);
                inner = t->inner;
            }
            break;
        case TYPE_ALIAS:
            if (!t->alias.is_opaque_alias)
            {
                return type_to_c_declaration_with(t->inner, declarator, misra_mode, lookup,
                                                  context);
            }
            break;
        default:
            break;
        }
    }
    if (outer)
    {
        char *res = type_to_c_declaration_with(inner, outer, misra_mode, lookup, context);
        zfree(outer);
        return res;
    }
    char *name = t->c_typedef_name ? xstrdup(t->c_typedef_name)
                                   : c_type_name(t, misra_mode, lookup, context);
    char *res = join_declaration(name, declarator);
    zfree(name);
    return res;
}

char *function_c_name(ASTNode *fn)
{
    if (!fn || !fn->func.name)
    {
        return NULL;
    }
    char *base = method_c_name(fn);
    if (!base)
    {
        base = fn->func.is_module_prefixed
                   ? mangle_module_symbol(fn->func.module_prefix, fn->func.name)
                   : fn->func.name;
    }
    if (fn->func.instance_arg_count == 0)
    {
        return base;
    }
    char **args = xmalloc(sizeof(char *) * (size_t)fn->func.instance_arg_count);
    for (int i = 0; i < fn->func.instance_arg_count; i++)
    {
        args[i] = type_mangle_spelling(fn->func.instance_args[i]);
    }
    return mangle_generic_name(base, args, fn->func.instance_arg_count);
}

char *method_c_name(ASTNode *fn)
{
    if (!fn || !fn->func.is_method || !fn->func.impl_target)
    {
        return NULL;
    }
    return mangle_method_symbol(type_impl_name(fn->func.impl_target), fn->func.trait_name,
                                fn->func.name);
}

char *declared_type_c_name(ASTNode *decl)
{
    if (!decl || (decl->kind != NODE_STRUCT && decl->kind != NODE_ENUM))
    {
        return NULL;
    }
    char *name = decl->kind == NODE_STRUCT ? decl->strct.name : decl->enm.name;
    int prefixed =
        decl->kind == NODE_STRUCT ? decl->strct.is_module_prefixed : decl->enm.is_module_prefixed;
    if (!name || !prefixed)
    {
        return name;
    }
    const char *prefix =
        decl->kind == NODE_STRUCT ? decl->strct.module_prefix : decl->enm.module_prefix;
    return mangle_module_symbol(prefix, name);
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
        deref->unary.op = OP_DEREF;
        deref->unary.operand = expr;
        deref->type_info = t->inner;
        deref->token = expr->token;
        expr = deref;
        t = pointee;
        pointee = get_inner_type(t->inner);
    }
    return expr;
}

// The character a one-letter escape (`\n`, `\t`...) stands for, or -1.
static int simple_escape(char c)
{
    switch (c)
    {
    case 'n':
        return '\n';
    case 't':
        return '\t';
    case 'r':
        return '\r';
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'v':
        return '\v';
    case 'e':
        return 27;
    default:
        return -1;
    }
}

static int is_hex_digit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// A string literal's string_val holds its text as written in C (escapes
// included), ready for codegen. At compile time we need the characters
// themselves: `code("];\n")` must yield a newline, not `\` and `n`.
static char *unescape_c_string(const char *s)
{
    char *out = xmalloc(strlen(s) + 1);
    char *p = out;
    while (*s)
    {
        if (*s != '\\' || !s[1])
        {
            *p++ = *s++;
            continue;
        }
        s++;
        char c = *s++;
        int simple = simple_escape(c);
        if (simple >= 0)
        {
            *p++ = (char)simple;
        }
        else if (c == 'x')
        {
            unsigned int val = 0;
            for (int digits = 0; digits < 2 && is_hex_digit(*s); digits++, s++)
            {
                val = (val << 4) | (unsigned int)(*s <= '9' ? *s - '0' : (*s | 0x20) - 'a' + 10);
            }
            *p++ = (char)val;
        }
        else if (c >= '0' && c <= '7')
        {
            unsigned int val = (unsigned int)(c - '0');
            for (int k = 0; k < 2 && *s >= '0' && *s <= '7'; k++)
            {
                val = (val << 3) | (unsigned int)(*s++ - '0');
            }
            *p++ = (char)val;
        }
        else
        {
            // \\, \", \' and anything unknown: the character itself.
            *p++ = c;
        }
    }
    *p = '\0';
    return out;
}

int is_string_literal(ASTNode *node)
{
    return node && node->kind == NODE_EXPR_LITERAL &&
           (node->literal.kind == LITERAL_STRING || node->literal.kind == LITERAL_RAW_STRING);
}

char *string_literal_text(ASTNode *literal)
{
    const char *text = literal->literal.string_val ? literal->literal.string_val : "";
    if (literal->literal.kind == LITERAL_RAW_STRING)
    {
        return xstrdup(text);
    }
    return unescape_c_string(text);
}

int is_pointer_type(Type *t)
{
    t = resolve_alias(t);
    return t && (t->kind == TYPE_POINTER || t->kind == TYPE_STRING);
}

Type *pointee_of(Type *t)
{
    t = resolve_alias(t);
    if (!t)
    {
        return NULL;
    }
    if (t->kind == TYPE_POINTER)
    {
        return t->inner;
    }
    return t->kind == TYPE_STRING ? type_new(TYPE_CHAR) : NULL;
}

ASTNode *ast_turn_into(ASTNode *node, NodeType kind)
{
    ASTNode *was = ast_create(node->kind);
    *was = *node;
    was->next = NULL;

    node->kind = kind;
    memset(&node->root, 0, sizeof(ASTNode) - offsetof(ASTNode, root)); // The union, last.
    return was;
}

ASTNode *written_expr(ASTNode *node)
{
    while (node && node->kind == NODE_IMPLICIT_CONVERSION)
    {
        node = node->conversion.expr;
    }
    return node;
}
