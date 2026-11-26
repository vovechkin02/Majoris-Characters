#include "emoji-catalog.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <unicode/unorm2.h>
#include <unicode/ustring.h>
#include <unicode/utf8.h>

#include "unicode-engine.h"

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define MAX_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
#define MAX_TEXT_BYTES (MAX_POINTS * 4)
#define MAX_TERM_BYTES 512

/** One exact normalized lookup key and its folded annotation terms. */
struct Annotation {
    /** UTF-8 emoji sequence without presentation selectors. */
    const char *key;
    /** Folded names and keywords from the source annotation dataset. */
    const char *words;
};

/** Immutable sorted locale-specific annotation table. */
struct AnnotationLocale {
    /** Source language or locale code. */
    const char *language;
    /** Sorted lookup records. */
    const struct Annotation *records;
    /** Number of records. */
    size_t count;
};

#include "generated/emoji-annotations.h"

/** Exact noncatalog identity and its fully qualified catalog index, if available. */
struct SequenceAlias {
    /** Exact UTF-8 text; sorted bytewise in the generated table. */
    const char *text;
    /** Catalog index or SIZE_MAX for standalone presentation-only bases. */
    size_t canonical;
};

#include "generated/emoji-sequences.h"

/** Decode bounded UTF-8, rejecting malformed input and oversized sequences. */
static size_t decode(const char *text, uint32_t *points) {
    if (!text || strnlen(text, MAX_TEXT_BYTES + 1) > MAX_TEXT_BYTES) {
        return 0;
    }
    size_t count = 0;
    int32_t length = (int32_t)strlen(text);
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT((const uint8_t *)text, offset, length, point);
        if (point < 0 || count == MAX_POINTS) {
            return 0;
        }
        points[count++] = (uint32_t)point;
    }
    return count;
}

/** Identify the five standard Fitzpatrick modifiers. */
static bool is_tone(uint32_t point) {
    return point >= 0x1F3FB && point <= 0x1F3FF;
}

/** Remove selectors and optionally modifiers, preserving all other components. */
static size_t annotation_key(const uint32_t *points, size_t count, uint32_t *key,
                             bool remove_tones) {
    size_t length = 0;
    for (size_t index = 0; index < count; index++) {
        uint32_t point = points[index];
        if (point != 0xFE0E && point != 0xFE0F && !(remove_tones && is_tone(point))) {
            key[length++] = point;
        }
    }
    return length;
}

/** Find one exact locale tag, ignoring ASCII case and normalizing separators at the caller. */
static const struct AnnotationLocale *find_locale(const char *language) {
    for (size_t index = 0; index < COUNT(annotation_locales); index++) {
        if (strcasecmp(language, annotation_locales[index].language) == 0) {
            return &annotation_locales[index];
        }
    }
    return NULL;
}

