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

/** Find one exact locale tag, ignoring ASCII case and normalizing separators at the caller. */
static const struct AnnotationLocale *find_locale(const gchar *language)
{
    for (gsize index = 0; index < G_N_ELEMENTS(annotation_locales); index++) {
        if (g_ascii_strcasecmp(language, annotation_locales[index].language) == 0) {
            return &annotation_locales[index];
        }
    }
    return NULL;
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

/** Check exact annotation keys before falling back to the unmodified skin-tone base. */
static gboolean locale_matches(const struct AnnotationLocale *locale, const gunichar *points,
                           gsize count, const gchar *term)
{
    if (!locale) {
        return FALSE;
    }
    for (guint attempt = 0; attempt < 2; attempt++) {
        guint32 key[MAX_POINTS];
        gsize length = annotation_key(points, count, key, attempt != 0);
        gchar *text = characters_unicode_encode(key, length);
        const gchar *words = lookup_words(locale, text);
        gboolean match = words && strstr(words, term);
        g_free(text);
        if (match) {
            return TRUE;
        }
    }
    return FALSE;
}

/** Search the locale and its parents, with a Chinese script fallback for regional tags. */
static gboolean matches_locale_chain(const gchar *locale, const gunichar *points, gsize count,
                                 const gchar *term)
{
    gchar language[64] = { 0 };
    if (!locale) {
        return FALSE;
    }
    gsize length = strcspn(locale, ".@:");
    if (length >= sizeof(language)) {
        return FALSE;
    }
    memcpy(language, locale, length);
    for (gsize index = 0; index < length; index++) {
        if (language[index] == '-') {
            language[index] = '_';
        }
    }
    if (locale_matches(find_locale(language), points, count, term)) {
        return TRUE;
    }
    if (g_ascii_strcasecmp(language, "zh_TW") == 0 || g_ascii_strcasecmp(language, "zh_HK") == 0 ||
        g_ascii_strcasecmp(language, "zh_MO") == 0) {
        if (locale_matches(find_locale("zh_Hant"), points, count, term)) {
            return TRUE;
        }
    } else if (g_ascii_strcasecmp(language, "zh_CN") == 0 || g_ascii_strcasecmp(language, "zh_SG") == 0) {
        if (locale_matches(find_locale("zh_Hans"), points, count, term)) {
            return TRUE;
        }
    }
    gchar *separator;
    while ((separator = strrchr(language, '_')) != NULL) {
        *separator = '\0';
        if (locale_matches(find_locale(language), points, count, term)) {
            return TRUE;
        }
    }
    return FALSE;
}

/** Match an annotation term without mutating either immutable source table. */
gboolean characters_emoji_keyword_matches(gsize index, const gchar *locale, const gchar *term)
{
    gchar folded[MAX_TERM_BYTES * 12 + 1];
    if (!fold_term(term, folded, sizeof(folded))) {
        return FALSE;
    }
    gunichar points[MAX_POINTS];
    gchar *text = characters_unicode_emoji_text(index);
    gsize count = characters_icu_decode_utf8(text, points, MAX_POINTS);
    g_free(text);
    if (!count) {
        return FALSE;
    }
    return matches_locale_chain(locale, points, count, folded) ||
           locale_matches(find_locale("en"), points, count, folded);
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

/** Return a stable label for a noncatalog sequence. */
const gchar *characters_emoji_alias_label(gsize index)
{
    const gchar *text = characters_emoji_alias_text(index);
    gunichar points[MAX_POINTS];
    gsize count = characters_icu_decode_utf8(text, points, MAX_POINTS);
    if (!count) {
        return NULL;
    }
    if (characters_emoji_is_presentation_base(points[0])) {
        if (count == 1) {
            return "Default presentation";
        }
        if (count == 2 && points[1] == 0xFE0E) {
            return "Text presentation";
        }
        if (count == 2 && points[1] == 0xFE0F) {
            return "Emoji presentation";
        }
    }
    return characters_unicode_emoji_label(characters_emoji_alias_canonical(index));
}

/** Allocate an informative name including the base identity for explicit presentation pairs. */
gchar *characters_emoji_alias_name(gsize index)
{
    const gchar *label = characters_emoji_alias_label(index);
    const gchar *text = characters_emoji_alias_text(index);
    gunichar points[MAX_POINTS];
    gsize count = characters_icu_decode_utf8(text, points, MAX_POINTS);
    if (!label || !count) {
        return NULL;
    }
    if (count > 2 || (count == 2 && points[1] != 0xFE0E && points[1] != 0xFE0F)) {
        return characters_try_strdup(label);
    }
    gchar *name = characters_unicode_name(points[0]);
    if (!name) {
        return characters_try_strdup(label);
    }
    gsize capacity = strlen(name) + strlen(label) + 4;
    gchar *result = g_try_malloc(capacity);
    if (result) {
        g_snprintf(result, capacity, "%s (%s)", name, label);
    }
    g_free(name);
    return result;
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

