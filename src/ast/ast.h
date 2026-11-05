// SPDX-License-Identifier: MIT

/**
 * @file ast.h
 * @brief AST node types, Type system, type introspection helpers.
 */

#ifndef ZC_ALLOW_INTERNAL
#error "ast/ast.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef AST_H
#define AST_H

#include "token.h"
#include <stdlib.h>

typedef struct ParserContext ParserContext;

// Forward declarations.
struct ASTNode;
typedef struct ASTNode ASTNode;

// ** Formal Type System **
// Used for Generics, Type Inference, and robust pointer handling.

/**
 * @brief Kind of literal value.
 */
typedef enum
{
    LITERAL_INT = 0,       ///< Integer literal.
    LITERAL_FLOAT = 1,     ///< Floating point literal.
    LITERAL_STRING = 2,    ///< String literal.
    LITERAL_CHAR = 3,      ///< Character literal.
    LITERAL_RAW_STRING = 4 ///< Raw string literal.
} LiteralKind;

/**
 * @brief Formal type system kinds.
 */
typedef enum
{
    TYPE_VOID,   ///< `void` type.
    TYPE_BOOL,   ///< `bool` type.
    TYPE_CHAR,   ///< `char` type.
    TYPE_STRING, ///< `string` type.
    TYPE_U0,     ///< `u0` type.
    TYPE_I8,     ///< `i8` type.
    TYPE_U8,     ///< `u8` type.
    TYPE_I16,    ///< `i16` type.
    TYPE_U16,    ///< `u16` type.
    TYPE_I32,    ///< `i32` type.
    TYPE_U32,    ///< `u32` type.
    TYPE_I64,    ///< `i64` type.
    TYPE_U64,    ///< `u64` type.
    TYPE_I128,   ///< `i128` type.
    TYPE_U128,   ///< `u128` type.
    TYPE_F32,    ///< `f32` type.
    TYPE_F64,    ///< `f64` type.
    TYPE_INT,    ///< `int` (alias, usually i32).
    TYPE_FLOAT,  ///< `float` (alias).
    TYPE_USIZE,  ///< `usize` (pointer size unsigned).
    TYPE_ISIZE,  ///< `isize` (pointer size signed).
    TYPE_BYTE,   ///< `byte`.
    TYPE_RUNE,   ///< `rune`.
    TYPE_UINT,   ///< `uint` (alias).
    // Portable C Types (FFI)
    TYPE_C_INT,       ///< `c_int` (int).
    TYPE_C_UINT,      ///< `c_uint` (unsigned int).
    TYPE_C_LONG,      ///< `c_long` (long).
    TYPE_C_ULONG,     ///< `c_ulong` (unsigned long).
    TYPE_C_LONGLONG,  ///< `c_longlong` (long long).
    TYPE_C_ULONGLONG, ///< `c_ulonglong` (unsigned long long).
    TYPE_C_SHORT,     ///< `c_short` (short).
    TYPE_C_USHORT,    ///< `c_ushort` (unsigned short).
    TYPE_C_CHAR,      ///< `c_char` (char).
    TYPE_C_UCHAR,     ///< `c_uchar` (unsigned char).

    TYPE_STRUCT,     ///< Struct type.
    TYPE_ENUM,       ///< Enum type.
    TYPE_POINTER,    ///< Pointer type (*).
    TYPE_ARRAY,      ///< Fixed size array [N].
    TYPE_VECTOR,     ///< SIMD vector type.
    TYPE_FUNCTION,   ///< Function pointer or reference.
    TYPE_GENERIC,    ///< Generic type parameter (T).
    TYPE_ALIAS,      ///< Opaque type alias.
    TYPE_BITINT,     ///< C23 _BitInt(N).
    TYPE_UBITINT,    ///< C23 unsigned _BitInt(N).
    TYPE_UNSAFE_ANY, ///< Internal unsafe ANY type for match bindings.
    TYPE_UNKNOWN     ///< Unknown/unresolved type.
} TypeKind;

/**
 * @brief Represents a formal type in the type system.
 */
