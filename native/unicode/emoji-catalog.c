#include "emoji-catalog.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <glib.h>

#include <unicode/unorm2.h>
#include <unicode/ustring.h>

#include "../allocation.h"
#include "icu-scalar-conversion.h"
#include "unicode-engine.h"

#define COUNT(array) G_N_ELEMENTS(array)
#define MAX_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
#define MAX_TEXT_BYTES (MAX_POINTS * 4)
#define MAX_TERM_BYTES 512

/** One exact normalized lookup key and its folded annotation terms. */
struct Annotation {
    /** UTF-8 emoji sequence without presentation selectors. */
    const gchar *key;
    /** Folded names and keywords from the source annotation dataset. */
    const gchar *words;
};

/** Immutable sorted locale-specific annotation table. */
struct AnnotationLocale {
    /** Source language or locale code. */
    const gchar *language;
    /** Sorted lookup records. */
    const struct Annotation *records;
    /** Number of records. */
    gsize count;
};

#include "generated/emoji-annotations.h"

/** Exact noncatalog identity and its fully qualified catalog index, if available. */
struct SequenceAlias {
    /** Exact UTF-8 text; sorted bytewise in the generated table. */
    const gchar *text;
    /** Catalog index or G_MAXSIZE for standalone presentation-only bases. */
    gsize canonical;
};

#include "generated/emoji-sequences.h"

/** Identify the five standard Fitzpatrick modifiers. */
static gboolean is_tone(gunichar point)
{
    return point >= 0x1F3FB && point <= 0x1F3FF;
}

/** Remove selectors and optionally modifiers, preserving all other components. */
static gsize annotation_key(const gunichar *points, gsize count, guint32 *key,
                             gboolean remove_tones)
{
    gsize length = 0;
    for (gsize index = 0; index < count; index++) {
        gunichar point = points[index];
        if (point != 0xFE0E && point != 0xFE0F && !(remove_tones && is_tone(point))) {
            key[length++] = point;
        }
    }
    return length;
}

/** Return an exact annotation locale index, or G_MAXUINT32 when absent. */
guint32 characters_emoji_locale_index(const gchar *language)
{
    if (language) {
        for (gsize index = 0; index < G_N_ELEMENTS(annotation_locales); index++) {
            if (g_ascii_strcasecmp(language, annotation_locales[index].language) == 0) {
                return (guint32)index;
            }
        }
    }
    return G_MAXUINT32;
}

/** Return the number of immutable locale tables for prepared-index validation. */
gsize characters_emoji_locale_count(void)
{
    return G_N_ELEMENTS(annotation_locales);
}

/** Binary-search one locale's sorted UTF-8 identity keys. */
static const gchar *lookup_words(const struct AnnotationLocale *locale, const gchar *key)
{
    if (!locale || !key) {
        return NULL;
    }
    gsize low = 0;
    gsize high = locale->count;
    while (low < high) {
        gsize middle = low + (high - low) / 2;
        gint comparison = strcmp(key, locale->records[middle].key);
        if (!comparison) {
            return locale->records[middle].words;
        }
        if (comparison < 0) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }
    return NULL;
}

