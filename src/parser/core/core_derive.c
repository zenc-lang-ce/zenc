// SPDX-License-Identifier: MIT
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analysis/const_fold.h"
#include "analysis/move_check.h"
#include "ast/ast.h"
#include "ast/primitives.h"
#include "constants.h"
#include "parser.h"
#include "plugins/plugin_manager.h"
#include "utils/emitter.h"
#include "utils/utils.h"

// 1 if `type` has a `clone` method: a concrete one (`String__clone`, `Point__Clone__clone`), one
// from a generic impl of its template (`Vec<int>` from `impl Vec<T>`), or one it derives further
// down.
static int type_has_clone_method(ParserContext *ctx, Type *type)
{
    char *type_name = type_def_name(type);
    // From `impl X` or from `impl Clone for X`.
    if (find_func(ctx, mangle_method_symbol(type_name, NULL, "clone")) ||
        find_func(ctx, mangle_method_symbol(type_name, "Clone", "clone")) ||
        derives_pending(ctx, type_name, "Clone"))
    {
        return 1;
    }
    // An instance's template is in its type (`Vec` of `Vec<int>`), made or not yet.
    const char *template_name = type->count > 0 ? type->name : type_name;
    for (GenericImplTemplate *it = ctx->impl_templates; it; it = it->next)
    {
        if (strcmp(it->struct_name, template_name) != 0)
        {
            continue;
        }
        ASTNode *impl = it->impl_node;
        ASTNode *methods = impl->kind == NODE_IMPL ? impl->impl.methods : impl->impl_trait.methods;
        for (ASTNode *m = methods; m; m = m->next)
        {
            if (m->kind == NODE_FUNCTION && m->func.name && strcmp(m->func.name, "clone") == 0)
            {
                return 1;
            }
        }
    }
    return 0;
}

// Whether a field of type `t` is a `Vec<String>`, which the JSON derives read and write as an
// array of strings.
static int is_vec_of_string(Type *t)
{
    t = resolve_alias(t);
    return type_is_instance_of(t, "Vec") && t->count == 1 && t->args[0] &&
           t->args[0]->kind == TYPE_STRUCT && t->args[0]->name &&
           strcmp(t->args[0]->name, "String") == 0;
}

// The traits `generate_derive_impls` knows how to derive.
static const char *const DERIVABLE_TRAITS[] = {"Clone",    "Copy",   "Debug", "Eq",
                                               "FromJson", "ToJson", NULL};

void check_derivable_trait(Token t, const char *name)
{
    char list[MAX_SHORT_MSG_LEN] = "Derivable traits:";
    for (const char *const *trait = DERIVABLE_TRAITS; *trait; trait++)
    {
        if (strcmp(*trait, name) == 0)
        {
            return;
        }
        size_t used = strlen(list);
        snprintf(list + used, sizeof(list) - used, "%s %s", trait == DERIVABLE_TRAITS ? "" : ",",
                 *trait);
    }
    const char *hints[] = {list, NULL};
    char msg[MAX_SHORT_MSG_LEN];
    snprintf(msg, sizeof(msg), "Unknown trait '%s' in @derive", name);
    zpanic_with_hints(t, msg, hints);
}

// Derived methods on generic types need a generic impl receiver.
static char *derive_self_type(ASTNode *decl, char *name)
{
    char **params = NULL;
    int count = 0;
    if (decl->kind == NODE_STRUCT && decl->strct.is_template)
    {
        params = decl->strct.generic_params;
        count = decl->strct.generic_param_count;
    }
    else if (decl->kind == NODE_ENUM && decl->enm.is_template)
    {
        params = decl->enm.generic_params;
        count = decl->enm.generic_param_count;
    }
    if (count == 0)
    {
        return name;
    }

    Emitter text;
    emitter_init_buffer(&text);
    emitter_printf(&text, "%s<%s", name, params[0]);
    for (int i = 1; i < count; i++)
    {
        emitter_printf(&text, ", %s", params[i]);
    }
    emitter_printf(&text, ">");
    return emitter_take_string(&text);
}

// Type of the `index`-th value a variant carries: its payload, or a field of the
// tuple that holds several values.
static char *variant_value_type(ParserContext *ctx, ASTNode *variant, int index)
{
    Type *payload = variant->variant.payload;
    if (variant->variant.payload_count == 1)
    {
        return type_mangle_spelling(payload);
    }
    ASTNode *tuple = payload && payload->name ? find_struct_def(ctx, payload->name) : NULL;
    ASTNode *field = tuple && tuple->kind == NODE_STRUCT ? tuple->strct.fields : NULL;
    for (int i = 0; field && i < index; i++)
    {
        field = field->next;
    }
    return field ? type_mangle_spelling(field->type_info) : NULL;
}