typedef struct Type
{
    TypeKind kind;          ///< The kind of type.
    int lifetime_depth;     ///< Nesting depth of this type's lifetime (0 = static, >0 = local).
    char *name;             ///< Name of the type (for STRUCT, GENERIC, ENUM).
    struct Type *inner;     ///< Inner type (for POINTER, ARRAY).
    struct Type **args;     ///< Generic arguments (for GENERIC instantiations).
    int count;              ///< Count of generic arguments.
    int is_const;           ///< 1 if const-qualified.
    int is_explicit_struct; ///< 1 if defined with "struct" keyword explicitly.
    int is_union;           ///< With is_explicit_struct: the keyword is "union" (a C union tag).
    int is_raw;             // Raw function pointer (fn*)
    int array_size;         ///< Size for fixed-size arrays. For TYPE_BITINT, this is the bit width.
    struct
    {
        int has_drop;     ///< 1 if type implements Drop trait (RAII).
        int has_iterable; ///< 1 if type implements Iterable trait.
    } traits;
    union
    {
        int is_varargs;  ///< 1 if function type is variadic.
        int is_restrict; ///< 1 if pointer is restrict-qualified.
        struct
        {
            int is_opaque_alias;
            char *alias_defined_in_file;
        } alias;
    };
} Type;

// ** AST Node Types **
/**
 * @brief AST Node Types.
 */
typedef enum
{
    NODE_ROOT,               ///< Root of the AST.
    NODE_FUNCTION,           ///< Function definition.
    NODE_BLOCK,              ///< Code block { ... }.
    NODE_RETURN,             ///< Return statement.
    NODE_VAR_DECL,           ///< Variable declaration.
    NODE_CONST,              ///< Constant definition.
    NODE_TYPE_ALIAS,         ///< Type alias (typedef).
    NODE_IF,                 ///< If statement.
    NODE_WHILE,              ///< While loop.
    NODE_FOR,                ///< For loop.
    NODE_FOR_RANGE,          ///< For-range loop (iterator).
    NODE_LOOP,               ///< Infinite loop.
    NODE_BREAK,              ///< Break statement.
    NODE_CONTINUE,           ///< Continue statement.
    NODE_MATCH,              ///< Match statement.
    NODE_MATCH_CASE,         ///< Case within match.
    NODE_EXPR_BINARY,        ///< Binary expression (a + b).
    NODE_EXPR_UNARY,         ///< Unary expression (!a).
    NODE_EXPR_LITERAL,       ///< Literal value.
    NODE_EXPR_VAR,           ///< Variable reference.
    NODE_EXPR_CALL,          ///< Function call.
    NODE_EXPR_MEMBER,        ///< Member access (a.b).
    NODE_EXPR_INDEX,         ///< Array index (a[b]).
    NODE_EXPR_CAST,          ///< Type cast.
    NODE_EXPR_SIZEOF,        ///< Sizeof expression.
    NODE_EXPR_STRUCT_INIT,   ///< Struct initializer.
    NODE_FIELD_INIT,         ///< Field of a struct initializer (var_decl: name, init_expr).
    NODE_EXPR_ARRAY_LITERAL, ///< Array literal.
    NODE_EXPR_TUPLE_LITERAL, ///< Tuple literal.
    NODE_EXPR_SLICE,         ///< Slice operation.
    NODE_STRUCT,             ///< Struct definition.
    NODE_FIELD,              ///< Struct field.
    NODE_ENUM,               ///< Enum definition.
    NODE_ENUM_VARIANT,       ///< Enum variant.
    NODE_TRAIT,              ///< Trait definition.
    NODE_IMPL,               ///< Impl block.
    NODE_IMPL_TRAIT,         ///< Trait implementation.
    NODE_INCLUDE,            ///< Include directive.
    NODE_LINK,               ///< Link directive (@link("path")).
    NODE_RAW_STMT,           ///< Raw statement (transpiler bypass).
    NODE_TEST,               ///< Test block.
    NODE_ASSERT,             ///< Assert statement.
    NODE_EXPECT,             ///< Expect statement (non-fatal assert).
    NODE_DEFER,              ///< Defer statement.
    NODE_DESTRUCT_VAR,       ///< Destructuring declaration.
    NODE_TERNARY,            ///< Ternary expression (?:).
    NODE_ASM,                ///< Assembly block.
    NODE_LAMBDA,             ///< Lambda function.
    NODE_PLUGIN,             ///< Plugin invocation.
    NODE_GOTO,               ///< Goto statement.
    NODE_LABEL,              ///< Label.
    NODE_TYPEOF,             ///< Typeof operator.
    NODE_TRY,                ///< Try statement (error handling).
    NODE_REFLECTION,         ///< Reflection info.
    NODE_REPL_PRINT,         ///< Implicit print (REPL).
    NODE_CUDA_LAUNCH,        ///< CUDA kernel launch (<<<...>>>).
    NODE_VA_START,           ///< va_start intrinsic.
    NODE_VA_END,             ///< va_end intrinsic.
    NODE_VA_COPY,            ///< va_copy intrinsic.
    NODE_VA_ARG,             ///< va_arg intrinsic.
    NODE_PREPROC_DIRECTIVE,  ///< C-style preprocessor directive (#define, etc).
    NODE_IMPORT,             ///< Import statement (recursive).
    NODE_COMPTIME,           ///< Comptime block (interpreted at compile time).
    NODE_AST_COMMENT,        ///< Comment node.
    NODE_EXPR_FSTRING,       ///< Interpolated string f"..." (a list of NODE_FSTRING_PART).
    NODE_FSTRING_PART,       ///< Piece of an f-string: literal text or a `{expr[:fmt]}`.
    NODE_ERRONEOUS           ///< Error sentinel (safe no-op node).
} NodeType;

