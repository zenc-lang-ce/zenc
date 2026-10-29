// SPDX-License-Identifier: MIT
// Unit tests of src/driver/driver.c.
#include "driver/driver.h"
#include "diagnostics/diagnostics.h"
#include <stdio.h>
#include "unit_test.h"

UTEST(driver_compile, leaves_no_parser_context_registered)
{
    // A compilation that stops early: the input file does not exist.
    ZenCompiler compiler = {0};
    compiler.config.input_file = "no/such/file.zc";
    compiler.config.quiet = 1;
    ASSERT_EQ(1, driver_compile(&compiler));

    // Its parser context lived on the stack of the finished call: it must not stay registered.
    ASSERT_TRUE(diag_parser_ctx() == NULL);
}

UTEST(driver_compile, does_not_carry_errors_over_from_a_previous_compilation)
{
    char path[] = "/tmp/zc_unit_driver_ok.zc";
    char out[] = "/tmp/zc_unit_driver_ok.c";
    FILE *f = fopen(path, "w");
    ASSERT_TRUE(f != NULL);
    fputs("fn main() -> int {\n    return 0;\n}\n", f);
    fclose(f);

    ZenCompiler compiler = {0};
    compiler.config.input_file = path;
    compiler.config.output_file = out;
    compiler.config.mode_transpile = 1;
    compiler.config.quiet = 1;
    compiler.error_count = 1; // as a previous compilation with an error leaves it
    ASSERT_EQ(0, driver_compile(&compiler));
    ASSERT_EQ(0, compiler.error_count);
}

UTEST(driver_compile, fails_on_an_error_found_while_generating_code)
{
    // A string as the arm of a match statement is reported by codegen (only codegen knows
    // that the match is a statement), as a recoverable error. RFC 0001 makes it an error for
    // one release; when it goes away, this test needs another recoverable codegen error.
    char path[] = "/tmp/zc_unit_driver_codegen_error.zc";
    char out[] = "/tmp/zc_unit_driver_codegen_error.c";
    remove(out);
    FILE *f = fopen(path, "w");
    ASSERT_TRUE(f != NULL);
    fputs("fn main() -> int {\n    let n = 1;\n    match n {\n        _ => \"one\",\n    }\n"
          "    return 0;\n}\n",
          f);
    fclose(f);

    ZenCompiler compiler = {0};
    compiler.config.input_file = path;
    compiler.config.output_file = out;
    compiler.config.mode_transpile = 1;
    compiler.config.quiet = 1;
    ASSERT_EQ(1, driver_compile(&compiler));
    ASSERT_EQ(1, compiler.error_count);

    // The C file with the error is not left behind.
    FILE *c_file = fopen(out, "r");
    ASSERT_TRUE(c_file == NULL);
}
