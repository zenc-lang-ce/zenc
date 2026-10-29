// SPDX-License-Identifier: MIT
#ifndef C23_COMPAT_H
#define C23_COMPAT_H

// C23 compatibility layer.
// Provides C23-standard names with fallbacks for C11/C17 compilers.
// Include this via zprep.h so all source files get it.

// C23 detection
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define ZEN_C23 1
#else
#define ZEN_C23 0
#endif

// bool / true / false — keywords in C23, macros in <stdbool.h> for C11
#if !ZEN_C23
#include <stdbool.h>
#endif

// constexpr — C23 keyword (GCC 13+, Clang 19+).
// Set via Makefile probe: -DHAS_CONSTEXPR if the compiler accepts the keyword.
// For compile-time constants usable as array sizes, use enum instead.
#ifdef HAS_CONSTEXPR
#define ZEN_CONSTEXPR constexpr
#else
#define ZEN_CONSTEXPR static const
#endif

// nullptr — C23 keyword
#if ZEN_C23
#define ZEN_NULL nullptr
#else
#define ZEN_NULL NULL
#endif

// Standard attributes via __has_c_attribute (C23, GCC 14+, Clang 16+).
// Fallback to GNU __attribute__ or nothing.
// Note: __has_c_attribute is a C23 feature; compilers that don't define it
// (e.g. TCC) skip the C23 attribute path entirely.

#ifdef __has_c_attribute
#if __has_c_attribute(maybe_unused)
#define ZEN_MAYBE_UNUSED [[maybe_unused]]
#else
#define ZEN_MAYBE_UNUSED __attribute__((unused))
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define ZEN_MAYBE_UNUSED __attribute__((unused))
#else
#define ZEN_MAYBE_UNUSED
#endif

#ifdef __has_c_attribute
#if __has_c_attribute(nodiscard)
#define ZEN_NODISCARD [[nodiscard]]
#else
#define ZEN_NODISCARD __attribute__((warn_unused_result))
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define ZEN_NODISCARD __attribute__((warn_unused_result))
#else
#define ZEN_NODISCARD
#endif

#ifdef __has_c_attribute
#if __has_c_attribute(fallthrough)
#define ZEN_FALLTHROUGH [[fallthrough]]
#else
#define ZEN_FALLTHROUGH ((void)0)
#endif
#else
#define ZEN_FALLTHROUGH ((void)0)
#endif

// returns_nonnull — not yet a C23 standard attribute, use GNU extension
#if defined(__GNUC__) || defined(__clang__)
#define ZEN_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define ZEN_RETURNS_NONNULL
#endif

// const / pure — GCC/Clang attributes for function purity analysis
// ZEN_CONST:  result depends only on arguments (no global reads)
// ZEN_PURE:   result depends on arguments + readable global memory
#if defined(__GNUC__) || defined(__clang__)
#define ZEN_CONST __attribute__((const))
#define ZEN_PURE __attribute__((pure))
#else
#define ZEN_CONST
#define ZEN_PURE
#endif

// format(printf) — GCC/Clang attribute for printf-like functions
// Uses __printf__ to avoid expansion of #define printf zprintf in colors.h
// On MinGW, GCC checks __printf__ against the msvcrt printf, which lacks C99 formats
// such as %zu; use the format <stdio.h> picks for the C runtime in use instead.
#ifdef __MINGW32__
#include <stdio.h>
#endif
#ifdef __MINGW_PRINTF_FORMAT
#define ZEN_FORMAT_PRINTF(fmt_arg, first_vararg)                                                   \
    __attribute__((format(__MINGW_PRINTF_FORMAT, fmt_arg, first_vararg)))
#elif defined(__GNUC__) || defined(__clang__)
#define ZEN_FORMAT_PRINTF(fmt_arg, first_vararg)                                                   \
    __attribute__((format(__printf__, fmt_arg, first_vararg)))
#else
#define ZEN_FORMAT_PRINTF(fmt_arg, first_vararg)
#endif

// unreachable() — C23 standard macro
#if ZEN_C23
// Use the standard macro (defined in <stddef.h> in C23)
#elif !defined(unreachable) && (defined(__GNUC__) || defined(__clang__))
#define unreachable() __builtin_unreachable()
#elif !defined(unreachable) && defined(_MSC_VER)
#define unreachable() __assume(0)
#elif !defined(unreachable)
#define unreachable() ((void)0)
#endif

#endif // C23_COMPAT_H