static void emit_variant_bindings(Emitter *code, int count, char prefix)
{
    for (int i = 0; i < count; i++)
    {
        emitter_printf(code, "%sref %c%d", i ? ", " : "", prefix, i);
    }
}

// Use ref bindings so neither operand is moved. Generic payloads need typed
// locals so a concrete payload type's own eq is found in the instantiated impl.
static void emit_enum_eq_body(Emitter *code, ParserContext *ctx, ASTNode *enm)
{
    const char *name = declared_type_c_name(enm);
    int is_generic = enm->enm.is_template && enm->enm.generic_param_count > 0;
    emitter_printf(code, "if self.tag != other.tag { return false; } match *self { ");
    for (ASTNode *v = enm->enm.variants; v; v = v->next)
    {
        int values = v->variant.payload_count;
        if (v->kind != NODE_ENUM_VARIANT || values == 0)
        {
            continue;
        }

        emitter_printf(code, "%s::%s(", name, v->variant.name);
        emit_variant_bindings(code, values, 'a');
        emitter_printf(code, ") => { match *other { %s::%s(", name, v->variant.name);
        emit_variant_bindings(code, values, 'b');
        emitter_printf(code, ") => { ");

        Emitter comparison;
        emitter_init_buffer(&comparison);
        for (int i = 0; i < values; i++)
        {
            const char *joiner = i ? " && " : "";
            const char *type = is_generic ? variant_value_type(ctx, v, i) : NULL;
            if (type)
            {
                emitter_printf(code, "let x%d: %s* = a%d; let y%d: %s* = b%d; ", i, type, i, i,
                               type, i);
                emitter_printf(&comparison, "%s*x%d == *y%d", joiner, i, i);
            }
            else
            {
                emitter_printf(&comparison, "%s*a%d == *b%d", joiner, i, i);
            }
        }
        emitter_printf(code, "return %s; }, _ => {} } }, ", emitter_take_string(&comparison));
    }
    emitter_printf(code, "_ => {} } return true;");
}

static void emit_clone(Emitter *code, ParserContext *ctx, ASTNode *decl, const char *self_type)
{
    emitter_printf(code, "impl %s { fn clone(self) -> %s { ", self_type, self_type);
    if (decl->kind == NODE_STRUCT)
    {
        emitter_printf(code, "return %s { ", self_type);
        for (ASTNode *field = decl->strct.fields; field; field = field->next)
        {
            if (field->kind != NODE_FIELD || !field->field.name)
            {
                continue;
            }

            Type *type = field->type_info;
            int cloned =
                type && type->kind == TYPE_STRUCT && type->name && type_has_clone_method(ctx, type);
            emitter_printf(code, "%s: self.%s%s, ", field->field.name, field->field.name,
                           cloned ? ".clone()" : "");
        }
        emitter_printf(code, "};");
    }
    else
    {
        emitter_printf(code, "return self;");
    }
    emitter_printf(code, " } }");
}

static void emit_struct_eq_body(Emitter *code, ParserContext *ctx, ASTNode *strct)
{
    int first = 1;
    emitter_printf(code, "return ");
    for (ASTNode *field = strct->strct.fields; field; field = field->next)
    {
        if (field->kind != NODE_FIELD)
        {
            continue;
        }
        const char *name = field->field.name;
        const char *type = type_mangle_spelling(field->type_info);
        if (!first)
        {
            emitter_printf(code, " && ");
        }

        int is_ptr = (field->type_info && field->type_info->kind == TYPE_POINTER) ||
                     (type && strchr(type, '*'));
        ASTNode *definition = is_ptr ? NULL : find_struct_def(ctx, type);
        if (definition && definition->kind == NODE_STRUCT)
        {
            emitter_printf(code, "%s(&self.%s, &other.%s)", mangle_method_symbol(type, NULL, "eq"),
                           name, name);
        }
        else
        {
            emitter_printf(code, "self.%s == other.%s", name, name);
        }
        first = 0;
    }
    if (first)
    {
        emitter_printf(code, "true");
    }
    emitter_printf(code, ";");
}

