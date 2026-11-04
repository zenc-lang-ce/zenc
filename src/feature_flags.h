// SPDX-License-Identifier: MIT
#ifndef ZEN_FEATURE_FLAGS_H
#define ZEN_FEATURE_FLAGS_H

/**
 * @file feature_flags.h
 * @brief Switches for behaviour that differs between release lines.
 *
 * Each flag guards its code with `#if FF_...`, so removing one is deleting its blocks.
 */

/**
 * FF_COMPATIBILITY_0_5: slices behave as in 0.5.1. A slice has a `cap` field (set to its
 * `len`), `Slice_char` and the other `Slice_X` names stand for `Slice<X>`, and an array
 * literal returned as a slice is copied to the heap and never freed. On for the 0.5.x
 * releases, which come from `main`; `next/0.6` sets it to 0, and 0.6 removes it with every
 * `#if FF_COMPATIBILITY_0_5` block. Tests that depend on it name it too
 * (`grep -rn FF_COMPATIBILITY_0_5 src tests`).
 */
#ifndef FF_COMPATIBILITY_0_5
#define FF_COMPATIBILITY_0_5 0
#endif

#endif
