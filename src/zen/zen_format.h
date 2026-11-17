// SPDX-License-Identifier: MIT
#ifndef ZEN_FORMAT_H
#ifndef ZC_ALLOW_INTERNAL
#error "zen/zen_format.h is internal to Zen C. Include the appropriate public header instead."
#endif

#define ZEN_FORMAT_H

/**
 * @brief Formats Zen C source code: what zc-format prints and the language server's formatting
 * gives.
 *
 * @param src The original source code.
 * @return char* The formatted source code (must be freed by caller).
 */
char *zen_format_source(const char *src);

#endif
