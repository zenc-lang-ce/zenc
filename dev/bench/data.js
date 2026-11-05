window.BENCHMARK_DATA = {
  "lastUpdate": 1793902401545,
  "repoUrl": "https://github.com/zenc-lang-ce/zenc",
  "entries": {
    "Zen Compiler Benchmarks": [
      {
        "commit": {
          "author": {
            "email": "work@petrsmrkovsky.com",
            "name": "Petr Smrkovský",
            "username": "BAGR"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d7e82d7d4e3892bac52801608ecd316b81cac7ff",
          "message": "Merge pull request #47 from zenc-lang-ce/fix/windows-build\n\nWindows: build without libtcc, and no printf format warnings",
          "timestamp": "2026-10-29T08:58:47+01:00",
          "tree_id": "ac3ca6d21b28fee7ff7df72ff1051ce7c5fc4fed",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/d7e82d7d4e3892bac52801608ecd316b81cac7ff"
        },
        "date": 1793260856337,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 18646,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 252,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 150,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 265,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 758,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 612,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 97,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 412,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 155,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "20e7e0799b1ddc2b505f46ca4f6fd2c4d6ace121",
          "message": "Merge pull request #53 from zenc-lang-ce/fix/doc-help-check-flags\n\nzc-doc: print its own help and let `zc help doc` delegate to it",
          "timestamp": "2026-10-29T10:56:46+01:00",
          "tree_id": "67c7139953b2a9f2e82efdafec0614a48fce68fe",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/20e7e0799b1ddc2b505f46ca4f6fd2c4d6ace121"
        },
        "date": 1793267920942,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 13974,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 221,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 74,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 155,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 95,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 238,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 686,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 26,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 117,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 563,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 88,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 380,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 139,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7ab55696905f3846ccf768a4c392df4044c5ba2c",
          "message": "Merge pull request #4 from zenc-lang-ce/fix/fstring-value-overflow\n\nf-string values: never write past the buffer",
          "timestamp": "2026-10-29T12:34:03+01:00",
          "tree_id": "bbb942c03c3e92d2ca68be25b844bbae2ec70e02",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/7ab55696905f3846ccf768a4c392df4044c5ba2c"
        },
        "date": 1793273716970,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 8631,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 158,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 33,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 42,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 77,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 42,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 157,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 435,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 61,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 361,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 32,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 249,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 94,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "507003037451ff4323e923a514560dd8f2989dd1",
          "message": "Merge pull request #49 from zenc-lang-ce/cleanup/remove-rewrite-expr-methods\n\nParser: remove rewrite_expr_methods",
          "timestamp": "2026-10-29T12:34:34+01:00",
          "tree_id": "8fec057770deb727f58a0af91f979758ca2172b1",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/507003037451ff4323e923a514560dd8f2989dd1"
        },
        "date": 1793273795428,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 13008,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 243,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 116,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 89,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 224,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 592,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 104,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 489,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 82,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 333,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 129,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3386ed535828342fc2e254d5673d3e69030a3fc1",
          "message": "Merge pull request #48 from zenc-lang-ce/refactor/parser-dedup\n\nParser: one copy of each duplicated piece",
          "timestamp": "2026-10-29T12:34:56+01:00",
          "tree_id": "0a744c3674588f73b3c9df51ea6c7198f950bcd3",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/3386ed535828342fc2e254d5673d3e69030a3fc1"
        },
        "date": 1793273876314,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 18591,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 223,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 153,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 761,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 631,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 99,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 413,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 154,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "51f6b0ce5d56b7f0210da3ad750c21c05819664c",
          "message": "Merge pull request #2 from zenc-lang-ce/refactor/mangle-methods\n\nMangling: one place for every symbol name",
          "timestamp": "2026-10-29T12:50:09+01:00",
          "tree_id": "42084356e7cfa5208412a5344f19c1f3348d671c",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/51f6b0ce5d56b7f0210da3ad750c21c05819664c"
        },
        "date": 1793274744001,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 19118,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 240,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 154,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 271,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 785,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 122,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 622,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 415,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 154,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "594af4967ae6d232ed46c19c3520dce77f46d6ac",
          "message": "Merge pull request #5 from zenc-lang-ce/feature/fstring-only-warning\n\nOnly f\"...\" interpolates (RFC 0001, part 1)",
          "timestamp": "2026-10-29T12:56:59+01:00",
          "tree_id": "024ad713362f101e66938f77550778a1bbe58f0c",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/594af4967ae6d232ed46c19c3520dce77f46d6ac"
        },
        "date": 1793275116202,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 12448,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 200,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 52,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 85,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 52,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 200,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 603,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 502,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 340,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 11,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 128,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "306f8d11dbe5a0a3bcfe70c9934de385c3582dff",
          "message": "Merge pull request #61 from zenc-lang-ce/feature/fstring-only-warning\n\nprint and println are functions of std/io.zc, and f-strings are built in codegen (RFC 0001, part 2)",
          "timestamp": "2026-10-29T14:26:21+01:00",
          "tree_id": "b5e6e288f2384ca8c5baa8808da6d857790835e9",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/306f8d11dbe5a0a3bcfe70c9934de385c3582dff"
        },
        "date": 1793280514442,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 19154,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 193,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 110,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 86,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 223,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 592,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 99,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 486,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 82,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 332,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 127,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c528dd07d13bbc158c4906a4aabcf3f25fb92c99",
          "message": "Merge pull request #63 from zenc-lang-ce/fix/enum-link-name\n\nEnums with @link_name: use the C name everywhere",
          "timestamp": "2026-10-29T17:28:21+01:00",
          "tree_id": "ad7c4cb131962a98012c162eaaf73ee26ffa7b51",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/c528dd07d13bbc158c4906a4aabcf3f25fb92c99"
        },
        "date": 1793291483182,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 27636,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 227,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 104,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 270,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 755,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 139,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 614,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 99,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 411,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 151,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e046eac7551fec4aa0ee47805bb72f80dece91af",
          "message": "Merge pull request #55 from zenc-lang-ce/fix/misra-match-twice\n\ntypecheck: report MISRA Rule 16.4 once for a match without a default arm",
          "timestamp": "2026-10-30T05:30:40+01:00",
          "tree_id": "1184f19f22f66309977f450b9a29315f6e2058a2",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/e046eac7551fec4aa0ee47805bb72f80dece91af"
        },
        "date": 1793334772820,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 19223,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 214,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 108,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 85,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 222,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 585,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 99,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 502,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 80,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 325,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 123,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a4874320cf5b0e656b95fc495e511886d5e378bd",
          "message": "Merge pull request #56 from zenc-lang-ce/fix/zc-help-options\n\ncmd: list only the options zc parses in its help",
          "timestamp": "2026-10-30T05:31:03+01:00",
          "tree_id": "f52587cabbca37b3a12dd6b5f1d0aac1e9240425",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/a4874320cf5b0e656b95fc495e511886d5e378bd"
        },
        "date": 1793334787477,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 15093,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 161,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 45,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 35,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 73,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 73,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 163,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 453,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 15,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 365,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 32,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 100,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "37e458fbf41a77a269aedfc03fd38f80850ac7d3",
          "message": "Merge pull request #57 from zenc-lang-ce/fix/impl-vtables-limit\n\ncodegen: let emit_impl_vtables track any number of trait implementations",
          "timestamp": "2026-10-30T05:31:24+01:00",
          "tree_id": "c5c8f311e6723cc14a2b2b235975dacc1c5edd86",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/37e458fbf41a77a269aedfc03fd38f80850ac7d3"
        },
        "date": 1793334856038,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 15436,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 173,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 51,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 36,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 79,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 167,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 508,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 17,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 421,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 278,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 107,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0b39f1ee3134d84f965bf588d75501875796c66e",
          "message": "Merge pull request #58 from zenc-lang-ce/fix/slice-literal\n\nparser: fix the crash and the invalid C of slice literals",
          "timestamp": "2026-10-30T05:31:51+01:00",
          "tree_id": "78ab03d0d6b35d904f8e92d352e5d4acf7d73575",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/0b39f1ee3134d84f965bf588d75501875796c66e"
        },
        "date": 1793334947988,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 24634,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 234,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 139,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 136,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 287,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 758,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 25,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 128,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 613,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 420,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 18,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 158,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a5744c611364377d42b42735183345772fd623ef",
          "message": "Merge pull request #64 from zenc-lang-ce/fix/index-overload-rule\n\nIndex operator: `x[i]` calls only `get`",
          "timestamp": "2026-10-30T05:32:19+01:00",
          "tree_id": "0cb47c9d2f2c7fe45f15b22db5a1a83dcda64d8c",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/a5744c611364377d42b42735183345772fd623ef"
        },
        "date": 1793335086418,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 20451,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 185,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 108,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 86,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 225,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 585,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 492,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 113,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 326,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 123,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "16a88a84efbe6384c93af526169933972556b1d7",
          "message": "Merge pull request #66 from zenc-lang-ce/fix/index-on-temporary\n\nIndex a temporary with `get` through a temporary variable",
          "timestamp": "2026-10-30T05:33:36+01:00",
          "tree_id": "e1ad9ad618f2fb80c82575296bb23d8c72f4c7c5",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/16a88a84efbe6384c93af526169933972556b1d7"
        },
        "date": 1793335159110,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 28114,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 252,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 149,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 108,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 783,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 630,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 431,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 161,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "380564319382326d64cb205ed98e9ec2a724e235",
          "message": "Merge pull request #54 from zenc-lang-ce/fix/repl-plot-use\n\nREPL: fix `:plot` and let zc-repl parse its own options and help",
          "timestamp": "2026-10-30T05:36:00+01:00",
          "tree_id": "c3c01078cfe8d6623ee6679a0866fef2ebce13d3",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/380564319382326d64cb205ed98e9ec2a724e235"
        },
        "date": 1793335164683,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 27704,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 243,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 766,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 615,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 412,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 154,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "159272512de13aa6fe67f830724f1173aed8f46b",
          "message": "Merge pull request #67 from zenc-lang-ce/fix/global-multi-let\n\nTop-level variables: every variable of a multi-let, and static, @thread_local and @cfg",
          "timestamp": "2026-10-30T05:37:28+01:00",
          "tree_id": "4a28a037b4fd31de45d163dd484e6ed6ce9ec143",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/159272512de13aa6fe67f830724f1173aed8f46b"
        },
        "date": 1793335297144,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 27262,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 246,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 152,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 265,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 757,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 613,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 404,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 152,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ae58d70c2b9f72ce270fb75c52281e1bc6e6ca8f",
          "message": "Merge pull request #68 from zenc-lang-ce/fix/eq-untyped-local-scope\n\ncodegen: keep the locals of a block out of the other functions",
          "timestamp": "2026-10-30T05:45:08+01:00",
          "tree_id": "077d7f276ff39a2fa999c3db2ba37423b15e43b4",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/ae58d70c2b9f72ce270fb75c52281e1bc6e6ca8f"
        },
        "date": 1793335673908,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 25604,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 227,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 106,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 748,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 603,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 408,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 154,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "84138083af07c29410d87c36eca6db20ae5b837d",
          "message": "Merge pull request #70 from zenc-lang-ce/refactor/rvalue-address\n\ncodegen: name the type of a temporary that needs an address",
          "timestamp": "2026-10-30T06:24:37+01:00",
          "tree_id": "73fb0f9b1ecf9cf7bda32f9c3f9bf3673cf158a4",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/84138083af07c29410d87c36eca6db20ae5b837d"
        },
        "date": 1793338042652,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 25140,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 251,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 740,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 601,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 403,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 150,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6a7b1e371a46ea1f2435cd8667202cac71873e10",
          "message": "Merge pull request #71 from zenc-lang-ce/fix/windows-build-tools\n\nbuild: build the standalone tools with build.bat and CMake",
          "timestamp": "2026-10-30T06:27:38+01:00",
          "tree_id": "faa7e9d33673ce0014d5c385ba033646d6ba8831",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/6a7b1e371a46ea1f2435cd8667202cac71873e10"
        },
        "date": 1793338183759,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 18154,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 194,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 52,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 79,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 179,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 580,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 51,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 478,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 51,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 77,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 317,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 10,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 118,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b321424b899f6771e7e44bf86967798d3a641db5",
          "message": "Merge pull request #72 from zenc-lang-ce/fix/cpp-rvalue-lifetime\n\ncodegen: keep a temporary alive during the method call in C++",
          "timestamp": "2026-10-30T06:55:52+01:00",
          "tree_id": "e085d7daf7ec27a6eb81261ede3d21efc38fa50e",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/b321424b899f6771e7e44bf86967798d3a641db5"
        },
        "date": 1793339854130,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 14349,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 158,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 37,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 80,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 74,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 162,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 494,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 17,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 386,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 9,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 103,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5b8b04130864d38c318905dcf76ac0b9954239d3",
          "message": "Merge pull request #73 from zenc-lang-ce/fix/generic-link-name\n\nparser: give generic instances, casts and sizeof the @link_name of their types",
          "timestamp": "2026-10-30T07:22:16+01:00",
          "tree_id": "55b0114ada52a2e30474b24734f002ccf03e7934",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/5b8b04130864d38c318905dcf76ac0b9954239d3"
        },
        "date": 1793341480212,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 13511,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 33,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 83,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 155,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 436,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 45,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 61,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 396,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 33,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 262,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 97,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ea1eb8ce8e5c478f36db0e79806d5b4ab48af655",
          "message": "Merge pull request #74 from zenc-lang-ce/fix/if-expr-string\n\nparser, codegen: let a string literal be the value of an if expression",
          "timestamp": "2026-10-30T07:38:00+01:00",
          "tree_id": "05ef65da22edd3d7db8d7a0e85d91437e82bc326",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/ea1eb8ce8e5c478f36db0e79806d5b4ab48af655"
        },
        "date": 1793342453690,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 26995,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 245,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 150,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 106,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 278,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 758,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 123,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 628,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 414,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 155,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ba9b6094b2a6c480e3ed74920c9b030c7b32d808",
          "message": "Merge pull request #75 from zenc-lang-ce/fix/lsp-string-id\n\nlsp: answer each request with its id as the client sent it",
          "timestamp": "2026-10-30T10:14:24+01:00",
          "tree_id": "5810c0d91f6110ccda2b41b713f83eba959ac3b5",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/ba9b6094b2a6c480e3ed74920c9b030c7b32d808"
        },
        "date": 1793351832822,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 25989,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 234,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 269,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 732,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 608,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 96,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 401,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 151,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7a0805708e49abef23e71c285051c5fc62517f80",
          "message": "Merge pull request #77 from zenc-lang-ce/fix/link-name-pass-instances\n\nparser: walk only the new generic instances in each link_name pass",
          "timestamp": "2026-10-30T10:15:10+01:00",
          "tree_id": "18089e571f858d269344f16f96dfe23ba95f37f1",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/7a0805708e49abef23e71c285051c5fc62517f80"
        },
        "date": 1793351858694,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 17463,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 174,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 60,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 48,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 89,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 82,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 160,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 521,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 45,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 449,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 318,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 9,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 119,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "cc8a57221ee07d7a1879621784834769f6f269a1",
          "message": "Merge pull request #76 from zenc-lang-ce/fix/lsp-recursion-depth\n\nparser: lower the recursion depth on every return of the type parsers",
          "timestamp": "2026-10-30T10:14:51+01:00",
          "tree_id": "5b75634ce9553b9646f9513525e13b6f5c42e2d3",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/cc8a57221ee07d7a1879621784834769f6f269a1"
        },
        "date": 1793351887282,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 26304,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 232,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 152,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 265,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 752,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 606,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 405,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 153,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "27fb30df5e80befa5a50a8401e1545dd6790fd70",
          "message": "Merge pull request #78 from zenc-lang-ce/feature/lsp-index-progress\n\nlsp: report the progress of indexing the workspace",
          "timestamp": "2026-10-30T10:25:26+01:00",
          "tree_id": "0243b0c19f98a241a6cbe7777dc7b57c97312350",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/27fb30df5e80befa5a50a8401e1545dd6790fd70"
        },
        "date": 1793352496544,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 26330,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 225,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 149,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 749,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 618,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 409,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 152,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "30d0508f36a7f2aef03617168301b8214922cf77",
          "message": "Merge pull request #79 from zenc-lang-ce/feature/lsp-exit-orphaned\n\nlsp: stop indexing when the editor has exited",
          "timestamp": "2026-10-30T10:27:26+01:00",
          "tree_id": "896d827be68c909f050d790544699bef431ebe13",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/30d0508f36a7f2aef03617168301b8214922cf77"
        },
        "date": 1793352607102,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 19882,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 199,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 110,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 87,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 226,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 594,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 492,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 83,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 332,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 126,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a95e66247da789551bc7e5b1941c201da02c9531",
          "message": "Merge pull request #80 from zenc-lang-ce/perf/scope-index\n\nparser: index the symbol lookups that walked lists of the whole program",
          "timestamp": "2026-10-30T12:36:55+01:00",
          "tree_id": "9988172451d09bb12d4e3df7b2d818acf7f2094b",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/a95e66247da789551bc7e5b1941c201da02c9531"
        },
        "date": 1793360285096,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 7386,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 145,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 48,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 36,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 78,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 78,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 166,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 416,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 17,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 344,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 247,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 9,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 101,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ca499645d799a450ff1079acd778595f4ecc53f5",
          "message": "Merge pull request #81 from zenc-lang-ce/fix/semantic-tokens-in-text\n\nlsp: take keyword tokens from the lexer, not from every word of the text",
          "timestamp": "2026-10-30T13:15:27+01:00",
          "tree_id": "379e40be4c6ec4f07d3868fcc3af71795f3d12c8",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/ca499645d799a450ff1079acd778595f4ecc53f5"
        },
        "date": 1793362628986,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 12010,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 225,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 150,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 108,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 265,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 654,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 567,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 391,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 158,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3f276f67c19345a7858dce1917b4bd96a3ac0b8d",
          "message": "Merge pull request #82 from zenc-lang-ce/fix/lsp-windows-binary-stdio\n\nlsp: read and write the protocol in binary mode on Windows",
          "timestamp": "2026-10-30T13:24:46+01:00",
          "tree_id": "996d980d30490c8ca4ccc1759045845906da6f22",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/3f276f67c19345a7858dce1917b4bd96a3ac0b8d"
        },
        "date": 1793363190072,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11016,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 217,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 152,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 625,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 524,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 370,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 148,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f0a3648965076f216cdba4c3202a05eff922ee17",
          "message": "Merge pull request #83 from zenc-lang-ce/fix/tuple-signature-overflow\n\nparser: build a tuple's signature as long as its element types need",
          "timestamp": "2026-10-30T19:45:09+01:00",
          "tree_id": "068718986a74a2cd6d8b28761966d41bf39c2e84",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/f0a3648965076f216cdba4c3202a05eff922ee17"
        },
        "date": 1793385985965,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 8404,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 173,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 51,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 112,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 86,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 226,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 496,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 419,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 83,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 295,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 120,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5a850abf449d05b69bf53bcfc8e893b9fb2702ed",
          "message": "Merge pull request #87 from zenc-lang-ce/fix/windows-console-codepage\n\nplatform: restore the console code page when zc exits on Windows",
          "timestamp": "2026-10-30T19:52:54+01:00",
          "tree_id": "d8d3e434e1e158f66bfcaa608327b6851838d228",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/5a850abf449d05b69bf53bcfc8e893b9fb2702ed"
        },
        "date": 1793386465333,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10962,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 196,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 126,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 109,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 87,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 224,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 545,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 452,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 83,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 297,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 124,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6dc652e142e22cf584ba7e2b58203f6f03a5363c",
          "message": "Merge pull request #89 from zenc-lang-ce/fix/lsp-member-completion\n\nlsp: complete the methods of a type after `.` and `::`",
          "timestamp": "2026-10-30T21:11:35+01:00",
          "tree_id": "b4ffc48c806d2c3f743ea3c7c2eaf3ebf21adc8d",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/6dc652e142e22cf584ba7e2b58203f6f03a5363c"
        },
        "date": 1793391181824,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9878,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 175,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 109,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 86,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 224,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 522,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 435,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 82,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 300,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 122,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6dc92ca48d8f7a951d8f9340c612b8e4fe300090",
          "message": "Merge pull request #90 from zenc-lang-ce/fix/const-fold-ub\n\nanalysis: remove the undefined behavior from constant folding",
          "timestamp": "2026-10-31T07:54:14+01:00",
          "tree_id": "bf66f3edf8f0be5f085840196e6bc81c97e968bc",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/6dc92ca48d8f7a951d8f9340c612b8e4fe300090"
        },
        "date": 1793429728881,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 7495,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 162,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 48,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 159,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 440,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 337,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 32,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 244,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 102,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5240a8e84ec1741854744dfa064486717533926d",
          "message": "Merge pull request #91 from zenc-lang-ce/fix/readln-string\n\nparser: read a string with readln(...) into a new string",
          "timestamp": "2026-10-31T07:54:33+01:00",
          "tree_id": "ef97288228fb1276907be63021795082b9dbe5f0",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/5240a8e84ec1741854744dfa064486717533926d"
        },
        "date": 1793429777882,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 8721,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 258,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 51,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 106,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 83,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 192,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 523,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 18,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 107,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 467,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 52,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 320,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 12,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 123,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a11a222e4d7b98d53152ca158620c571e4fa67ed",
          "message": "Merge pull request #92 from zenc-lang-ce/fix/run-program-args\n\ndriver: pass the arguments after the file to the program of zc run",
          "timestamp": "2026-10-31T07:55:01+01:00",
          "tree_id": "4d94a5fb9125645fa133f8b8ebcb0495c49322ae",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/a11a222e4d7b98d53152ca158620c571e4fa67ed"
        },
        "date": 1793429814304,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10428,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 334,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 112,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 85,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 52,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 225,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 498,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 425,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 390,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 122,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f0bd024569bbf05f274ea42bec61f6fe287e59ad",
          "message": "Merge pull request #93 from zenc-lang-ce/fix/reject-struct-patterns\n\nparser: reject struct patterns in match",
          "timestamp": "2026-10-31T07:55:23+01:00",
          "tree_id": "f33c5c24d09191b3bbaa93d961f8a01ef0c5098b",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/f0bd024569bbf05f274ea42bec61f6fe287e59ad"
        },
        "date": 1793429833963,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11162,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 220,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 149,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 264,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 640,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 538,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 381,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 153,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d04c1726097e2e2bd0f01dfc2dd4243fcd67d06c",
          "message": "Merge pull request #95 from zenc-lang-ce/fix/check-skip-std-errors\n\ntypecheck: do not report errors inside the standard library",
          "timestamp": "2026-10-31T08:23:14+01:00",
          "tree_id": "f7a709b11769d5323254a3a427090440e861829b",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/d04c1726097e2e2bd0f01dfc2dd4243fcd67d06c"
        },
        "date": 1793431487415,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11279,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 220,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 147,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 106,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 270,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 644,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 535,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 371,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 152,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3427fe3e683dbbc7ccafb0ff556cbb9c76644bec",
          "message": "Merge pull request #94 from zenc-lang-ce/feature/fat-arrow-lambdas\n\nparser: lambdas and match arms take =>, -> is member access",
          "timestamp": "2026-10-31T08:23:31+01:00",
          "tree_id": "06d826a3e632e4cd7c85f6675d3a09d7d29d613a",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/3427fe3e683dbbc7ccafb0ff556cbb9c76644bec"
        },
        "date": 1793431534115,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11080,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 238,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 629,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 533,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 97,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 370,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 148,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "100406e8ae1f903fe5891f8b51511521c216d27c",
          "message": "Merge pull request #96 from zenc-lang-ce/docs/changelog-0.5\n\ndocs: bring the 0.5.0 changelog up to date",
          "timestamp": "2026-10-31T08:23:50+01:00",
          "tree_id": "22af53f14945723e10e875ebd388e2c06c08fcc7",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/100406e8ae1f903fe5891f8b51511521c216d27c"
        },
        "date": 1793431557163,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9189,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 185,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 109,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 85,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 227,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 500,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 418,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 80,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 295,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 127,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a60e55edebd68ebdc927d4d8101448139c5f9ef6",
          "message": "Merge pull request #97 from zenc-lang-ce/fix/lsp-signature-zen-types\n\nlsp: show a function's signature as written in completion and signature help",
          "timestamp": "2026-10-31T08:39:38+01:00",
          "tree_id": "e0df401822c095d1ec0c52eca39e082e6f626e75",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/a60e55edebd68ebdc927d4d8101448139c5f9ef6"
        },
        "date": 1793432506863,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11598,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 228,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 151,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 106,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 269,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 639,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 549,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 99,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 372,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 152,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "fcaae910ceabe79bfd6eb3a4c00aa7e9a62033fe",
          "message": "Merge pull request #98 from zenc-lang-ce/fix/param-symbol-position\n\nparser: register a function's parameters with their name token",
          "timestamp": "2026-11-01T07:47:25+01:00",
          "tree_id": "6e484ee3774af6e11240d06bd6c75484620261e2",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/fcaae910ceabe79bfd6eb3a4c00aa7e9a62033fe"
        },
        "date": 1793515746481,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 8746,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 191,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 48,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 77,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 170,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 491,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 15,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 78,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 414,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 73,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 283,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 10,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 114,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7ccb7a8ae7af756e8d86ac02ace4467a6aeea6d9",
          "message": "Merge pull request #99 from zenc-lang-ce/feature/lsp-c-struct-fields\n\nlsp: complete the fields of a struct from a C header",
          "timestamp": "2026-11-01T07:53:34+01:00",
          "tree_id": "8a88fb3045f8cc29fe308743d367016da72795f6",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/7ccb7a8ae7af756e8d86ac02ace4467a6aeea6d9"
        },
        "date": 1793516122217,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11602,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 232,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 147,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 627,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 531,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 97,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 378,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 149,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8d96cf92192ac9825814eb648b79bd42f1531042",
          "message": "Merge pull request #100 from zenc-lang-ce/fix/closure-call-checks\n\ntypecheck: check the arguments and the unused parameters of closures",
          "timestamp": "2026-11-01T07:59:43+01:00",
          "tree_id": "f8eb88dc414331755ce9cac61ed13614763a8e19",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/8d96cf92192ac9825814eb648b79bd42f1531042"
        },
        "date": 1793516483993,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11691,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 239,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 149,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 107,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 274,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 629,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 542,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 378,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 150,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "80eaa3fff03b8e110140b88dc9b73bb61c4ae6ae",
          "message": "Merge pull request #101 from zenc-lang-ce/docs/changelog-highlights\n\ndocs: keep the most significant changes in CHANGELOG.md",
          "timestamp": "2026-11-01T08:12:09+01:00",
          "tree_id": "7337334626f52b0e67cfdc2d42e347186d73390d",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/80eaa3fff03b8e110140b88dc9b73bb61c4ae6ae"
        },
        "date": 1793517227059,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 12110,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 224,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 149,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 107,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 645,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 549,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 381,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 155,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f7ee40f23563cd8162fc6389105d8e230107c978",
          "message": "Merge pull request #105 from zenc-lang-ce/docs/new-org-links\n\ndocs: link the READMEs to the zenc-lang-ce repositories and site",
          "timestamp": "2026-11-02T06:09:54+01:00",
          "tree_id": "de660bc57b02969ba95a3e4ca16dc49f5af0d078",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/f7ee40f23563cd8162fc6389105d8e230107c978"
        },
        "date": 1793596276489,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 6836,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 142,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 35,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 73,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 153,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 377,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 61,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 325,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 33,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 238,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 98,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a75e65025c40f9ff1b1e10d1a30941311569cb02",
          "message": "Merge pull request #104 from zenc-lang-ce/site/single-zola-site\n\nci: bump the website version only in its configuration",
          "timestamp": "2026-11-02T06:09:36+01:00",
          "tree_id": "7fd4e7a9868a7b6b8e97c1280db88f8826c0af72",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/a75e65025c40f9ff1b1e10d1a30941311569cb02"
        },
        "date": 1793596290587,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11860,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 216,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 663,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 534,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 99,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 371,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 151,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2bec5703f408eb6354ecbe6bbf563711ee033ecf",
          "message": "Merge pull request #108 from zenc-lang-ce/docs/repo-link-names\n\ndocs: repository names in the README links, changelog link and security contact",
          "timestamp": "2026-11-02T06:59:38+01:00",
          "tree_id": "8bf2fe43a6602ddfb8d9865e372b597cbfda9d59",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/2bec5703f408eb6354ecbe6bbf563711ee033ecf"
        },
        "date": 1793599274623,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10806,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 214,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 60,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 77,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 124,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 94,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 219,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 605,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 61,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 121,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 513,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 91,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 353,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 13,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 139,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "21a12578b9fd7be099b3b1e0198126997b936bd4",
          "message": "Merge pull request #111 from zenc-lang-ce/fix/variant-constructor-missing-args\n\nparser: report a variant constructor called without its payload instead of crashing",
          "timestamp": "2026-11-03T06:06:32+01:00",
          "tree_id": "78191d7874ac40daf66f49095fed2c9037494ccf",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/21a12578b9fd7be099b3b1e0198126997b936bd4"
        },
        "date": 1793682488388,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11753,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 220,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 153,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 647,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 541,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 377,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 153,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8c3eda01c42f8b73fbaa508524f2cd6e9f9bdb4f",
          "message": "Merge pull request #112 from zenc-lang-ce/fix/zc-root-env\n\nzc.com: import the bundled standard library from any directory",
          "timestamp": "2026-11-03T06:23:32+01:00",
          "tree_id": "5d918641857d787946339483127726cd920a2976",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/8c3eda01c42f8b73fbaa508524f2cd6e9f9bdb4f"
        },
        "date": 1793683505270,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10047,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 190,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 72,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 110,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 88,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 52,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 204,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 560,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 18,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 113,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 471,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 84,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 312,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 11,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 127,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5f4ed1aeee9d61c1f9e84b48b68f7aaa0119f9cf",
          "message": "Merge pull request #114 from zenc-lang-ce/ci/ape-release-warnings\n\nci: APE build without warnings, a working cosmocc cache and Node 24 actions",
          "timestamp": "2026-11-03T06:25:14+01:00",
          "tree_id": "f4c8203a75d7e4989f5ec5dd1e44cab1bb6c4df7",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/5f4ed1aeee9d61c1f9e84b48b68f7aaa0119f9cf"
        },
        "date": 1793683606780,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11026,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 214,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 101,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 621,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 121,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 519,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 96,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 364,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 148,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c5280ec1fcfd3caa7804cd7727a3b907fc337c23",
          "message": "Merge pull request #113 from zenc-lang-ce/fix/zc-boot-downloads\n\nape: make zc-boot.com install this project's zc.com",
          "timestamp": "2026-11-03T06:25:40+01:00",
          "tree_id": "886e1b3db6210d0fd609be42fe395e15ba2da77d",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/c5280ec1fcfd3caa7804cd7727a3b907fc337c23"
        },
        "date": 1793683655344,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 8233,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 156,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 60,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 92,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 73,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 161,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 460,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 388,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 10,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 107,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2b2d2eb58b18a51918abe46803ea8ced4d697de6",
          "message": "Merge pull request #115 from zenc-lang-ce/docs/changelog-0.5.1\n\ndocs: add 0.5.1 to CHANGELOG.md",
          "timestamp": "2026-11-03T07:58:00+01:00",
          "tree_id": "7dcb345fd83ed4c6aef9960e263db0ab0ffdf3b8",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/2b2d2eb58b18a51918abe46803ea8ced4d697de6"
        },
        "date": 1793689204341,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11424,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 218,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 151,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 630,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 537,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 99,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 373,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 152,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "78bab2b17853d0b6834b8f6822bea788422b67b4",
          "message": "Merge pull request #116 from zenc-lang-ce/ci/prerelease-tags\n\nci: publish a tag with a suffix as a pre-release",
          "timestamp": "2026-11-03T08:04:09+01:00",
          "tree_id": "0b3e7e6e787be8722edac62b18d08691ef636556",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/78bab2b17853d0b6834b8f6822bea788422b67b4"
        },
        "date": 1793689545981,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11683,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 232,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 150,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 104,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 269,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 638,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 532,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 371,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 151,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c43f458cd0e65e1a3cf1b450bc51b512e55c1fe1",
          "message": "Merge pull request #119 from zenc-lang-ce/ci/native-release-binaries\n\nrelease: native binaries for each system and the tools as APE",
          "timestamp": "2026-11-03T09:01:58+01:00",
          "tree_id": "b182665a86f458570f639308bdfae55cd2bf5913",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/c43f458cd0e65e1a3cf1b450bc51b512e55c1fe1"
        },
        "date": 1793693010590,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10904,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 215,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 147,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 620,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 522,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 96,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 367,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 149,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6fccc9788c9b049dbda881356fea0957b63ac442",
          "message": "Merge pull request #121 from zenc-lang-ce/ci/linux-tar-gz\n\nrelease: pack the Linux binaries as tar.gz",
          "timestamp": "2026-11-03T14:50:18+01:00",
          "tree_id": "f2404694562d4d50f3be4d9ace83b911aea453d8",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/6fccc9788c9b049dbda881356fea0957b63ac442"
        },
        "date": 1793713932522,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11606,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 229,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 630,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 119,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 539,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 372,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 149,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "9e365797a7a8fe48cfe8acbfcc62a4e5867bae88",
          "message": "Merge pull request #122 from zenc-lang-ce/typecheck/match-exhaustiveness\n\ntypecheck: one exhaustiveness check for match, on patterns kept as written",
          "timestamp": "2026-11-04T16:52:54+01:00",
          "tree_id": "32fad1796a663a160cc774df35a13ac369dfbd27",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/9e365797a7a8fe48cfe8acbfcc62a4e5867bae88"
        },
        "date": 1793807665823,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 7393,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 154,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 59,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 41,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 89,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 43,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 159,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 447,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 378,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 258,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 10,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 103,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8588c3268c4954979fe4b0f79e4e2710bb6b4d76",
          "message": "Merge pull request #123 from zenc-lang-ce/typecheck/enum-variant-expressions\n\ntypecheck: accept Enum::Variant() for a variant without a payload",
          "timestamp": "2026-11-05T05:53:42+01:00",
          "tree_id": "666932651c60317fe859d3f435e8fb5a214f2fb8",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/8588c3268c4954979fe4b0f79e4e2710bb6b4d76"
        },
        "date": 1793854518343,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9892,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 194,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 109,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 85,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 222,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 506,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 439,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 300,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 123,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f99d6e12cb70cb2ed628dea07369c703666925af",
          "message": "Merge pull request #124 from zenc-lang-ce/typecheck/type-mismatch\n\ntypecheck: returns in lambdas, Self literals and casts to @link_name structs",
          "timestamp": "2026-11-05T05:57:55+01:00",
          "tree_id": "a80fe6149bd613e7ca8595a87e079aacd0da2f3e",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/f99d6e12cb70cb2ed628dea07369c703666925af"
        },
        "date": 1793855698650,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 7641,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 154,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 60,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 90,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 161,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 458,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 45,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 385,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 46,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 262,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 10,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 106,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5e3a26d8904b55c54e6960c21dc67134b82f9224",
          "message": "Merge pull request #125 from zenc-lang-ce/refactor/source-type-names\n\nparser, codegen: keep types in the AST only as a Type with source names",
          "timestamp": "2026-11-05T05:58:24+01:00",
          "tree_id": "9adcbcb6dd883c758de15d343d9960a5a8a07690",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/5e3a26d8904b55c54e6960c21dc67134b82f9224"
        },
        "date": 1793857075353,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9042,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 216,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 265,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 546,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 120,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 509,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 96,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 358,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 146,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "bd18e053be2bef50ffceb8c79b42b85bef1684e7",
          "message": "Merge pull request #142 from zenc-lang-ce/fix/ci-windows-std-tsan\n\ntypecheck, utils, codegen: fix the Windows, ThreadSanitizer, analyzer and C++ jobs on main",
          "timestamp": "2026-11-05T07:02:40+01:00",
          "tree_id": "b9616de5f7178e30558db99f30c6c5107ed99bfb",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/bd18e053be2bef50ffceb8c79b42b85bef1684e7"
        },
        "date": 1793858656063,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11099,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 206,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 79,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 61,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 122,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 97,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 60,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 220,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 550,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 103,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 461,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 94,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 356,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 13,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 143,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "9c3b3b9d62a1845ad0e2ffa9921452d2820de101",
          "message": "Merge pull request #137 from zenc-lang-ce/refactor/instance-cleanup\n\ntypecheck, parser, codegen: answer questions about a type from its Type, not its spelling",
          "timestamp": "2026-11-05T07:58:39+01:00",
          "tree_id": "fa27827ab18cced7cd37a1613e55139037411caa",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/9c3b3b9d62a1845ad0e2ffa9921452d2820de101"
        },
        "date": 1793862005602,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 204,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 73,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 59,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 158,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 96,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 59,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 246,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 534,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 26,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 61,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 111,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 453,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 92,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 350,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 139,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c50620f95eb43b1c4dfca999e999786fcc48b464",
          "message": "Merge pull request #139 from zenc-lang-ce/refactor/c-spelling\n\nast, parser, codegen: write C from C spellings, not the spelling for people",
          "timestamp": "2026-11-05T07:59:15+01:00",
          "tree_id": "5c1d3d6f5d44ca8a11b51d3727db24932c0273ef",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/c50620f95eb43b1c4dfca999e999786fcc48b464"
        },
        "date": 1793862078110,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11121,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 223,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 73,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 140,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 109,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 282,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 581,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 25,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 72,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 125,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 493,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 383,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 18,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 157,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "630fbdcf9f7e0424d868200374871cecf2ada776",
          "message": "Merge pull request #140 from zenc-lang-ce/refactor/zen-primitive-names\n\nast, codegen: name primitives as the program writes them in messages and reflection",
          "timestamp": "2026-11-05T09:18:19+01:00",
          "tree_id": "0fc667221eba66484fa078eee8cc370f41600cba",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/630fbdcf9f7e0424d868200374871cecf2ada776"
        },
        "date": 1793866790563,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9955,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 290,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 59,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 109,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 89,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 220,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 471,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 20,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 97,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 432,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 84,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 300,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 123,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0056757d6a5dfcb7598483d4d3fcd8a2ed7ca339",
          "message": "Merge pull request #144 from zenc-lang-ce/fix/primitive-zen-name-const\n\nast: mark primitive_zen_name const for the sanitizer builds",
          "timestamp": "2026-11-05T10:16:54+01:00",
          "tree_id": "66dafae80a6c8f801bf57cb4c868f9b4fd2d0bea",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/0056757d6a5dfcb7598483d4d3fcd8a2ed7ca339"
        },
        "date": 1793870302341,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9833,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 177,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 110,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 87,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 224,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 466,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 97,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 390,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 83,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 301,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 125,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d996404088e26d52d6642814fe498b888b81a63c",
          "message": "Merge pull request #145 from zenc-lang-ce/compat/slices-0.5\n\nparser, codegen: keep 0.5.1 slices behind FF_COMPATIBILITY_0_5",
          "timestamp": "2026-11-05T11:33:56+01:00",
          "tree_id": "0d26ce8c1a1dbf435e27f49001a90e7094948e87",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/d996404088e26d52d6642814fe498b888b81a63c"
        },
        "date": 1793874934488,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11552,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 221,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 151,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 106,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 586,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 137,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 498,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 376,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 151,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "352306f4be27245e5589a42242fd1ac16bd97b7a",
          "message": "Merge pull request #146 from zenc-lang-ce/refactor/impl-target-type\n\nparser, codegen: keep an impl's target as its Type",
          "timestamp": "2026-11-05T12:43:45+01:00",
          "tree_id": "97639a9d2c9c7177c508fdd2468e450649f1e294",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/352306f4be27245e5589a42242fd1ac16bd97b7a"
        },
        "date": 1793879127496,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11700,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 236,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 150,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 106,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 595,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 133,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 498,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 102,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 385,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 154,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f5e9576103736a8eff6e9648f45bf8d9109c2bfe",
          "message": "Merge pull request #147 from paulboul1013/fix/issue13-std-diagnostic\n\nfix(parser): use the module path for implicit std import diagnostics",
          "timestamp": "2026-11-05T13:17:16+01:00",
          "tree_id": "ef0da27736db3a0ce48851f082ea06a7a2dc9cc2",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/f5e9576103736a8eff6e9648f45bf8d9109c2bfe"
        },
        "date": 1793881135773,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11915,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 224,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 71,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 148,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 108,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 265,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 587,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 142,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 487,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 101,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 380,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 154,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5e01fc9bcbecb155fa6a9f59dd53228ced295f34",
          "message": "Merge pull request #149 from zenc-lang-ce/refactor/option-result-by-type\n\ntypecheck, codegen: tell an Option or a Result by its Type",
          "timestamp": "2026-11-05T13:50:36+01:00",
          "tree_id": "268272f3350fb76e151625af42e59529384028dc",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/5e01fc9bcbecb155fa6a9f59dd53228ced295f34"
        },
        "date": 1793883142711,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9402,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 177,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 109,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 88,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 224,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 456,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 381,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 82,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 297,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 124,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b59457db7d50707097b3a1935bca6eb532a40450",
          "message": "Merge pull request #150 from zenc-lang-ce/fix/fuzz-harness-curr-func-ret\n\nfuzz: declare curr_func_ret as the Type it is",
          "timestamp": "2026-11-05T15:07:02+01:00",
          "tree_id": "88105abcf9a388b6b151d083ed4dda13cdefb618",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/b59457db7d50707097b3a1935bca6eb532a40450"
        },
        "date": 1793887697232,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 8348,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 157,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 36,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 54,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 81,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 78,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 52,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 165,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 396,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 17,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 53,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 342,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 74,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 267,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 9,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 109,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7cdc93ba5862cb2796b0205c477993c3041e9f91",
          "message": "Merge pull request #155 from zenc-lang-ce/refactor/lsp-indexing-tuple\n\nparser, lsp: type tuple literals while the workspace is indexed",
          "timestamp": "2026-11-05T15:12:00+01:00",
          "tree_id": "e210742b9c5097fbf1c05666d837f45fe3b067f6",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/7cdc93ba5862cb2796b0205c477993c3041e9f91"
        },
        "date": 1793887990803,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 7648,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 146,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 51,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 49,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 74,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 72,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 48,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 153,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 368,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 48,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 62,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 318,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 32,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 238,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 99,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8b8a1157fc4968ca1922ad038ad82c12acc0bb95",
          "message": "Merge pull request #152 from zenc-lang-ce/fix/fields-link-name\n\ncodegen: name a @link_name struct by its C name in @fields",
          "timestamp": "2026-11-05T15:12:41+01:00",
          "tree_id": "e32dd56cda7e01260b9b756ccaec57e5cf69132c",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/8b8a1157fc4968ca1922ad038ad82c12acc0bb95"
        },
        "date": 1793888088288,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 9289,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 181,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 50,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 110,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 87,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 55,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 222,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 467,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 97,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 400,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 56,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 82,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 300,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 14,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 124,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "71ef95eb4478beb535a19a773480cb04b5b9905d",
          "message": "Merge pull request #151 from zenc-lang-ce/fix/match-pointer-option-result\n\ncodegen: match through a pointer subject",
          "timestamp": "2026-11-05T15:12:57+01:00",
          "tree_id": "b6432029dc39c562e3ea222c69c715af3e26f135",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/71ef95eb4478beb535a19a773480cb04b5b9905d"
        },
        "date": 1793888111584,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10936,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 217,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 146,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 104,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 65,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 263,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 567,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 23,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 68,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 134,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 472,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 98,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 364,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 151,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f235102066498414fda96ed8c26cc383c9bffe4e",
          "message": "Merge pull request #154 from zenc-lang-ce/fix/c-reserved-names\n\ncodegen: write names that are C or C++ keywords under another name",
          "timestamp": "2026-11-05T15:35:20+01:00",
          "tree_id": "b929776eb627113856fe19d3ec90c8405e19d767",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/f235102066498414fda96ed8c26cc383c9bffe4e"
        },
        "date": 1793889410670,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 10465,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 2,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 198,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 61,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 118,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 93,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 57,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 215,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 526,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 18,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 60,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 101,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 446,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 60,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 90,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 341,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 12,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 138,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "14831a8f25da6fa55656c66490dfbf87b86770cb",
          "message": "Merge pull request #165 from zenc-lang-ce/refactor/remove-pending-closure-frees\n\ncodegen: remove the pending closure frees, which nothing ever queued",
          "timestamp": "2026-11-05T19:11:50+01:00",
          "tree_id": "7554e32a7b7efb248374a0079d4bd15a8de41a2e",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/14831a8f25da6fa55656c66490dfbf87b86770cb"
        },
        "date": 1793902395943,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 7199,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 156,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 47,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 44,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 75,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 72,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 48,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 155,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 351,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 16,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 45,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 64,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 313,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 34,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 231,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 8,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 99,
            "unit": "ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "1103977+pherrymason@users.noreply.github.com",
            "name": "Raul",
            "username": "pherrymason"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "4ffc12068423a12af46cb4ce1584b4c75d15c65b",
          "message": "Merge pull request #164 from zenc-lang-ce/fix/tuple-moves-and-sizeof\n\ntypecheck, parser: move tuples that hold a value with Drop; sizeof of a tuple of variables",
          "timestamp": "2026-11-05T19:11:28+01:00",
          "tree_id": "adb3cae3644e6d2cae14bbaaf40cc9101086ffe4",
          "url": "https://github.com/zenc-lang-ce/zenc/commit/4ffc12068423a12af46cb4ce1584b4c75d15c65b"
        },
        "date": 1793902401449,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Compiler (Full Suite Transpilation)",
            "value": 11450,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_bigint_factorial)",
            "value": 3,
            "unit": "ms"
          },
          {
            "name": "Compile (test_bigint_factorial)",
            "value": 217,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_comptime)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_comptime)",
            "value": 69,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_fib_recursive)",
            "value": 58,
            "unit": "ms"
          },
          {
            "name": "Compile (test_fib_recursive)",
            "value": 67,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hashmap)",
            "value": 147,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hashmap)",
            "value": 105,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_hello)",
            "value": 0,
            "unit": "ms"
          },
          {
            "name": "Compile (test_hello)",
            "value": 66,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_json_parse)",
            "value": 266,
            "unit": "ms"
          },
          {
            "name": "Compile (test_json_parse)",
            "value": 570,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_mandelbrot)",
            "value": 22,
            "unit": "ms"
          },
          {
            "name": "Compile (test_mandelbrot)",
            "value": 70,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sha256)",
            "value": 134,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sha256)",
            "value": 497,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_sort_large)",
            "value": 63,
            "unit": "ms"
          },
          {
            "name": "Compile (test_sort_large)",
            "value": 100,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_string_concat)",
            "value": 1,
            "unit": "ms"
          },
          {
            "name": "Compile (test_string_concat)",
            "value": 372,
            "unit": "ms"
          },
          {
            "name": "Runtime (test_vec_ops)",
            "value": 19,
            "unit": "ms"
          },
          {
            "name": "Compile (test_vec_ops)",
            "value": 151,
            "unit": "ms"
          }
        ]
      }
    ]
  }
}