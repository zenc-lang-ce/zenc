window.BENCHMARK_DATA = {
  "lastUpdate": 1793335164757,
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
      }
    ]
  }
}