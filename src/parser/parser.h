// SPDX-License-Identifier: MIT

#ifndef ZC_ALLOW_INTERNAL
#error "parser/parser.h is internal to Zen C. Include the appropriate public header instead."
#endif

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "compiler.h"
#include "../diagnostics/diagnostics.h"
#include "../utils/emitter.h"
#include "../utils/mangle.h"
#include "../utils/string_list.h"

// Operator precedence for expression parsing

/**
 * @brief Operator precedence for expression parsing.
 */
typedef enum
{
    PREC_NONE,       ///< No precedence.
    PREC_ASSIGNMENT, ///< Assignment operators.
    PREC_TERNARY,    ///< Ternary operator.
    PREC_OR,         ///< Logical OR.
    PREC_AND,        ///< Logical AND.
    PREC_EQUALITY,   ///< Equality operators.
    PREC_COMPARISON, ///< Comparison operators.
    PREC_TERM,       ///< Addition and subtraction.
    PREC_FACTOR,     ///< Multiplication and division.
    PREC_POWER,      ///< Exponentiation (**).
    PREC_UNARY,      ///< Unary operators.
    PREC_CALL,       ///< Function calls.
    PREC_PRIMARY     ///< Primary expressions.
} Precedence;

// Main entry points
// Forward declarations
struct ParserContext;
struct MoveState;
typedef struct ParserContext ParserContext;

/**
 * @brief Attributes for a declaration (e.g., @packed, @cfg).
 */
typedef struct DeclarationAttributes
{
    int is_packed;
    int align;
    char *cfg_condition;
    int vector_size;
    int cuda_global;
    int cuda_device;
    int cuda_host;
    int is_pure;
    int is_required;
    int is_deprecated;
    char *deprecated_msg;
    int is_inline;
    int is_noinline;
    int is_noreturn;
    int is_cold;
    int is_hot;
    int is_constructor;
    int is_destructor;
    int is_unused;
    int is_weak;
    int is_export;
    int is_thread_local;
    int is_comptime;
    char *section;
    Attribute *custom_attributes;
    char **derived_traits;
    int derived_count;
    char *link_name;
    char *crepr_c_type; // @crepr("C.kind.name") attribute
    char *link_path;    // @link("path/to/file.c")
} DeclarationAttributes;

/**
 * @brief Parses attributes (prefixed with @).
 */
DeclarationAttributes parse_attributes(ParserContext *ctx, Lexer *l);

/**
 * @brief Parses a program.
 */
ASTNode *parse_program(ParserContext *ctx, Lexer *l);

extern Type *curr_func_ret; ///< Return type of the function whose body is being parsed.
extern ParserContext *token_parser_ctx;

#include "ast/symbols.h"

/**
 * @brief Registry entry for a function signature.
 *
 * Stores metadata about declared functions for type checking and call validation.
 */
typedef struct FuncSig
{
    char *name;               ///< Function name.
    Token decl_token;         ///< declaration token.
    int total_args;           ///< Total argument count.
    ASTNode **default_values; ///< Default value of each argument, NULL where it has none.
    Type **arg_types;         ///< Argument types.
    Type *ret_type;           ///< Return type.
    int is_varargs;           ///< 1 if variadic.
    int required;             ///< 1 if return value must be used.
    int is_pure;              ///< 1 if marked @pure.
    char *link_name;          ///< Overriding C identifier (from @link_name).
    const char *own_name;     ///< The name the program writes, when the C name adds to it:
                              ///< `push` of `Vec__int32_t__push`, `Rect` of `Shape__Rect`.
    int elide_from_idx;       ///< Index of parameter for lifetime elision (-1 if none)
    struct FuncSig *next;     ///< Next function in registry.
} FuncSig;

/**
 * @brief Tracks a lambda (anonymous function) within the parser.
 */
typedef struct LambdaRef
{
    ASTNode *node; ///< The AST node for the lambda.
    struct LambdaRef *next;
} LambdaRef;

/**
 * @brief Template for a generic struct.
 */
typedef struct GenericTemplate
{
    char *name;           ///< Template name.
    ASTNode *struct_node; ///< The struct AST node (containing generic params).
    struct GenericTemplate *next;
} GenericTemplate;

/**
 * @brief Template for a generic function.
 */
typedef struct GenericFuncTemplate
{
    char *name;          ///< Template name.
    char *generic_param; ///< Generic parameters string (legacy).
    ASTNode *func_node;  ///< The function AST node.
    struct GenericFuncTemplate *next;
} GenericFuncTemplate;

/**
 * @brief Template for a generic implementation block.
 */
typedef struct GenericImplTemplate
{
    char *struct_name;   ///< Target struct name.
    char *generic_param; ///< Generic parameters.
    ASTNode *impl_node;  ///< The impl block AST node.
    struct GenericImplTemplate *next;
} GenericImplTemplate;

/**
 * @brief Tracks a concrete instantiation of a generic template.
 */
typedef struct Instantiation
{
    char *name;          ///< Mangled name of the instantiation (e.g. "Vec_int").
    char *template_name; ///< Original template name (e.g. "Vec").
    Type **args;         ///< The type each parameter of the template takes.
    int count;
    ASTNode *struct_node; ///< The AST node of the instantiated struct.
    struct Instantiation *next;
} Instantiation;

/**
 * @brief Reference to a parsed struct (list node).
 */
typedef struct StructRef
{
    ASTNode *node;
    struct StructRef *next;
} StructRef;

/**
 * @brief Definition of a struct (lookup cache).
 */
typedef struct StructDef
{
    char *name;
    ASTNode *node;
    struct StructDef *next;
} StructDef;

// Hash table entry for fast struct/enum lookup (used by find_struct_def).
enum
{
    STRUCT_HASH_SIZE = 2048
};
typedef struct
{
    char name[256]; // empty string = empty slot (avoids dangling stack pointer issues with ASAN)
    ASTNode *node;
} StructHashEntry;

/**
 * @brief The tuples the program uses, each once, for the C structs codegen emits.
 */
typedef struct TupleType
{
    Type *type; ///< A TYPE_TUPLE.
    char *name; ///< Its C struct name (type_mangle_spelling), to find it without spelling it again.
    struct TupleType *next;
} TupleType;

/**
 * @brief Registry of enum variants.
 */