/** Binary-search one locale's sorted UTF-8 identity keys. */
static const char *lookup_words(const struct AnnotationLocale *locale, const char *key) {
    if (!locale || !key) {
        return NULL;
    }
    size_t low = 0;
    size_t high = locale->count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        int comparison = strcmp(key, locale->records[middle].key);
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
static bool fold_term(const char *term, char *output, int32_t capacity) {
    if (!term || !*term || strnlen(term, MAX_TERM_BYTES + 1) > MAX_TERM_BYTES) {
        return false;
    }
    UChar input[MAX_TERM_BYTES + 1];
    UChar folded[MAX_TERM_BYTES * 4 + 1];
    int32_t length;
    UErrorCode status = U_ZERO_ERROR;
    u_strFromUTF8(input, COUNT(input), &length, term, -1, &status);
    const UNormalizer2 *normalizer = unorm2_getNFKCCasefoldInstance(&status);
    if (U_FAILURE(status)) {
        return false;
    }
    length = unorm2_normalize(normalizer, input, length, folded, COUNT(folded), &status);
    if (U_FAILURE(status)) {
        return false;
    }
    u_strToUTF8(output, capacity, &length, folded, length, &status);
    return U_SUCCESS(status) && length > 0 && length < capacity;
}

/** Check exact annotation keys before falling back to the unmodified skin-tone base. */
static bool locale_matches(const struct AnnotationLocale *locale, const uint32_t *points,
                           size_t count, const char *term) {
    if (!locale) {
        return false;
    }
    for (unsigned attempt = 0; attempt < 2; attempt++) {
        uint32_t key[MAX_POINTS];
        size_t length = annotation_key(points, count, key, attempt != 0);
        char *text = characters_unicode_encode(key, length);
        const char *words = lookup_words(locale, text);
        bool match = words && strstr(words, term);
        free(text);
        if (match) {
            return true;
        }
    }
    return false;
}

/** Search the locale and its parents, with a Chinese script fallback for regional tags. */
static bool matches_locale_chain(const char *locale, const uint32_t *points, size_t count,
                                 const char *term) {
    char language[64] = { 0 };
    if (!locale) {
        return false;
    }
    size_t length = strcspn(locale, ".@:");
    if (length >= sizeof(language)) {
        return false;
    }
    memcpy(language, locale, length);
    for (size_t index = 0; index < length; index++) {
        if (language[index] == '-') {
            language[index] = '_';
        }
    }
    if (locale_matches(find_locale(language), points, count, term)) {
        return true;
    }
    if (strcasecmp(language, "zh_TW") == 0 || strcasecmp(language, "zh_HK") == 0 ||
        strcasecmp(language, "zh_MO") == 0) {
        if (locale_matches(find_locale("zh_Hant"), points, count, term)) {
            return true;
        }
    } else if (strcasecmp(language, "zh_CN") == 0 || strcasecmp(language, "zh_SG") == 0) {
        if (locale_matches(find_locale("zh_Hans"), points, count, term)) {
            return true;
        }
    }
    char *separator;
    while ((separator = strrchr(language, '_')) != NULL) {
        *separator = '\0';
        if (locale_matches(find_locale(language), points, count, term)) {
            return true;
        }
    }
    return false;
}

/** Match an annotation term without mutating either immutable source table. */
bool characters_emoji_keyword_matches(size_t index, const char *locale, const char *term) {
    char folded[MAX_TERM_BYTES * 12 + 1];
    if (!fold_term(term, folded, sizeof(folded))) {
        return false;
    }
    uint32_t points[MAX_POINTS];
    char *text = characters_unicode_emoji_text(index);
    size_t count = decode(text, points);
    free(text);
    if (!count) {
        return false;
    }
    return matches_locale_chain(locale, points, count, folded) ||
           locale_matches(find_locale("en"), points, count, folded);
}

/** Return whether a scalar has both standardized presentation selectors. */
bool characters_emoji_is_presentation_base(uint32_t point) {
    size_t low = 0;
    size_t high = COUNT(presentation_bases);
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (point == presentation_bases[middle]) {
            return true;
        }
        if (point < presentation_bases[middle]) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }
    return false;
}

/** Return the exact noncatalog identity index, never a normalized approximation. */
size_t characters_emoji_alias_index(const char *text) {
    if (!text || strnlen(text, MAX_TEXT_BYTES + 1) > MAX_TEXT_BYTES) {
        return SIZE_MAX;
    }
    size_t low = 0;
    size_t high = COUNT(sequence_aliases);
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        int comparison = strcmp(text, sequence_aliases[middle].text);
        if (!comparison) {
            return middle;
        }
        if (comparison < 0) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }
    return SIZE_MAX;
}

/** Return the number of exact supplementary sequence identities. */
size_t characters_emoji_alias_count(void) {
    return COUNT(sequence_aliases);
}

/** Return the exact noncatalog sequence, or NULL for invalid indexes. */
const char *characters_emoji_alias_text(size_t index) {
    return index < COUNT(sequence_aliases) ? sequence_aliases[index].text : NULL;
}

/** Return the associated fully qualified index when one exists. */
size_t characters_emoji_alias_canonical(size_t index) {
    return index < COUNT(sequence_aliases) ? sequence_aliases[index].canonical : SIZE_MAX;
}

/** Return a stable label for a noncatalog sequence. */
const char *characters_emoji_alias_label(size_t index) {
    const char *text = characters_emoji_alias_text(index);
    uint32_t points[MAX_POINTS];
    size_t count = decode(text, points);
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
char *characters_emoji_alias_name(size_t index) {
    const char *label = characters_emoji_alias_label(index);
    const char *text = characters_emoji_alias_text(index);
    uint32_t points[MAX_POINTS];
    size_t count = decode(text, points);
    if (!label || !count) {
        return NULL;
    }
    if (count > 2 || (count == 2 && points[1] != 0xFE0E && points[1] != 0xFE0F)) {
        return strdup(label);
    }
    char *name = characters_unicode_name(points[0]);
    if (!name) {
        return strdup(label);
    }
    size_t capacity = strlen(name) + strlen(label) + 4;
    char *result = malloc(capacity);
    if (result) {
        snprintf(result, capacity, "%s (%s)", name, label);
    }
    free(name);
    return result;
}

/** Expose annotation provenance independently of the Unicode character repertoire. */
const char *characters_emoji_annotation_source(void) {
    return EMOJI_ANNOTATION_SOURCE;
}

/** Validate the complete input before attempting catalog resolution. */
bool characters_emoji_is_candidate(const char *text) {
    uint32_t points[MAX_POINTS];
    size_t count = decode(text, points);
    return count && (characters_unicode_emoji_index(points, count) != SIZE_MAX ||
                     characters_emoji_alias_index(text) != SIZE_MAX);
}