static void emit_eq(Emitter *code, ParserContext *ctx, ASTNode *decl, const char *self_type)
{
    emitter_printf(code, "impl %s { fn eq(self, other: %s*) -> bool { ", self_type, self_type);
    if (decl->kind == NODE_ENUM)
    {
        ASTNode *variant = decl->enm.variants;
        while (variant && !variant->variant.payload)
        {
            variant = variant->next;
        }

        if (variant)
        {
            emit_enum_eq_body(code, ctx, decl);
        }
        else
        {
            // Comparing plain enum values with == would call this eq again.
            emitter_printf(code, "return (int)*self == (int)*other;");
        }
    }
    else
    {
        emit_struct_eq_body(code, ctx, decl);
    }
    emitter_printf(code, " } }");
}

static void emit_from_json_field(Emitter *code, ASTNode *f)
{
    const char *fn = f->field.name;
    const char *ft = type_mangle_spelling(f->type_info);
    if (!fn || !ft)
    {
        return;
    }
    // Map types to appropriate get_* calls
    // A number: an integer with a sign (bool and char have their own branch).
    int is_int_type = is_signed_type(f->type_info) || is_unsigned_type(f->type_info);
    if (is_int_type)
    {
        // get_int gives an `int`: converted to the field's own width.
        emitter_printf(code, "let _f_%s = (%s)(*j).get_int(\"%s\").unwrap_or(0);\n", fn,
                       type_to_string(f->type_info), fn);
    }
    else if (strcmp(ft, "double") == 0)
    {
        emitter_printf(code, "let _f_%s = (*j).get_float(\"%s\").unwrap_or(0.0);\n", fn, fn);
    }
    else if (strcmp(ft, "bool") == 0)
    {
        emitter_printf(code, "let _f_%s = (*j).get_bool(\"%s\").unwrap_or(false);\n", fn, fn);
    }
    else if (strcmp(ft, "char*") == 0)
    {
        emitter_printf(code, "let _f_%s = (*j).get_string(\"%s\").unwrap_or(\"\");\n", fn, fn);
    }
    else if (strcmp(ft, "String") == 0)
    {
        emitter_printf(code,
                       "let _f_%s = "
                       "String::new((*j).get_string(\"%s\").unwrap_or(\"\"));\n",
                       fn, fn);
    }
    else if (is_vec_of_string(f->type_info))
    {
        emitter_printf(code,
                       "let _f_%s = Vec<String>::new();\n"
                       "let _arr_%s = (*j).get_array(\"%s\");\n"
                       "if _arr_%s.is_some() {\n"
                       "  let _a_%s = _arr_%s.unwrap();\n"
                       "  for let _i_%s: usize = 0; _i_%s < _a_%s.len(); _i_%s = _i_%s + "
                       "1 {\n"
                       "    let _item_%s = _a_%s.at(_i_%s);\n"
                       "    if _item_%s.is_some() {\n"
                       "      let _str_%s = (*_item_%s.unwrap()).as_string();\n"
                       "      if _str_%s.is_some() {\n"
                       "        let _s_%s = String::new(_str_%s.unwrap());\n"
                       "        _f_%s.push(_s_%s); _s_%s.forget();\n"
                       "      }\n"
                       "    }\n"
                       "  }\n"
                       "}\n",
                       fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn,
                       fn, fn, fn, fn);
    }
    else
    {
        // Nested struct: call NestedType::from_json recursively
        emitter_printf(code,
                       "let _opt_%s = (*j).get(\"%s\");\n"
                       "let _f_%s: %s;\n"
                       "if _opt_%s.is_some() { _f_%s = "
                       "%s::from_json(_opt_%s.unwrap()).unwrap(); }\n",
                       fn, fn, fn, ft, fn, fn, ft, fn);
    }
}

static void emit_from_json(Emitter *code, ASTNode *strct, const char *self_type)
{
    emitter_printf(code, "impl %s { fn from_json(j: JsonValue*) -> Result<%s> { ", self_type,
                   self_type);
    for (ASTNode *field = strct->strct.fields; field; field = field->next)
    {
        if (field->kind == NODE_FIELD)
        {
            emit_from_json_field(code, field);
        }
    }

    emitter_printf(code, "return Result<%s>::Ok(%s { ", self_type, self_type);
    const char *separator = "";
    for (ASTNode *field = strct->strct.fields; field; field = field->next)
    {
        if (field->kind != NODE_FIELD)
        {
            continue;
        }

        // The local vector and the returned field need independent ownership.
        const char *clone = is_vec_of_string(field->type_info) ? ".clone()" : "";
        emitter_printf(code, "%s%s: _f_%s%s", separator, field->field.name, field->field.name,
                       clone);
        separator = ", ";
    }
    emitter_printf(code, " });  } }");
}