// ** AST Node Structure **
typedef struct Attribute
{
    char *name;
    char **args;
    int count;
    struct Attribute *next;
} Attribute;

/**
 * @brief A name as the source writes it, optionally qualified: `Variant`,
 *        `Enum::Variant`, `Type::method`. Codegen builds its C name.
 */
typedef struct QualifiedName
{
    char *scope; ///< `Enum` or `Type`; NULL if the name is not qualified.
    char *name;  ///< `Variant` or `method`.
} QualifiedName;

/** The kind of one alternative of a match case. */
typedef enum
{
    PATTERN_WILDCARD, ///< `_`.
    PATTERN_NAME,     ///< A name: a variant (`Variant`, `Enum::Variant`) or a constant (`MAX`).
    PATTERN_LITERAL,  ///< A number, char or string literal, as written.
    PATTERN_RANGE     ///< `start..end` or `start..=end`.
} PatternKind;

/** One alternative of a match case: `A || B` is a list of two. */
typedef struct MatchPattern
{
    PatternKind kind;
    QualifiedName name; ///< PATTERN_NAME: the name, as written.
    char *literal;      ///< PATTERN_LITERAL: the literal, as written.
    char *range_start;  ///< PATTERN_RANGE: the start, as written.
    char *range_end;    ///< PATTERN_RANGE: the end, as written.
    int is_inclusive;   ///< PATTERN_RANGE: `..=` rather than `..`.
    struct MatchPattern *next;
} MatchPattern;

struct ASTNode
{
    NodeType kind;
    ASTNode *next;
    int line;          // Source line number for debugging.
    char *doc_comment; // Attached documentation comment.

    // Type information.
    Type *type_info; // Formal type object (for inference/generics).
    Token token;
    Token definition_token; // For LSP: Location where the symbol used in this
                            // node was defined.
    char *cfg_condition;    // C preprocessor condition from @cfg
    char *link_name;        // Optional override for external C name
    int is_checked;         // Flag to track semantic analysis (avoid redundant passes).

    union
    {
        struct
        {
            ASTNode *children;
        } root;