typedef struct EnumVariantReg
{
    char *enum_name;    ///< Name of the enum.
    char *variant_name; ///< Name of the variant.
    int tag_id;         ///< Integration tag value.
    struct EnumVariantReg *next;
} EnumVariantReg;

/**
 * @brief Functions marked as deprecated.
 */
typedef struct DeprecatedFunc
{
    char *name;
    char *reason; ///< Optional reason for deprecation.
    struct DeprecatedFunc *next;
} DeprecatedFunc;

/**
 * @brief Represents a module (namespace/file).
 */
typedef struct Module
{
    char *alias;      ///< Import alias (or default name).
    char *path;       ///< File path.
    char *base_name;  ///< Base name of the module.
    int is_c_header;  ///< 1 if this is a C header import.
    int is_re_export; ///< 1 if this module was re-exported.
} Module;

/**
 * @brief Symbol imported via selective import (import { X }).
 */
typedef struct SelectiveImport
{
    char *symbol;        ///< Symbol name.
    char *alias;         ///< Local alias.
    char *source_module; ///< Origin module.
} SelectiveImport;

/**
 * @brief Registry for trait implementations.
 */
typedef struct ImplReg
{
    char *trait; ///< Trait name.
    char *strct; ///< Implementing struct name.
    struct ImplReg *next;
} ImplReg;

/**
 * @brief Loaded compiler plugin.
 */
typedef struct ImportedPlugin
{
    char *name;  ///< Plugin name (e.g., "brainfuck").
    char *alias; ///< Optional usage alias.
} ImportedPlugin;

/**
 * @brief Type alias definition.
 */
typedef struct TypeAlias
{
    char *alias;     ///< New type name.
    Type *type_info; ///< The aliased type.
    struct TypeAlias *next;
    int is_opaque;
    char *defined_in_file;
} TypeAlias;

#include "../utils/zmap_types.h"

/**
 * @brief Module/import state container (extracted from ParserContext god struct).
 */
typedef struct ModuleState
{
    zmap_ModMap modules;           ///< Map: alias → Module*.
    zmap_SelMap selective_imports; ///< Map: symbol → SelectiveImport*.
    char *current_module_prefix;   ///< Prefix for current module (namespacing).
    char *current_module_alias;    ///< Alias the current module is imported with (`math` of
                                   ///< `import "m.zc" as math`); NULL for a selective import.
    zmap_FileSet imported_files;   ///< Set: imported file paths (key=value).
    zmap_FileSet
        currently_parsing; ///< Set: files whose import is in progress (for cycle detection).
    zmap_PluginMap imported_plugins; ///< Map: alias → ImportedPlugin*.
    zmap_FileSet wildcard_imports;   ///< Set: module base names imported via `as *`.
    zmap_FileSet module_prefixes;    ///< Map: module file path → its symbol prefix.
    zmap_FileSet prefix_owners;      ///< Map: symbol prefix → the file path that owns it.
} ModuleState;

/* * Initialize all maps in a ModuleState. Call once after zeroing the struct. */
static inline void module_state_init(ModuleState *ms)
{
    ms->modules = zmap_init(ModMap, zmap_hash_cstr, zmap_cmp_cstr);
    ms->selective_imports = zmap_init(SelMap, zmap_hash_cstr, zmap_cmp_cstr);
    ms->imported_files = zmap_init(FileSet, zmap_hash_cstr, zmap_cmp_cstr);
    ms->imported_plugins = zmap_init(PluginMap, zmap_hash_cstr, zmap_cmp_cstr);
    ms->currently_parsing = zmap_init(FileSet, zmap_hash_cstr, zmap_cmp_cstr);
    ms->wildcard_imports = zmap_init(FileSet, zmap_hash_cstr, zmap_cmp_cstr);
    ms->module_prefixes = zmap_init(FileSet, zmap_hash_cstr, zmap_cmp_cstr);
    ms->prefix_owners = zmap_init(FileSet, zmap_hash_cstr, zmap_cmp_cstr);
}

/**
 * @brief A generic parameter in scope (`T` of `fn max<T>`).
 */
typedef struct
{
    char *name;
    Token token; ///< Where it is declared, for the language server.
} GenericParameter;

/**
 * @brief A generic parameter written in a type, for the language server: the declaration
 *        itself too, where `at` is `declaration`.
 */
typedef struct GenericParameterUse
{
    Token at;
    Token declaration;
    struct GenericParameterUse *next;
} GenericParameterUse;

/**
 * @brief A function body whose parsing is postponed until all declarations are known.
 */
typedef struct PendingBody
{
    ASTNode *node;           ///< Function or test whose body is still missing.
    char *name;              ///< Function name as written.
    Lexer body_start;        ///< Lexer positioned at the body's opening '{'.
    Scope *scope;            ///< Scope holding the parameters.
    int in_method_with_self; ///< `.field` shorthand allowed in this body.
    int is_template;         ///< The body of a generic template: parsed before the concrete ones.
    GenericParameter *generics; ///< The generic parameters in scope at the declaration (`T`, `U`).
    int generic_count;
    struct PendingBody *next;
} PendingBody;

/**
 * @brief Where the prototype of a function a whole program may leave out (see
 *        prototype_span_begin) is in the C file being written: bytes [start, end).
 */
typedef struct PrototypeSpan
{
    const char *name; ///< The function's name in C.
    long start;
    long end;
} PrototypeSpan;

/**
 * @brief Global compilation state and symbol table.
 *
 * ParserContext maintains the state of the compiler during parsing and analysis.
 * It holds symbol tables, type definitions, function registries, and configuration.
 */
struct ParserContext
{
    ZenCompiler *compiler;  ///< Reference to the unified compiler state.
    CompilerConfig *config; ///< Shortcut to compiler->config.
    int recursion_depth;    ///< Guard against stack overflow.
    Scope *global_scope;    ///< Root of the unified symbol table.
    Scope *current_scope;   ///< Current lexical scope for variable lookup.
    FuncSig *func_registry; ///< Registry of declared function signatures (DEPRECATED: moved to
                            ///< global_scope).
    zmap_FuncByName func_registry_by_name; ///< func_registry by name: the one registered last.

