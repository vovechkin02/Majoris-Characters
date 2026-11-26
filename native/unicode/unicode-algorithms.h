#ifndef CHARACTERS_UNICODE_ALGORITHMS_H
#define CHARACTERS_UNICODE_ALGORITHMS_H

#include <glib.h>

/** Select a pinned simple case mapping. */
typedef enum {
    /** Uppercase mapping. */
    CHARACTERS_CASE_UPPER,
    /** Lowercase mapping. */
    CHARACTERS_CASE_LOWER,
    /** Titlecase mapping. */
    CHARACTERS_CASE_TITLE
} CharactersCaseMapping;

/**
 * characters_unicode_simple_case:
 * Return a pinned simple case mapping, or the input when unmapped.
 */
gunichar characters_unicode_simple_case(gunichar point, CharactersCaseMapping mapping);

/**
 * characters_unicode_graphemes:
 * Count extended graphemes in bounded UTF-8, returning -1 for invalid input.
 */
gint characters_unicode_graphemes(const gchar *text);

/**
 * characters_unicode_graphemes_points:
 * Count extended graphemes in explicit scalar storage, including NUL; -1 on error.
 */
gint characters_unicode_graphemes_points(const gunichar *points, gsize count);

/**
 * characters_unicode_casefold:
 * Return owned default full casefolded UTF-8; NULL on invalid or oversized input.
 */
gchar *characters_unicode_casefold(const gchar *text);

/**
 * characters_unicode_nfkc_casefold:
 * Return owned Unicode 17 NFKC_Casefold UTF-8, including valid empty results.
 */
gchar *characters_unicode_nfkc_casefold(const gchar *text);

#endif