        struct
        {
            char *name;
            char *generic_params; // <T, U>
            ASTNode *body;
            Type **arg_types;
            ASTNode **default_values; // Default value of each parameter, NULL where it has none.
            char **param_names;       // Explicit parameter names.
            int count;
            Type *ret_type_info;
            int is_varargs;
            int is_inline;
            int required; // @required: warn if return value is discarded.
            // GCC attributes
            int noinline;    // @noinline
            int constructor; // @constructor
            int destructor;  // @destructor
            int unused;      // @unused
            int weak;        // @weak
            int is_export;   // @export (visibility default).
            int cold;        // @cold
            int hot;         // @hot
            int noreturn;    // @noreturn
            int pure;        // @pure
            char *section;   // @section("name")
            int is_comptime; // @comptime function
            int is_extern;   // extern fn (C linkage in C++ mode)
            // CUDA qualifiers
            int cuda_global; // @global -> __global__
            int cuda_device; // @device -> __device__
            int cuda_host;   // @host -> __host__

            char **c_type_overrides; // @ctype("...") per parameter
            int elide_from_idx;      // Index of parameter for lifetime elision (-1 if none)

            Attribute *attributes; // Custom attributes

            // Declaration context (to parse a deferred body)
            char *module_prefix;  // C prefix of the declaring module (NULL if none)
            char *impl_struct;    // Type of the enclosing impl block (NULL for free function)
            const char *filename; // Source file that declares the function
        } func;

        struct
        {
            ASTNode *statements;
        } block;

        struct
        {
            ASTNode *value;
        } ret;

        struct
        {
            char *name;
            ASTNode *init_expr;
            Type *type_info;
            int is_static;
            int is_thread_local;
            int is_export;
        } var_decl;

        struct
        {
            char *name;
            Type *payload;
            int payload_count; ///< Values the variant carries; several make a tuple payload.
            int tag_id;
        } variant;

        struct
        {
            char *name;
            ASTNode *variants;
            int is_template;
            char *generic_param;
            int is_export;
        } enm;

        struct
        {
            char *alias; // The aliased type is the node's type_info.
            int is_opaque;
            char *defined_in_file;
            int is_export;
        } type_alias;

        struct
        {
            ASTNode *condition;
            ASTNode *then_body;
            ASTNode *else_body;
        } if_stmt;

        struct
        {
            ASTNode *condition;
            ASTNode *body;
            char *loop_label;
        } while_stmt;

        struct
        {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *step;
            ASTNode *body;
            char *loop_label;
        } for_stmt;

        struct
        {
            char *var_name;
            ASTNode *start;
            ASTNode *end;
            char *step;
            int is_inclusive;
            ASTNode *body;
            char *loop_label;
        } for_range;

        struct
        {
            ASTNode *body;
            char *loop_label;
        } loop_stmt;

        struct
        {
            ASTNode *expr;
            ASTNode *cases;
        } match_stmt;

        struct
        {
            MatchPattern *patterns; ///< The alternatives of the case, as written.
            char **binding_names;   // Multiple bindings
            int binding_count;      // Count
            int *binding_refs;      // Ref flags per binding
            int is_destructuring;
            ASTNode *guard;
            ASTNode *body;
            int is_default;
        } match_case;

        struct
        {
            char *op;
            ASTNode *left;
            ASTNode *right;
        } binary;

        struct
        {
            char *op;
            ASTNode *operand;
            int owns_temp; ///< `&_rval` of a temporary that the enclosing call must drop.
            int temp_id;   ///< While emitting that call: 1 + id of the named temporary.
        } unary;

        struct
        {
            LiteralKind kind;
            unsigned long long int_val;
            double float_val;
            char *string_val;
        } literal;

        struct
        {
            char *name;
            char *suggestion;
            struct ZenSymbol *symbol;
            QualifiedName variant; ///< An enum variant: its enum (resolved) and its name.
        } var_ref;

        struct
        {
            ASTNode *callee;
            ASTNode *args;
            char **arg_names;
            int count;
        } call;

        struct
        {
            ASTNode *target;
            char *field;
            int is_pointer_access;
        } member;

        struct
        {
            ASTNode *array;
            ASTNode *index;
            ASTNode *extra_indices; // Linked list of additional indices (for v[i, j, k])
            int index_count;        // Total index count (1 for v[i], 2 for v[i,j], etc.)
        } index;

        struct
        {
            ASTNode *array;
            ASTNode *start;
            ASTNode *end;
        } slice;

        struct
        {
            ASTNode *expr; // The target type is the node's type_info.
        } cast;