    // Lambdas
    LambdaRef *global_lambdas; ///< All lambdas generated during parsing, in creation order.
    int lambda_counter;        ///< Counter for generating unique lambda IDs.
    int fstring_counter;       ///< Counter for generating unique f-string IDs.

// Generics
#define MAX_KNOWN_GENERICS 1024
    GenericParameter
        known_generics[MAX_KNOWN_GENERICS]; ///< Stack of currently active generic type parameters.
    int known_generics_count;               ///< Count of active generic parameters.
    GenericParameterUse *generic_parameter_uses; ///< Every one read, newest first.
    GenericTemplate *templates;                  ///< Struct generic templates.
    GenericFuncTemplate *func_templates;         ///< Function generic templates.
    GenericImplTemplate *impl_templates;         ///< Implementation block templates.

    // Instantiations
    Instantiation *instantiations;                  ///< Cache of instantiated generic types.
    struct PendingInstance *pending_instances;      ///< Instances waiting for the declarations.
    struct PendingInstance *pending_instances_tail; ///< Last of `pending_instances`.
    int declarations_read; ///< Set once every declaration is read: instances are made at once.
    ASTNode *instantiated_structs; ///< List of AST nodes for instantiated structs.
    ASTNode *instantiated_funcs;   ///< List of AST nodes for instantiated functions.

    // Structs/Enums
    StructRef *parsed_structs_list;                ///< List of all parsed struct nodes.
    StructHashEntry struct_hash[STRUCT_HASH_SIZE]; ///< Hash table for fast struct/enum lookup.
    StructRef *parsed_enums_list;                  ///< List of all parsed enum nodes.
    StructRef *parsed_funcs_list;                  ///< List of all parsed function nodes.
    StructRef *parsed_impls_list;                  ///< List of all parsed impl blocks.
    StructRef *parsed_globals_list;                ///< List of all parsed global variables.
    StructDef *struct_defs;        ///< Registry of struct definitions (map name -> node).
    EnumVariantReg *enum_variants; ///< Registry of enum variants for global lookup.
    ImplReg *registered_impls;     ///< Cache of type/trait implementations.

    // The std modules the compiler loads on its own (std/mem.zc for Drop…), as NODE_IMPORTs
    // for the typechecker: they are not in the tree.
    ASTNode *implicit_std_imports;

    // Names find_struct_def did not find, cached during codegen (utils_registry.c).
    struct StructMissCache *struct_misses;
    unsigned int struct_registry_gen; ///< Bumped whenever a struct or enum is registered.

    // Types
    TupleType *used_tuples;  ///< Cache of generated tuple types.
    TypeAlias *type_aliases; ///< Defined type aliases.

    // Modules/Imports
    ModuleState imports;
    const char *current_filename; ///< Current file being parsed (for token/file attribution).

    // Config/State
    Type *current_impl_target;     ///< Type the impl being read adds methods to (`Vec<T>` in
                                   ///< `impl Vec<T>`), or NULL outside an impl.
    ASTNode *current_impl_methods; ///< Head of method list for current impl block.
    int in_method_with_self;       ///< 1 if parsing body of method with self parameter.
    int self_is_pointer;           ///< 1 if self is a pointer receiver (self*).
    int in_constant_expression;    ///< 1 while parse_constant_expression reads an expression.

    // Deferred function bodies
    PendingBody *pending_bodies;               ///< First body waiting to be parsed (FIFO).
    PendingBody *pending_bodies_tail;          ///< Last one, to append in O(1).
    PendingBody *pending_template_bodies;      ///< Bodies of generic templates, parsed first.
    PendingBody *pending_template_bodies_tail; ///< Last of `pending_template_bodies`.
    struct BodyCopy *body_copies; ///< Instances made before their template's body was read.
    int parse_depth;              ///< Nesting of parse_program_nodes; bodies are parsed at 0.

    // Internal tracking
    DeprecatedFunc *deprecated_funcs; ///< Registry of deprecated functions.

    // LSP / Fault Tolerance
    int is_fault_tolerant; ///< 1 if parser should recover from errors (LSP mode).
    int suppress_errors;   ///< 1 to swallow parser errors silently (probe parses).
    int had_error; ///< Set by zpanic_at when fault-tolerant; checked by parser loops to bail out.

    // Every symbol seen, for the language server, the MISRA checks and find_symbol_in_all
    ZenSymbol *all_symbols;                ///< comprehensive list of all symbols seen.
    zmap_DeclSet all_symbols_set;          ///< all_symbols by declaration (kind, name, position).
    zmap_SymbolByName all_symbols_by_name; ///< all_symbols by name: the last one registered.

    // External C interop: suppress undefined warnings for external symbols
    int has_external_includes;
    zmap_FileSet c_function_types; ///< C function name -> its return type as C text ("": none).
    zmap_FileSet c_typedef_types;  ///< C typedef name -> the type it names as C text.
    zmap_FileSet c_variable_types; ///< C variable name -> its type as C text.
    zmap_FileSet c_macro_names;    ///< C macro that is only another name -> that name.
    StringList c_system_includes;  ///< The headers of `include <x.h>`, as written.
    int c_system_headers_read;     ///< Those and the preamble's are in the maps above.
    int is_comptime;               // Flag for comptime execution context
    char **extern_symbols;         ///< Explicitly declared extern symbols.
    int extern_symbol_count;       ///< Count of external symbols.

    // Codegen context (mutable state during code generation):
    struct
    {
        Emitter emitter;    ///< Emitter for code generation.
        FILE *hoist_out;    ///< File stream for hoisting code (e.g. from plugins).
        int skip_preamble;  ///< If 1, codegen won't emit standard preamble (includes etc).
        int is_repl;        ///< 1 if running in REPL mode.
        int in_defer_block; ///< 1 if currently parsing inside a defer block.

        ASTNode *global_user_structs;     ///< List of user defined structs.
        char *current_impl_type;          ///< Type currently being implemented (in impl block).
        int tmp_counter;                  ///< Counter for temporary variables.
        int fstring_counter;              ///< Counter for the buffers of f-string values.
        ASTNode *defer_stack[1024];       ///< Stack of deferred nodes (max 1024).
        int defer_count;                  ///< Counter for defer statements in current scope.
        ASTNode *current_lambda;          ///< Current lambda being generated.
        Type *current_func_ret_type_info; ///< Return type of the function being generated.
        Type *expected_init_type; ///< Declared type of the `let` whose initializer is parsed.
        int *loop_defer_boundary; ///< Defer stack index at start of each enclosing loop.
        int loop_defer_capacity;  ///< Allocated slots of loop_defer_boundary.
        int loop_depth;           ///< Current loop nesting depth.
        int func_defer_boundary;  ///< Defer stack index at function entry.

