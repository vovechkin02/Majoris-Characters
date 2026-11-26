#ifndef CHARACTERS_ICU_ADAPTER_H
#define CHARACTERS_ICU_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

/** Count ICU extended grapheme clusters in validated, bounded UTF-8. */
int characters_icu_graphemes(const char *text);

/** Write direct decomposition scalars to a caller-owned 32-element array; zero if absent. */
size_t characters_icu_decomposition(uint32_t point, uint32_t *points);

/**
 * Normalize a bounded scalar array, preserving embedded NUL.
 * Return an owned scalar array (free with free), or NULL; length is in scalars.
 */
uint32_t *characters_icu_normalize_points(const uint32_t *points, int count,
                                         const char *form, int *output_count);

/**
 * characters_unicode_encode:
 * @points: (array length=length): Unicode scalars.
 * @length: Number of scalars, from 1 to 32.
 *
 * Returns: (transfer full) (nullable): UTF-8 text, or NULL for invalid input,
 * embedded NUL or allocation failure. No normalization is performed.
 */
char *characters_unicode_encode(const uint32_t *points, size_t length);

/**
 * characters_unicode_normalize:
 * @text: NUL-terminated UTF-8 text, at most 1 MiB.
 * @form: NFC, NFD, NFKC or NFKD.
 *
 * Returns: (transfer full) (nullable): Normalized UTF-8, or NULL for invalid
 * text, form, size or allocation failure. The original text is not modified.
 */
char *characters_unicode_normalize(const char *text, const char *form);

#endif