        struct
        {
            char *struct_name;
            ASTNode *fields;
        } struct_init;

        struct
        {
            ASTNode *elements;
            int count;
        } array_literal;

        struct
        {
            ASTNode *elements;
            int count;
        } tuple_literal;

        struct
        {
            char *name;
            ASTNode *fields;
            int is_template;
            char **generic_params;   // Array of generic parameter names (for example, ["K", "V"])
            int generic_param_count; // Number of generic parameters
            char *parent;
            int is_union;
            int is_packed;         // @packed attribute.
            int align;             // @align(N) attribute, 0 = default.
            int is_incomplete;     // Forward declaration (prototype)
            int is_export;         // @export attribute
            Attribute *attributes; // Custom attributes
            char **used_structs;   // Names of structs used/mixed-in
            int used_struct_count;
            int is_opaque;
            char *defined_in_file; // File where the struct is defined (for privacy check)
            char *crepr_c_type;    // @crepr("C.kind") - use C type instead of redefining
        } strct;

        struct
        {
            char *name; // The field's type is the node's type_info.
            int bit_width;
        } field;

        struct
        {
            char *name;
            ASTNode *methods;
            char **generic_params;
            int generic_param_count;
        } trait;

        struct
        {
            char *struct_name;
            ASTNode *methods;
        } impl;

        struct
        {
            char *trait_name;
            char *target_type;
            ASTNode *methods;
        } impl_trait;

        struct
        {
            char *path;
            int is_system;
        } include;

        struct
        {
            char *path;
        } link;

        struct
        {
            char *content;
        } raw_stmt;

        struct
        {
            char *name;
            ASTNode *body;

            // Declaration context
            char *module_prefix;  // C prefix of the declaring module
            const char *filename; // Source file that declares the test
        } test_stmt;

        struct
        {
            ASTNode *condition;
            char *message;
            int message_is_literal;   ///< 1 if message was a string literal, 0 if variable
            ASTNode *message_fstring; ///< The message when it interpolates (message is NULL).
        } assert_stmt;

        struct
        {
            ASTNode *stmt;
        } defer_stmt;

        struct
        {
            char *plugin_name;
            char *body;
            int start_line;
            int start_col;
            int end_line;
            int end_col;
        } plugin_stmt;

        struct
        {
            char **names;
            Type **type_infos; // Explicit types (NULL entries if inferred)
            int count;
            ASTNode *init_expr;
            int is_struct_destruct;
            char *struct_name;  // "Point" (or NULL if unchecked/inferred).
            char **field_names; // NULL if same as 'names', otherwise mapped.
            int is_guard;
            char *guard_variant; // "Some", "Ok".
            ASTNode *else_block;
        } destruct;

        struct
        {
            ASTNode *cond;
            ASTNode *true_expr;
            ASTNode *false_expr;
        } ternary;

        struct
        {
            char *code;
            int is_volatile;
            int register_size; // The register size to use of 32/64/128 bits
            char **outputs;
            char **output_modes;
            char **inputs;
            char **clobbers;
            int output_count;
            int input_count;
            int clobber_count;
        } asm_stmt;

        struct
        {
            char *path;
            ASTNode *module_root;
        } import_stmt;

        struct
        {
            char **param_names; // Parameter and return types: type_info->args and ->inner.
            ASTNode *body;
            int count;
            int lambda_id;
            int is_expression;
            int is_bare; // 1 if should be emitted without void* ctx (for fn*)
            char **captured_vars;
            Type **captured_types_info; // NULL where the parser found no type.
            int capture_count;
            int *capture_modes;
            int default_capture_mode;
            char **explicit_captures;
            int *explicit_capture_modes;
            int explicit_capture_count;
        } lambda;

        struct
        {
            ASTNode *expr;
            Type *target_type_info;
            int is_type;
        } size_of;

        struct
        {
            char *label_name;
            ASTNode *goto_expr;
        } goto_stmt;

        struct
        {
            char *label_name;
        } label_stmt;

        struct
        {
            char *target_label;
        } break_stmt;

        struct
        {
            char *target_label;
        } continue_stmt;