        /// Write only the function bodies the program reaches (a whole program, written to a
        /// file): see emit_function_bodies.
        int emit_reached_only;
        PrototypeSpan *prototype_spans; ///< For remove_unused_prototypes.
        int prototype_span_count;
        int prototype_span_cap;
        const char *open_span_name; ///< The prototype being written, or NULL.
        long open_span_start;
        const char **written_bodies; ///< Sorted C names of the bodies emit_function_bodies wrote.
        int written_body_count;
        const char **left_out_bodies; ///< Sorted C names of the bodies it left out.
        int left_out_body_count;
    } cg;

    // Type Validation
    struct TypeUsage *pending_type_validations; ///< List of types to validate after parsing.
    int is_speculative;     ///< Flag to suppress side effects during speculative parsing.
    int silent_warnings;    ///< Suppress warnings (e.g., during codegen interpolation).
    char *last_doc_comment; ///< Last seen doc-comment (for association).

    // Flow Analysis (Move Semantics)
    struct MoveState *move_state;

    // Registry of traits (encapsulated)
    TraitReg *registered_traits;

    // Extensibility hooks — function pointers for optional modules.
    // When NULL, the corresponding feature is disabled/no-op.
    // Set by driver.c at startup based on configuration.

    /// Hook: identifier collision check (MISRA Rule 5.1/5.2)
    void (*hook_check_identifier_collision)(Token tok, const char *name1, const char *name2,
                                            int limit);

    /// Hook: preprocessor expression check (MISRA Rule 20.8/20.9)
    void (*hook_check_preprocessor_expr)(struct ParserContext *ctx, Token tok,
                                         const char *expression);

    /// Hook: standard macro name check (MISRA Rule 5.10)
    void (*hook_check_standard_macro_name)(Token tok, const char *name);

    /// Hook: plugin lookup
    void *(*hook_find_plugin)(const char *name);

    /// Hook: plugin API initialization
    void (*hook_plugin_init_api)(void *api, const char *filename, int line,
                                 struct CompilerConfig *cfg);

    /// Hook: zen fact trigger
    int (*hook_zen_trigger)(int t, Token location, struct CompilerConfig *cfg);
};

// Intrusive linked-list iteration utilities
// zlist.h (src/utils/zlist.h) provides full node-based doubly-linked lists.
// These macros are for the compiler's existing intrusive singly-linked lists
// where the `next` pointer is embedded directly in the struct.
#define LIST_FOR_EACH(head, cursor, next_field)                                                    \
    for (cursor = (head); cursor; cursor = cursor->next_field)

#define LIST_FOR_EACH_SAFE(head, cursor, tmp, next_field)                                          \
    for (cursor = (head), tmp = cursor ? cursor->next_field : NULL; cursor;                        \
         cursor = tmp, tmp = cursor ? cursor->next_field : NULL)

// Recursion Safety
enum
{
    MAX_RECURSION_DEPTH = 1024
};

#define RECURSION_GUARD(ctx, l, ret)                                                               \
    if (++((ctx)->recursion_depth) > MAX_RECURSION_DEPTH)                                          \
    {                                                                                              \
        zpanic_at(lexer_peek(l), "Recursion limit exceeded");                                      \
        (ctx)->recursion_depth--;                                                                  \
        return ret;                                                                                \
    }

#define RECURSION_GUARD_TOKEN(ctx, tok, ret)                                                       \
    if (++((ctx)->recursion_depth) > MAX_RECURSION_DEPTH)                                          \
    {                                                                                              \
        zpanic_at(tok, "Recursion limit exceeded");                                                \
        (ctx)->recursion_depth--;                                                                  \
        return ret;                                                                                \
    }

#define RECURSION_EXIT(ctx) ((ctx)->recursion_depth)--

// Break out of a parsing loop when EOF is reached unexpectedly.
// Place after the closing-delimiter checks in every while(1) token loop.
#define TOK_EOF_GUARD(tok)                                                                         \
    if ((tok).kind == TOK_EOF)                                                                     \
    {                                                                                              \
        zpanic_at(tok, "Unexpected end of file");                                                  \
        break;                                                                                     \
    }

#define ATTACH_DOC_COMMENT(ctx, node)                                                              \
    if ((node) && (ctx)->last_doc_comment)                                                         \
    {                                                                                              \
        (node)->doc_comment = (ctx)->last_doc_comment;                                             \
        (ctx)->last_doc_comment = NULL;                                                            \
    }

/** @brief A generic instance read in a declaration before its template or an argument was declared.
 */
typedef struct PendingInstance
{
    Type *type;  ///< The instance (`Vec<PId>`), resolved in place.
    Token token; ///< Where it was read, for errors.
    struct PendingInstance *next;
} PendingInstance;

typedef struct TypeUsage
{
    char *name;
    Token location;
    struct TypeUsage *next;
} TypeUsage;

// Type validation prototypes

/**
 * @brief Registers a type usage.
 */
void register_type_usage(ParserContext *ctx, const char *name, Token t);

/**
 * @brief Whether `name`, read where a type goes, names one: a struct, enum, alias, trait,
 * primitive, extern symbol, selective import or a C type of the whitelist.
 */
int is_known_type_name(ParserContext *ctx, const char *name);

/**
 * @brief Warns that the type `use` names is unknown, with the closest known name as a hint.
 */
void warn_unknown_type(ParserContext *ctx, const TypeUsage *use);

/**
 * @brief Warns about every type name the parse read that names no type. Always returns 1:
 * the C compiler decides whether the name is an external C type.
 */
int validate_types(ParserContext *ctx);

/**
 * @brief Reports functions and methods defined twice under the same C name.
 *
 * Prototypes, externs, generic templates, `@cfg` definitions and definitions
 * inside a preprocessor `#if` block are not counted. Returns 0 on error.
 */
int validate_function_definitions(ParserContext *ctx, ASTNode *root);

/**
 * @brief Traverses all parsed structs and propagates `has_drop` from fields to their parent
 * structs.
 */
void propagate_drop_traits(ParserContext *ctx);
/**
 * @brief Whether a value of type `t` needs dropping: an owning closure, a struct with Drop, or a
 *        tuple or fixed-size array holding a value that needs dropping.
 */
