// SPDX-License-Identifier: MIT
// Unit tests of src/utils/c_headers.c.
#include "utils/c_headers.h"
#include <stdio.h>
#include <string.h>
#include "unit_test.h"

// Writes `text` to `path` and reads it as a header.
static CHeaderFile *header_from(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");
    if (!f)
    {
        return NULL;
    }
    fputs(text, f);
    fclose(f);
    return c_headers_get(path);
}

// The entry for `name` in `header`, or NULL.
static const CHeaderEntry *entry_named(const CHeaderFile *header, const char *name)
{
    for (int i = 0; header && i < header->symbol_count; i++)
    {
        if (strcmp(header->symbols[i].name, name) == 0)
        {
            return &header->symbols[i];
        }
    }
    return NULL;
}

UTEST(c_headers, function_return_types)
{
    CHeaderFile *header =
        header_from("/tmp/zc_unit_c_headers_functions.h",
                    "size_t strlen(const char *s);\n"
                    "extern void *memcpy(void *__restrict dest, const void *src, size_t n);\n"
                    "static inline int clamp(int v) { return v; }\n"
                    "char **split(const char *text,\n"
                    "             char separator);\n"
                    "struct tm *localtime(const time_t *t);\n"
                    "unsigned long long counter(void);\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("size_t", entry_named(header, "strlen")->c_type);
    ASSERT_STREQ("void *", entry_named(header, "memcpy")->c_type);
    ASSERT_STREQ("char **", entry_named(header, "split")->c_type);
    ASSERT_STREQ("struct tm *", entry_named(header, "localtime")->c_type);
    ASSERT_STREQ("unsigned long long", entry_named(header, "counter")->c_type);
}

UTEST(c_headers, attributes_and_system_macros_are_not_types)
{
    CHeaderFile *header =
        header_from("/tmp/zc_unit_c_headers_attributes.h",
                    "__BEGIN_DECLS\n"
                    "extern FILE *fopen(const char *__restrict path, const char *mode) __wur;\n"
                    "__attribute__((noreturn)) void die(const char *message);\n"
                    "int _Nullable maybe(void);\n"
                    "API_AVAILABLE(macos(10.15)) const char *version(void);\n"
                    "__int128 wide(void);\n"
                    "__END_DECLS\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("FILE *", entry_named(header, "fopen")->c_type);
    ASSERT_STREQ("void", entry_named(header, "die")->c_type);
    ASSERT_STREQ("int", entry_named(header, "maybe")->c_type);
    ASSERT_STREQ("const char *", entry_named(header, "version")->c_type);
    ASSERT_STREQ("__int128", entry_named(header, "wide")->c_type);
}

UTEST(c_headers, annotation_macros_before_the_name)
{
    // As in macOS's <malloc/_malloc.h> and <_string.h>.
    CHeaderFile *header =
        header_from("/tmp/zc_unit_c_headers_annotations.h",
                    "void * __sized_by_or_null(__size) malloc(size_t __size) __alloc_size(1);\n"
                    "void *_LIBC_SIZE(__n)\n"
                    "        memcpy(void *_LIBC_SIZE(__n) __dst, const void *__src, size_t __n);\n"
                    "int *__error(void);\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("void *", entry_named(header, "malloc")->c_type);
    ASSERT_STREQ("void *", entry_named(header, "memcpy")->c_type);
    ASSERT_STREQ("int *", entry_named(header, "__error")->c_type);
    ASSERT_TRUE(entry_named(header, "__sized_by_or_null") == NULL);
}

UTEST(c_headers, a_declaration_after_an_inline_body)
{
    // As in glibc's <string.h>, whose C++ overloads have bodies.
    CHeaderFile *header = header_from("/tmp/zc_unit_c_headers_inline.h",
                                      "inline const char *first(const char *s) { return s; }\n"
                                      "char *second(char *s);\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("char *", entry_named(header, "second")->c_type);
}

UTEST(c_headers, extern_variable_types)
{
    CHeaderFile *header =
        header_from("/tmp/zc_unit_c_headers_variables.h", "extern char **environ;\n"
                                                          "extern const int table_size;\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("char **", entry_named(header, "environ")->c_type);
    ASSERT_STREQ("const int", entry_named(header, "table_size")->c_type);
}

UTEST(c_headers, an_annotation_macro_after_a_variable_name)
{
    // macOS's <stdio.h>: the streams' names are followed by an attribute macro.
    CHeaderFile *header = header_from("/tmp/zc_unit_c_headers_annotated_variables.h",
                                      "extern FILE *__stdinp __swift_nonisolated_unsafe;\n"
                                      "extern __darwin_time_t __timezone;\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("FILE *", entry_named(header, "__stdinp")->c_type);
    ASSERT_TRUE(entry_named(header, "__swift_nonisolated_unsafe") == NULL);
    // A reserved name with only a storage word before it is the type, not an annotation.
    ASSERT_STREQ("__darwin_time_t", entry_named(header, "__timezone")->c_type);
}

UTEST(c_headers, typedef_types)
{
    CHeaderFile *header =
        header_from("/tmp/zc_unit_c_headers_typedefs.h", "typedef long __darwin_time_t;\n"
                                                         "typedef __darwin_time_t time_t;\n"
                                                         "typedef __signed char __int8_t;\n"
                                                         "typedef int handler(int);\n"
                                                         "typedef int row[4];\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("long", entry_named(header, "__darwin_time_t")->c_type);
    ASSERT_STREQ("__darwin_time_t", entry_named(header, "time_t")->c_type);
    ASSERT_STREQ("__signed char", entry_named(header, "__int8_t")->c_type);
    // A function type or an array has no plain type to name.
    ASSERT_TRUE(entry_named(header, "handler")->c_type == NULL);
    ASSERT_TRUE(entry_named(header, "row")->c_type == NULL);
}

UTEST(c_headers, macros_that_stand_for_a_name)
{
    CHeaderFile *header = header_from("/tmp/zc_unit_c_headers_macros.h",
                                      "#define stdin __stdinp\n"
                                      "#define stdout (stdout) /* glibc and musl */\n"
                                      "#define BUFSIZ 1024\n"
                                      "#define EOF (-1)\n"
                                      "#define pair a b\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_STREQ("__stdinp", entry_named(header, "stdin")->c_type);
    ASSERT_STREQ("stdout", entry_named(header, "stdout")->c_type);
    ASSERT_TRUE(entry_named(header, "BUFSIZ")->c_type == NULL);
    ASSERT_TRUE(entry_named(header, "EOF")->c_type == NULL);
    ASSERT_TRUE(entry_named(header, "pair")->c_type == NULL);
}

UTEST(c_headers, tags_record_their_keyword)
{
    CHeaderFile *header =
        header_from("/tmp/zc_unit_c_headers_tags.h", "struct vec { int x; };\n"
                                                     "union num { int i; float f; };\n"
                                                     "typedef struct Point { int x; } Point;\n");
    ASSERT_TRUE(header != NULL);
    ASSERT_EQ(C_TAG_STRUCT, entry_named(header, "vec")->tag);
    ASSERT_EQ(C_TAG_UNION, entry_named(header, "num")->tag);
    // The tag comes first; the typedef name of the same spelling has no keyword.
    int tags = 0;
    int names = 0;
    for (int i = 0; i < header->symbol_count; i++)
    {
        if (strcmp(header->symbols[i].name, "Point") == 0)
        {
            tags += header->symbols[i].tag == C_TAG_STRUCT;
            names += header->symbols[i].tag == C_TAG_NONE;
        }
    }
    ASSERT_EQ(1, tags);
    ASSERT_EQ(1, names);
}