static void emit_to_json_field(Emitter *code, ASTNode *f)
{
    const char *fn = f->field.name;
    const char *ft = type_mangle_spelling(f->type_info);
    if (!fn || !ft)
    {
        return;
    }
    // A number: an integer with a sign (bool and char have their own branch).
    int is_int_type = is_signed_type(f->type_info) || is_unsigned_type(f->type_info);
    if (is_int_type)
    {
        emitter_printf(code, "_obj.set(\"%s\", JsonValue::number((double)self.%s));\n", fn, fn);
    }
    else if (strcmp(ft, "double") == 0)
    {
        emitter_printf(code, "_obj.set(\"%s\", JsonValue::number(self.%s));\n", fn, fn);
    }
    else if (strcmp(ft, "bool") == 0)
    {
        emitter_printf(code, "_obj.set(\"%s\", JsonValue::bool(self.%s));\n", fn, fn);
    }
    else if (strcmp(ft, "char*") == 0)
    {
        emitter_printf(code, "_obj.set(\"%s\", JsonValue::string(self.%s));\n", fn, fn);
    }
    else if (strcmp(ft, "String") == 0)
    {
        emitter_printf(code, "_obj.set(\"%s\", JsonValue::string(self.%s.c_str()));\n", fn, fn);
    }
    else if (is_vec_of_string(f->type_info))
    {
        emitter_printf(code,
                       "let _arr_%s = JsonValue::array();\n"
                       "for let _i_%s: usize = 0; _i_%s < self.%s.length(); _i_%s = _i_%s "
                       "+ 1 {\n"
                       "  _arr_%s.push(JsonValue::string(self.%s.get(_i_%s).c_str()));\n"
                       "}\n"
                       "_obj.set(\"%s\", _arr_%s);\n",
                       fn, fn, fn, fn, fn, fn, fn, fn, fn, fn, fn);
    }
    else
    {
        // Nested struct: call to_json recursively
        emitter_printf(code, "_obj.set(\"%s\", self.%s.to_json());\n", fn, fn);
    }
}

static void emit_to_json(Emitter *code, ASTNode *strct, const char *self_type)
{
    emitter_printf(code,
                   "impl %s { fn to_json(self) -> JsonValue { let _obj = JsonValue::object();\n",
                   self_type);
    for (ASTNode *field = strct->strct.fields; field; field = field->next)
    {
        if (field->kind == NODE_FIELD)
        {
            emit_to_json_field(code, field);
        }
    }
    emitter_printf(code, "return _obj; } }");
}

ASTNode *generate_derive_impls(ParserContext *ctx, ASTNode *strct, char **traits, int count)
{
    ASTNode *head = NULL, *tail = NULL;
    char *name = declared_type_c_name(strct);
    const char *self_type = derive_self_type(strct, name);

    for (int i = 0; i < count; i++)
    {
        const char *trait = traits[i];
        Emitter code;
        emitter_init_buffer(&code);

        if (strcmp(trait, "Clone") == 0)
        {
            emit_clone(&code, ctx, strct, self_type);
        }
        else if (strcmp(trait, "Eq") == 0)
        {
            emit_eq(&code, ctx, strct, self_type);
        }
        else if (strcmp(trait, "Debug") == 0)
        {
            emitter_printf(&code,
                           "impl %s { fn to_string(self) -> char* { return f\"%s {{ ... }}\"; } }",
                           self_type, name);
        }
        else if (strcmp(trait, "Copy") == 0)
        {
            emitter_printf(&code, "impl Copy for %s {}", name);
        }
        else if (strcmp(trait, "FromJson") == 0)
        {
            if (strct->kind != NODE_STRUCT)
            {
                zwarn_at(strct->token, "@derive(FromJson) only works on structs");
                continue;
            }
            emit_from_json(&code, strct, self_type);
        }
        else if (strcmp(trait, "ToJson") == 0)
        {
            if (strct->kind != NODE_STRUCT)
            {
                zwarn_at(strct->token, "@derive(ToJson) only works on structs");
                continue;
            }
            emit_to_json(&code, strct, self_type);
        }

        char *source = emitter_take_string(&code);
        if (source)
        {
            Lexer tmp;
            lexer_init(&tmp, source, ctx->config, ctx->current_filename);
            ASTNode *impl = parse_impl(ctx, &tmp);
            if (impl)
            {
                if (!head)
                {
                    head = impl;
                }
                else
                {
                    tail->next = impl;
                }
                tail = impl;
            }
        }
    }
    return head;
}