int type_needs_drop(ParserContext *ctx, Type *t);
/**
 * @brief Whether a value of type `t` will be dropped, decided from the declarations alone: what
 *        type_needs_drop says once propagate_drop_traits has run, but also before it, while bodies
 *        are parsed (a struct with an `impl Drop` or a field that will be dropped).
 */
int type_will_drop(ParserContext *ctx, Type *t);
void fix_type_refs_has_drop(ParserContext *ctx);

/**
 * @brief Propagates inner types for vector types (SIMD).
 */
void propagate_vector_inner_types(ParserContext *ctx);

// Token helpers

/**
 * @brief Duplicates a token.
 */
char *token_strdup(Token t);

/**
 * @brief Checks if a token matches a string.
 */
int is_token(Token t, const char *s);

/**
 * @brief Expects a token of a specific type.
 */
Token z_parse_expect(Lexer *l, ZenTokenType type, const char *msg);

/**
 * @brief Skips comments in the lexer.
 */
void skip_comments(Lexer *l);
void token_set_parser_ctx(ParserContext *ctx);

/**
 * @brief Consumes tokens until a semicolon is found.
 */
char *consume_until_semicolon(Lexer *l);

/**
 * @brief Skips a `{ ... }` block, braces included. The next token must be `{`.
 * @return 1 on success, 0 if the file ends before the block is closed.
 */
int skip_braced_block(Lexer *l);

// Errors for syntax Zen C does not have, each with what to write instead
// (utils_syntax_hints.c).
void report_unexpected_toplevel(Token t);
void report_toplevel_const(Lexer *l, Token t);
void report_local_const(Token t);
/** @return 1 if `extern` is followed by `enum`, `const`, `def`, `type` or `alias` (reported). */
int report_extern_with_kind(Lexer *l);
void report_not_a_field(Lexer *l, Token t);

/**
 * @brief Consumes and rewrites tokens.
 */

// C reserved word warnings

/**
 * @brief Checks if a name is a C reserved word.
 */
int is_c_reserved_word(const char *name);

// ZenSymbol table

/**
 * @brief Enters a new scope (pushes to scope stack).
 */
/**
 * @brief Checks if a character is valid in an identifier (isalnum or underscore).
 */
int is_ident_char(char c);

void enter_scope(ParserContext *ctx);

/**
 * @brief Exits the current scope (pops from scope stack).
 */
void exit_scope(ParserContext *ctx);

/**
 * @brief Adds a symbol to the current scope.
 */
void add_symbol(ParserContext *ctx, const char *n, Type *type_info, int is_export);

/**
 * @brief Adds a symbol with definition token location.
 */
void add_symbol_with_token(ParserContext *ctx, const char *n, Type *type_info, Token tok,
                           int is_export);

/**
 * @brief Finds a symbol's type information.
 */
Type *find_symbol_type_info(ParserContext *ctx, const char *n);

/**
 * @brief Finds a symbol's entry.
 */
ZenSymbol *find_symbol_entry(ParserContext *ctx, const char *n);

/**
 * @brief Finds a symbol in all scopes.
 */
ZenSymbol *find_symbol_in_all(ParserContext *ctx, const char *n);
char *find_similar_symbol(ParserContext *ctx, const char *name);
/**
 * @brief Picks the instance for `Template::method` from the type of the `let` being
 * initialized: `let w: Wrap<int> = Wrap::make(3);` calls `Wrap__int32_t__make`.
 * @return That function's name, or NULL if the `let` is not declared as an instance of the
 * template or the instance has no such method.
 */
char *expected_instance_method(ParserContext *ctx, const char *template_name,
                               const char *method_name);
/**
 * @brief The function that `value.method()` calls on a value of the type `type_name` names
 * (as `type_impl_name` gives it): `Type__method`, the method of a trait the type implements
 * (`Type__Trait__method`), also through a generic impl, or the method of a struct it mixes in
 * with `use`.
 * @return 1 if there is one. Its name is `*owner_out` (`type_name`, or the mixed-in struct)
 * followed by `*method_out` (`method`, or `Trait__method`). 0 if the type has no such method.
 */
int find_method_call_target(ParserContext *ctx, const char *type_name, const char *method,
                            const char **owner_out, char **method_out);
/**
 * @brief Where a generic impl of `template_name` declares `method_name`: the declaration of
 * `Vec<int>::new`, whose instance is not a function until it is instantiated.
 * @return The token of the method's name, or TOKEN_UNKNOWN if no generic impl declares it.
 */
Token template_method_token(ParserContext *ctx, const char *template_name, const char *method_name);

/**
 * @brief Normalizes a type name (e.g., "int" -> "int32_t").
 */
const char *normalize_type_name(const char *name);

// Function registry

/**
 * @brief Registers a function. `own_name` is the name the program writes when the C name adds to
 * it (a method's or a variant's); NULL for a free function.
 */
void register_func(ParserContext *ctx, Scope *scope, const char *name, int count,
                   ASTNode **default_values, Type **arg_types, Type *ret_type, int is_varargs,
                   int is_pure, const char *link_name, const char *own_name, Token decl_token,
                   int is_export);

/**
 * @brief The name a call's callee was written with: a variant's (`Ok` of `Result__Ok`), a
 * method's (`push` of `Vec__int32_t__push`) or a generic function's (`id` of `id__int32_t`).
 * @return NULL when the C name is that name.
 */
const char *callee_own_name(ParserContext *ctx, ASTNode *callee);

/**
 * @brief A copy of the default value of argument `index` of `sig`, for a call that leaves the
 *        argument out. Each call gets its own: it is linked into the call's arguments, and
 *        later passes rewrite it in place.
 *
 * The copy is compiled where the call is, so a name in it that is a local variable there
 * would be that variable, not the one the function sees: that is an error, at `call`.
 */
ASTNode *copy_default_value(ParserContext *ctx, FuncSig *sig, int index, Token call);

/**
 * @brief Appends to `refs` (a copy of) each variable name `node` uses, f-string expressions
 *        included. Of a nested lambda, only the names it captures; nothing of a comptime block.
 */
void find_var_refs(ASTNode *node, char ***refs, int *ref_count);

/**
 * @brief What each parameter of a generic stands for in one of its instances: `T` is `int`.
 */
