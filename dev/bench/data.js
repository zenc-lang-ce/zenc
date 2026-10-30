window.BENCHMARK_DATA = {
  "lastUpdate": 1793385986041,
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
      }
    ]
  }
}