        struct
        {
            ASTNode *expr;
        } try_stmt;

        struct
        {
            int kind; // 0=type_name, 1=fields.
            Type *target_type;
        } reflection;

        struct
        {
            ASTNode *expr;
        } repl_print;

        struct
        {
            ASTNode *parts; ///< NODE_FSTRING_PART list, in source order.
        } fstring;

        struct
        {
            char *text;        ///< Literal text, braces unescaped (NULL for a `{expr}`).
            ASTNode *expr;     ///< The interpolated expression (NULL for text).
            char *fmt;         ///< printf conversion written after ':' (`.2f`), or NULL.
            Type *symbol_type; ///< Type of `expr` found by name when it has no type_info.
        } fstring_part;

        struct
        {
            ASTNode *call;       // The kernel call (NODE_EXPR_CALL)
            ASTNode *grid;       // Grid dimensions expression
            ASTNode *block;      // Block dimensions expression
            ASTNode *shared_mem; // Optional shared memory size (NULL = default)
            ASTNode *stream;     // Optional CUDA stream (NULL = default)
        } cuda_launch;

        struct
        {
            ASTNode *ap;
            ASTNode *last_arg;
        } va_start_args;

        struct
        {
            ASTNode *ap;
        } va_end_args;

        struct
        {
            ASTNode *dest;
            ASTNode *src;
        } va_copy_args;

        struct
        {
            ASTNode *ap;
            Type *type_info;
        } va_arg_val;

        struct
        {
            char *content;
        } comment;

        struct
        {
            ASTNode *body;      ///< The comptime block body (parsed statements).
            ASTNode *generated; ///< Generated AST nodes (from yield() output).
            int in_block;       ///< Inside a function body: the output is statements.
        } comptime;
    };
};

ASTNode *ast_create(NodeType type);
void ast_free(ASTNode *node);

/// Most child fields a node kind has (NODE_CUDA_LAUNCH has 5).
#define AST_MAX_CHILDREN 8

/**
 * @brief Where the children of a node are stored.
 *
 * Each slot is the address of a child field of the node; the field holds the
 * head of a list linked by `next`, or NULL. Through a slot a walker can read
 * the child (`*slot`) or replace it (`*slot = other`).
 */
typedef struct
{
    ASTNode **slots[AST_MAX_CHILDREN];
    int count;
} ASTChildren;

/**
 * @brief The child fields of a node, in source order.
 *
 * Not included: the tree of an imported module (`import_stmt.module_root`,
 * shared between importers; walk it yourself with a visited set), the array
 * `func.default_values`, and the pieces of an f-string (`fstring.parts`): their
 * expressions carry the position of the string, not their own.
 * For NODE_COMPTIME, both `body` (the code written) and `generated` (the code
 * it produced).
 */
ASTChildren ast_children(ASTNode *node);

Type *type_new(TypeKind kind);
Type *type_new_ptr(Type *inner);
Type *type_new_array(Type *inner, int size);
Type *type_clone(Type *t);
int type_eq(Type *a, Type *b);
int is_integer_type(Type *t);
int is_incomplete_type(struct ParserContext *ctx, Type *t);
int is_unsigned_type(Type *t);
int is_signed_type(Type *t);
int is_boolean_type(Type *t);
int is_float_type(Type *t);
int is_composite_expression(ASTNode *node);
char *type_to_string(Type *t);
char *type_to_c_string(Type *t, int misra_mode);
/**
 * @brief The C name of the struct or enum `name` (its @link_name), or NULL to write `name`.
 */
typedef const char *(*CNameLookup)(void *context, const char *name);
/**
 * @brief type_to_c_string, asking `lookup` for the C name of each struct and enum.
 */
char *type_to_c_string_with(Type *t, int misra_mode, CNameLookup lookup, void *context);
char *infer_type(struct ParserContext *ctx, struct ASTNode *node);
char *get_field_type_str(struct ParserContext *ctx, const char *struct_name,
                         const char *field_name);
Type *get_inner_type(Type *t);

typedef struct TraitReg
{
    char *name;
    struct TraitReg *next;
} TraitReg;

#endif