typedef struct Bindings
{
    char **params; ///< The template's parameters: `K`, `V`.
    Type **args;   ///< The type each one takes: `int`, `String`.
    int count;
} Bindings;

/**
 * @brief A function of an instance copied before its template's body was read: the body is
 * copied once it is.
 */
typedef struct BodyCopy
{
    ASTNode *instance;          ///< The copy, still without a body.
    ASTNode *template_function; ///< The function it was copied from.
    Bindings bindings;          ///< What the template's parameters stand for in the instance.
    const char *template_name;  ///< The template's type name (`Vec`), for copy_ast_replacing.
    const char *instance_name;  ///< The instance's type name (`Vec__int32_t`).
    struct BodyCopy *next;
} BodyCopy;

/** @brief `parts` joined by commas: `K,V`. */
char *join_with_commas(char **parts, int count);
/** @brief The bindings of `params` (`K,V`) to `args`. */
Bindings bindings_new(const char *params, Type **args, int count);
/**
 * @brief A copy of `t` for an instance: each parameter becomes the type it is bound to, and the
 * template's name `os` without arguments (`Self`) becomes the instance's `ns`.
 */
Type *replace_type_formal(ParserContext *ctx, Type *t, const Bindings *b, const char *os,
                          const char *ns);
/** @brief A copy of the AST `n` for an instance, with replace_type_formal on its types. */
ASTNode *copy_ast_replacing(ParserContext *ctx, ASTNode *n, const Bindings *b, const char *os,
                            const char *ns);

/**
 * @brief Registers a function template.
 */
void register_func_template(ParserContext *ctx, const char *name, const char *param, ASTNode *node);

/**
 * @brief Finds a function template.
 */
GenericFuncTemplate *find_func_template(ParserContext *ctx, const char *name);

// Generic/template helpers
/**
 * @brief Registers a known generic type parameter, declared at `name`.
 */
void register_generic(ParserContext *ctx, Token name);

/**
 * @brief Records `t`, a known generic parameter written in a type, for the language server.
 */
void record_generic_parameter_use(ParserContext *ctx, Token t);

/**
 * @brief Checks if a name is a known generic parameter.
 */
int is_known_generic(ParserContext *ctx, char *name);

/** @brief Whether `t` still names a generic parameter (`Vec<T>` inside its template). */
int type_is_generic_dependent(ParserContext *ctx, Type *t);

/**
 * @brief Checks if a name is a primitive type.
 */
int is_primitive_type_name(const char *name);

/**
 * @brief Maps a primitive type name string to its `TypeKind` enum.
 */
TypeKind get_primitive_type_kind(const char *name);

/**
 * @brief Registers an implementation template.
 */
void register_impl_template(ParserContext *ctx, const char *sname, const char *param,
                            ASTNode *node);
/**
 * @brief Adds a struct to the list.
 */
void add_to_struct_list(ParserContext *ctx, ASTNode *node);

/**
 * @brief Adds an enum to the list.
 */
void add_to_enum_list(ParserContext *ctx, ASTNode *node);

/**
 * @brief Adds a function to the list.
 */
void add_to_func_list(ParserContext *ctx, ASTNode *node);

/**
 * @brief Adds an implementation to the list.
 */
void add_to_impl_list(ParserContext *ctx, ASTNode *node);

/**
 * @brief Adds a global to the list.
 */
void add_to_global_list(ParserContext *ctx, ASTNode *node);

/**
 * @brief Registers the built-in names: `true`, `false` and C's `va_list`. The C functions and
 * variables a program calls undeclared take their types from the C headers
 * (c_function_return_type, c_variable_type).
 */
void register_builtins(ParserContext *ctx);
void register_comptime_builtins(ParserContext *ctx);
void register_tuple(ParserContext *ctx, Type *tuple);
ASTNode *parse_tuple_expression(ParserContext *ctx, Lexer *l, Type *tuple, ASTNode *expr);
/** @brief Whether a tuple literal comes next: `(` and a comma before its `)`. */
int tuple_literal_follows(Lexer *l);

/** @brief Parses just the comptime block body, returning the statement list. */
ASTNode *parse_comptime_body(ParserContext *ctx, Lexer *l);

/**
 * @brief Patches self arguments in a function.
 */
char *escape_c_string(const char *input);
char *resolve_struct_name_from_type(ParserContext *ctx, Type *t, int *is_ptr_out,
                                    char **allocated_out);
/**
 * @brief Parses the pieces of an f-string into a NODE_EXPR_FSTRING.
 *
 * Text pieces keep their characters (braces unescaped); each `{expr[:fmt]}` is parsed and
 * typed with the current scope, with warnings silenced. No C is built here: the codegen
 * decides how each piece is written and where (a buffer for a value, a stream for a direct
 * print). Returns NULL after reporting an error.
 */
ASTNode *parse_fstring_parts(ParserContext *ctx, Token srctoken, const char *content);

/** @brief Rejects a plain "..." that interpolates or escapes braces (RFC 0001: only f"..."). */
void reject_implicit_interpolation(ParserContext *ctx, Token t, const char *content);

/**
 * @brief Checks if a token is a reserved keyword.
 */
int is_reserved_keyword(Token t);

/**
 * @brief Checks if an identifier is valid (not a keyword).
 */
void check_identifier(Token t);
void check_reserved_keyword(Token t);
void check_c_identifier(Token t);

/**
 * @brief Main loop to parse top-level nodes in a file.
 */
ASTNode *parse_program_nodes(ParserContext *ctx, Lexer *l);

// --- Parser function declarations (needed to avoid implicit declarations) ---
ASTNode *parse_statement(ParserContext *ctx, Lexer *l);
ASTNode *parse_block(ParserContext *ctx, Lexer *l);
/** A block that is the value of an expression: its last string literal is the value. */
ASTNode *parse_value_block(ParserContext *ctx, Lexer *l);
ASTNode *parse_expression(ParserContext *ctx, Lexer *l);
/**
 * An expression the compiler computes while parsing (an array size, the value of an enum
 * variant, a C macro): it may name a variant without its enum (`[int; Big]`).
 */