/** Fold a bounded query using ICU NFKC case folding; false on invalid or oversized output. */
static gboolean fold_term(const gchar *term, gchar *output, int32_t capacity)
{
    if (!term || !*term || strnlen(term, MAX_TERM_BYTES + 1) > MAX_TERM_BYTES) {
        return FALSE;
    }
    UChar input[MAX_TERM_BYTES + 1];
    UChar folded[MAX_TERM_BYTES * 4 + 1];
    int32_t length;
    UErrorCode status = U_ZERO_ERROR;
    u_strFromUTF8(input, G_N_ELEMENTS(input), &length, term, -1, &status);
    const UNormalizer2 *normalizer = unorm2_getNFKCCasefoldInstance(&status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    length = unorm2_normalize(normalizer, input, length, folded, G_N_ELEMENTS(folded), &status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    u_strToUTF8(output, capacity, &length, folded, length, &status);
    return U_SUCCESS(status) && length > 0 && length < capacity;
}

/** Allocate a folded operand once for a prepared query; NULL on invalid or empty input. */
gchar *characters_emoji_fold_term(const gchar *term)
{
    gchar folded[MAX_TERM_BYTES * 12 + 1];
    return fold_term(term, folded, sizeof(folded)) ? characters_try_strdup(folded) : NULL;
}

/** Encode one bounded annotation key into caller-owned stack storage. */
static gboolean encode_key(const gunichar *points, gsize count, gboolean remove_tones,
                           gchar *text)
{
    gunichar key[MAX_POINTS];
    gsize length = annotation_key(points, count, key, remove_tones);
    gsize offset = 0;
    for (gsize index = 0; index < length; index++) {
        if (!characters_unicode_is_scalar(key[index]) || !key[index]) {
            return FALSE;
        }
        offset += (gsize)g_unichar_to_utf8(key[index], text + offset);
    }
    text[offset] = '\0';
    return offset != 0;
}

/** Match one folded term using only the caller's resolved locale indexes. */
gboolean characters_emoji_keyword_matches(const gunichar *points, gsize count,
                                          const guint32 *locales, gsize locale_count,
                                          const gchar *term)
{
    if (!points || !count || count > MAX_POINTS || !locales || !locale_count ||
        locale_count > G_N_ELEMENTS(annotation_locales) || !term || !*term) {
        return FALSE;
    }
    gchar exact[MAX_TEXT_BYTES + 1];
    gchar base[MAX_TEXT_BYTES + 1];
    if (!encode_key(points, count, FALSE, exact)) {
        return FALSE;
    }
    gboolean has_base = encode_key(points, count, TRUE, base) && strcmp(exact, base) != 0;
    for (gsize index = 0; index < locale_count; index++) {
        if (locales[index] >= G_N_ELEMENTS(annotation_locales)) {
            continue;
        }
        const struct AnnotationLocale *locale = &annotation_locales[locales[index]];
        const gchar *words = lookup_words(locale, exact);
        if (words && strstr(words, term)) {
            return TRUE;
        }
        if (has_base) {
            words = lookup_words(locale, base);
            if (words && strstr(words, term)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/** Return whether a scalar has both standardized presentation selectors. */
gboolean characters_emoji_is_presentation_base(gunichar point)
{
    gsize low = 0;
    gsize high = G_N_ELEMENTS(presentation_bases);
    while (low < high) {
        gsize middle = low + (high - low) / 2;
        if (point == presentation_bases[middle]) {
            return TRUE;
        }
        if (point < presentation_bases[middle]) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }
    return FALSE;
}

/** Return the exact noncatalog identity index, never a normalized approximation. */
gsize characters_emoji_alias_index(const gchar *text)
{
    if (!text || strnlen(text, MAX_TEXT_BYTES + 1) > MAX_TEXT_BYTES) {
        return G_MAXSIZE;
    }
    gsize low = 0;
    gsize high = G_N_ELEMENTS(sequence_aliases);
    while (low < high) {
        gsize middle = low + (high - low) / 2;
        gint comparison = strcmp(text, sequence_aliases[middle].text);
        if (!comparison) {
            return middle;
        }
        if (comparison < 0) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }
    return G_MAXSIZE;
}

/** Return the number of exact supplementary sequence identities. */
gsize characters_emoji_alias_count(void)
{
    return G_N_ELEMENTS(sequence_aliases);
}

/** Return the exact noncatalog sequence, or NULL for invalid indexes. */
const gchar *characters_emoji_alias_text(gsize index)
{
    return index < G_N_ELEMENTS(sequence_aliases) ? sequence_aliases[index].text : NULL;
}

/** Return the associated fully qualified index when one exists. */
gsize characters_emoji_alias_canonical(gsize index)
{
    return index < G_N_ELEMENTS(sequence_aliases) ? sequence_aliases[index].canonical : G_MAXSIZE;
}

/** Expose annotation provenance independently of the Unicode character repertoire. */
const gchar *characters_emoji_annotation_source(void)
{
    return EMOJI_ANNOTATION_SOURCE;
}

/** Validate the complete input before attempting catalog resolution. */
gboolean characters_emoji_is_candidate(const gchar *text)
{
    gunichar points[MAX_POINTS];
    gsize count = characters_icu_decode_utf8(text, points, MAX_POINTS);
    return count && (characters_unicode_emoji_index(points, count) != G_MAXSIZE ||
                     characters_emoji_alias_index(text) != G_MAXSIZE);
}

