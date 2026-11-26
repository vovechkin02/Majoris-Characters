#ifndef CHARACTERS_ICU_ADAPTER_H
#define CHARACTERS_ICU_ADAPTER_H

#include <glib.h>

/** Count Unicode 17 extended grapheme clusters in validated, bounded UTF-8. */
gint characters_icu_graphemes(const gchar *text);

/** Write direct decomposition scalars to a caller-owned 32-element array; zero if absent. */
gsize characters_icu_decomposition(gunichar point, gunichar *points);

/** Return the pinned Unicode 17 simple uppercase mapping, or the input if invalid or unmapped. */
gunichar characters_icu_uppercase(gunichar point);

/** Return the pinned Unicode 17 simple lowercase mapping, or the input if invalid or unmapped. */
gunichar characters_icu_lowercase(gunichar point);

/** Return the pinned Unicode 17 simple titlecase mapping, or the input if invalid or unmapped. */
gunichar characters_icu_titlecase(gunichar point);

/** Return the ICU bidirectional mirror, or the input if invalid or unmapped. */
gunichar characters_icu_mirror(gunichar point);

/**
 * Write recursive canonical decomposition to a caller-owned 32-element scalar
 * array. Return its scalar count, or zero for no mapping, invalid input or error.
 * Compatibility mappings are not included. Unlike direct decomposition, this
 * operation recursively expands canonical mappings, including Hangul Jamo.
 */
gsize characters_icu_canonical_decomposition(gunichar point, gunichar *points);

/**
 * Normalize a bounded scalar array, preserving embedded NUL.
 * Return an owned scalar array (free with g_free()), or NULL; length is in scalars.
 */
gunichar *characters_icu_normalize_points(const gunichar *points, gint count,
                                         const gchar *form, gint *output_count);

/**
 * characters_unicode_encode:
 * @points: (array length=length): Unicode scalars.
 * @length: Number of scalars, from 1 to 32.
 *
 * Returns: (transfer full) (nullable): UTF-8 text, or NULL for invalid input,
 * embedded NUL or allocation failure. No normalization is performed.
 */
gchar *characters_unicode_encode(const gunichar *points, gsize length);

/**
 * characters_unicode_normalize:
 * @text: NUL-terminated UTF-8 text, at most 1 MiB.
 * @form: NFC, NFD, NFKC or NFKD.
 *
 * Returns: (transfer full) (nullable): Normalized UTF-8, or NULL for invalid
 * text, form, size or allocation failure. The original text is not modified.
 */
gchar *characters_unicode_normalize(const gchar *text, const gchar *form);

#endif