ASTNode *parse_constant_expression(ParserContext *ctx, Lexer *l);
ASTNode *parse_expr_prec(ParserContext *ctx, Lexer *l, Precedence min_prec);
ASTNode *parse_return(ParserContext *ctx, Lexer *l);
ASTNode *parse_assert(ParserContext *ctx, Lexer *l);
ASTNode *parse_expect(ParserContext *ctx, Lexer *l);
ASTNode *parse_plugin(ParserContext *ctx, Lexer *l, Token tk);
char *token_get_string_content(Token t);
/**
 * @brief Where the content of the string literal `t` starts in its text: the length of its
 *        opening quotes (`"`, `f"`, `"""`...). *end_offset, if given, gets that of its closing
 * ones.
 */
int token_string_content_offset(Token t, int *end_offset);
ASTNode *find_struct_def(ParserContext *ctx, const char *name);
ASTNode *parse_function(ParserContext *ctx, Lexer *l, int is_extern, const char *link_name,
                        int is_export);
void push_pending_body(ParserContext *ctx, PendingBody *pending);
void check_unused_parameters(ParserContext *ctx, Scope *scope, const char *func_name);
void parse_pending_bodies(ParserContext *ctx);
ASTNode *parse_struct(ParserContext *ctx, Lexer *l, int is_union, int is_packed, int align,
                      const char *link_name, int is_export);
ASTNode *parse_enum(ParserContext *ctx, Lexer *l, const char *link_name, int is_export);
ASTNode *parse_impl(ParserContext *ctx, Lexer *l);
ASTNode *parse_trait(ParserContext *ctx, Lexer *l);
ASTNode *parse_include(ParserContext *ctx, Lexer *l);
ASTNode *parse_import(ParserContext *ctx, Lexer *l, int is_re_export);
ASTNode *parse_def(ParserContext *ctx, Lexer *l, int is_export);
ASTNode *parse_test(ParserContext *ctx, Lexer *l);
ASTNode *parse_var_decl(ParserContext *ctx, Lexer *l, int is_export);
ASTNode *parse_type_alias(ParserContext *ctx, Lexer *l, int is_opaque, int is_export);
int parse_and_convert_args(ParserContext *ctx, Lexer *l, ASTNode ***default_values, int *count,
                           Type ***arg_types, char ***param_names, Token **param_tokens,
                           int *is_varargs, char ***ctype_overrides);
Type *parse_type_formal(ParserContext *ctx, Lexer *l);
Type *type_from_string_helper(ParserContext *ctx, const char *c);
Type *type_from_name(ParserContext *ctx, const char *name);
Type *alias_type(TypeAlias *alias_node);
FuncSig *find_func(ParserContext *ctx, const char *name);
EnumVariantReg *find_enum_variant(ParserContext *ctx, const char *name);
/**
 * @brief The C name of the struct or enum `name`: the @link_name of its definition, or NULL
 *        when it has none. A CNameLookup over a ParserContext.
 */
const char *link_name_of(void *ctx, const char *name);
/**
 * @brief The C name a struct or enum definition takes: its @link_name (`link_name`, may be
 *        NULL), else `name__zc` when `name` is a keyword of the C or C++ being generated, else
 *        NULL (the definition goes by its name).
 */
char *definition_link_name(ParserContext *ctx, const char *link_name, const char *name);
EnumVariantReg *find_enum_variant_of(ParserContext *ctx, const char *enum_name,
                                     const char *variant_name);
/**
 * @brief The type a match on a value of type `scrutinee` takes apart: the value's type, with
 *        aliases and pointers followed.
 */
Type *match_subject_type(Type *scrutinee);
/**
 * @brief Whether a match covers every value of its subject (of type `scrutinee`, may be
 *        NULL): NULL if it does, else the first variant it misses, or "_" when the subject is
 *        not an enum, an Option or a Result, so only a default case covers it. A case with a
 *        guard covers nothing.
 */
const char *match_missing_variant(ParserContext *ctx, ASTNode *match, Type *scrutinee);
int expr_is_rvalue(ParserContext *ctx, ASTNode *e);
TypeAlias *find_type_alias_node(ParserContext *ctx, const char *name);
char *extract_module_name(const char *path);
char *module_prefix_for(ParserContext *ctx, const char *path);
ASTNode *transform_to_trait_object(ParserContext *ctx, const char *target_trait, ASTNode *expr);
int check_impl(ParserContext *ctx, const char *trait_name, const char *type_name);
Instantiation *find_instantiation(ParserContext *ctx, const char *name);
int check_opaque_alias_compat(ParserContext *ctx, Type *a, Type *b);
Module *find_module(ParserContext *ctx, const char *alias);
/**
 * @brief How programs write `name`, declared in the module being parsed: `math::Vector` when
 * the module is imported with an alias, `Vector` when its names are imported one by one.
 */
char *module_written_name(ParserContext *ctx, const char *name);
/**
 * @brief A type as the program writes it (type_to_zen_c_string_with), with a struct or enum of
 * a module written as its importer writes it: `math::Vector`, not `_math__Vector`.
 */
char *zen_c_type_spelling(ParserContext *ctx, Type *t);
SelectiveImport *find_selective_import(ParserContext *ctx, const char *name);
void re_export_wildcard_symbols(ParserContext *ctx, const char *module_base);
void re_export_propagated(ParserContext *ctx, const char *alias, const char *parent_prefix,
                          const char *base_name);
ASTNode *find_concrete_struct_def(ParserContext *ctx, const char *name);
Type *find_type_alias(ParserContext *ctx, const char *alias);
Type *parse_type_base(ParserContext *ctx, Lexer *l);
char *parse_type(ParserContext *ctx, Lexer *l);
ASTNode *find_trait_def(ParserContext *ctx, const char *name);
int is_extern_symbol(ParserContext *ctx, const char *name);
int is_file_imported(ParserContext *ctx, const char *path);
void mark_file_imported(ParserContext *ctx, const char *path);
/**
 * @brief Whether the headers of the C preamble declare `name` (strstr, fputs, va_end...): it
 *        can be called without a Zen C declaration, and a declaration needs no prototype.
 */
int is_preamble_c_function(ParserContext *ctx, const char *name);
/**
 * @brief Whether a call to or use of the undeclared `name` may still reach C: a preamble
 *        function, a runtime helper, an extern symbol, a zenc.json function, a macro (all
 *        caps), or anything when the program includes or imports C headers.
 */
