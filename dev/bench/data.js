window.BENCHMARK_DATA = {
  "lastUpdate": 1793274744028,
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
      }
    ]
  }
}