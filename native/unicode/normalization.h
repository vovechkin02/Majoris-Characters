#ifndef CHARACTERS_NORMALIZATION_H
#define CHARACTERS_NORMALIZATION_H

#include <glib.h>

#include <unicode/utypes.h>

/**
 * Normalize an explicit-length UTF-16 buffer using the Unicode 17 snapshot.
 * Returns allocated UTF-16 (free with g_free()), including a trailing NUL, or NULL
 * on invalid input or allocation failure. Embedded NULs are preserved.
 */
UChar *characters_normalize_utf16(const UChar *input, int32_t length,
                                  const gchar *form, int32_t *output_length);

#endif