int should_suppress_undef_warning(ParserContext *ctx, const char *name);
struct CHeaderFile;
/// The Type of a C type written as C text (`const char *`, `size_t`, `struct tm *`), or NULL.
Type *c_type_from_text(ParserContext *ctx, const char *text);
/// Remembers the return types of the functions a C header declares, and the types its
/// typedefs name.
void register_c_header_types(ParserContext *ctx, const struct CHeaderFile *header);
/// The return type of a C function the program calls undeclared, from the headers it includes
/// and those of the hosted preamble; NULL when it is not known.
Type *c_function_return_type(ParserContext *ctx, const char *name);
/// Whether a C header the program sees (its includes and those of the preamble) declares the
/// function `name`: a definition of it in the program must keep external linkage.
int c_headers_declare_function(ParserContext *ctx, const char *name);
/// The type of the C variable `name` its header declares (`stdin`, through a macro such as
/// `#define stdin __stdinp`), or NULL.
Type *c_variable_type(ParserContext *ctx, const char *name);
/// Reads the headers of the hosted preamble again into the cache on disk (`zc cache
/// c-headers`): how many headers it holds, or -1 when it could not be written.
int refresh_c_header_cache(const CompilerConfig *config);
void init_builtins(void);
void register_extern_symbol(ParserContext *ctx, const char *name);
void register_impl(ParserContext *ctx, const char *trait, const char *type);
void register_struct_def(ParserContext *ctx, const char *name, ASTNode *node);
void register_type_alias(ParserContext *ctx, const char *alias, Type *type, int is_opaque,
                         const char *defined_in_file, Token tok, int is_export);
/** @brief The type of a slice of `elem`, `[T]`: the instance `Slice<T>` of a built-in template. */
Type *instantiate_slice(ParserContext *ctx, Type *elem, Token token);
#if FF_COMPATIBILITY_0_5
/**
 * @brief The slice a 0.5.1 name stands for (`Slice_char` is `Slice<char>`), or NULL when the
 * name is not `Slice_` followed by a primitive, a struct, an enum or an alias.
 */
Type *compat_slice_from_name(ParserContext *ctx, const char *name, Token token);
#endif
void register_lambda(ParserContext *ctx, ASTNode *node);
void register_template(ParserContext *ctx, const char *name, ASTNode *node);
void register_enum_variant(ParserContext *ctx, const char *vname, const char *ename, int tag);
void register_plugin(ParserContext *ctx, const char *name, const char *alias);
void register_selective_import(ParserContext *ctx, const char *symbol, const char *alias,
                               const char *source_module);
void register_deprecated_func(ParserContext *ctx, const char *name, const char *replacement);
void try_parse_macro_const(ParserContext *ctx, const char *name);
void parser_audit_preprocessor(ParserContext *ctx, Token t);
char *normalize_raw_content(const char *content);
char *parse_condition_raw(ParserContext *ctx, Lexer *l);
const char *resolve_plugin(ParserContext *ctx, const char *name_or_alias);
ASTNode *parse_embed(ParserContext *ctx, Lexer *l);
ASTNode *parse_macro_call(ParserContext *ctx, Lexer *l, char *macro_name);
/** @brief The C name of an instance of a generic function: `f__int32_t` for `f<int>`. */
char *function_instance_name(FunctionInstance instance);
/**
 * @brief Instantiates a generic function, unless its arguments still name a generic parameter.
 * @return Its C name, or NULL if `instance.name` is not a generic function.
 */
char *instantiate_function(ParserContext *ctx, FunctionInstance instance);
/** @brief The generic struct or enum template named `name`, or NULL. */
GenericTemplate *find_generic_template(ParserContext *ctx, const char *name);
/**
 * @brief Instantiates the generic a type names (`Vec<int>`), unless its arguments still name a
 * generic parameter.
 */
void instantiate_type(ParserContext *ctx, Type *instance, Token token);
/**
 * @brief Instantiates the generic instances that declarations named before their template or an
 *        argument was declared, once every declaration is read; an alias argument is replaced by
 *        the type it names.
 */
void resolve_pending_instances(ParserContext *ctx);
/**
 * @brief Gives the instances copied before their template's body was read a copy of that body,
 *        and instantiates the generics the copies name.
 */
void copy_instance_bodies(ParserContext *ctx);

// stmt/ declarations
ASTNode *parse_match_after_keyword(ParserContext *ctx, Lexer *l, Token start_token);
ASTNode *parse_match(ParserContext *ctx, Lexer *l);
ASTNode *parse_loop(ParserContext *ctx, Lexer *l);
ASTNode *parse_defer(ParserContext *ctx, Lexer *l);
ASTNode *parse_asm(ParserContext *ctx, Lexer *l);
ASTNode *parse_if(ParserContext *ctx, Lexer *l);
ASTNode *parse_while(ParserContext *ctx, Lexer *l);
ASTNode *parse_for(ParserContext *ctx, Lexer *l);

// utils/ declarations
void audit_section_5(ParserContext *ctx, Scope *scope, const char *name, const char *link_name,
                     Token tok);
void instantiate_methods(ParserContext *ctx, GenericImplTemplate *it,
                         const char *mangled_struct_name, Type **args, int count);
const char *get_closest_type_hint(ParserContext *ctx, const char *name);
void add_instantiated_func(ParserContext *ctx, ASTNode *fn);
char *ast_to_string(ASTNode *node);
void struct_hash_insert(ParserContext *ctx, const char *name, ASTNode *node);
/** @brief Records that a struct or enum was registered, so cached misses may be stale. */
void struct_registry_changed(ParserContext *ctx);
/** @brief Starts caching the names find_struct_def does not find (used by codegen). */
void struct_miss_cache_enable(ParserContext *ctx);
/** @brief Adds a copy of s to ctx->all_symbols, unless its declaration is already there. */
void register_in_all_symbols(ParserContext *ctx, ZenSymbol *s);

// core/ declarations
// Rejects a name `@derive` does not know, listing the ones it does.
void check_derivable_trait(Token t, const char *name);
ASTNode *generate_derive_impls(ParserContext *ctx, ASTNode *strct, char **traits, int count);

// decl/ declarations
void replace_it_with_var(ASTNode *node, char *var_name);

// struct/ declarations
void load_std_module(ParserContext *ctx, const char *path);
void auto_import_std_mem(ParserContext *ctx);
Type *parse_type_obj(ParserContext *ctx, Lexer *l);

// expr/ declarations
ASTNode *parse_primary(ParserContext *ctx, Lexer *l);
Precedence get_token_precedence(Token t);

#endif // PARSER_H
