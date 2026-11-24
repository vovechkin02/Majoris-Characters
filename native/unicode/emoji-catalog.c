#include "emoji-catalog.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

/** An exact copyable alternative and its display label. */
struct Variant {
    /** Owned UTF-8 identity. */
    char *text;
    /** Borrowed immutable label. */
    const char *label;
};

/** Owns all alternatives until the caller frees the collection. */
struct CharactersEmojiVariants {
    /** Owned array, allocated to catalog size plus presentation alternatives. */
    struct Variant *items;
    /** Number of initialized entries. */
    size_t count;
};

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

/** Find a matching locale by exact normalized tag, then its primary language. */
static const struct AnnotationLocale *find_locale(const char *locale) {
    char language[64] = { 0 };
    if (locale) {
        size_t length = strcspn(locale, ".@:");
        if (length < sizeof(language)) {
            memcpy(language, locale, length);
            for (size_t index = 0; index < length; index++) {
                if (language[index] == '-') {
                    language[index] = '_';
                }
            }
        }
    }
    while (*language) {
        for (size_t index = 0; index < COUNT(annotation_locales); index++) {
            if (strcmp(language, annotation_locales[index].language) == 0) {
                return &annotation_locales[index];
            }
        }
        char *separator = strrchr(language, '_');
        if (!separator) {
            break;
        }
        *separator = '\0';
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
    const struct AnnotationLocale *local = find_locale(locale);
    const struct AnnotationLocale *english = find_locale("en");
    return locale_matches(local, points, count, folded) ||
           (local != english && locale_matches(english, points, count, folded));
}

/** Resolve only exact catalog entries or a standardized two-scalar presentation pair. */
static size_t resolve(const uint32_t *points, size_t count) {
    size_t index = characters_unicode_emoji_index(points, count);
    if (index != SIZE_MAX) {
        return index;
    }
    if (count == 1 || (count == 2 && points[1] == 0xFE0E)) {
        uint32_t pair[] = { points[0], 0xFE0F };
        return characters_unicode_emoji_index(pair, COUNT(pair));
    }
    return SIZE_MAX;
}

/** Expose annotation provenance independently of the Unicode character repertoire. */
const char *characters_emoji_annotation_source(void) {
    return EMOJI_ANNOTATION_SOURCE;
}

/** Validate the complete input before attempting catalog resolution. */
bool characters_emoji_is_candidate(const char *text) {
    uint32_t points[MAX_POINTS];
    size_t count = decode(text, points);
    return count && resolve(points, count) != SIZE_MAX;
}

/** Compute a family key without synthesizing any sequence; only catalog entries are compared. */
static size_t family_key(const uint32_t *points, size_t count, const char *label,
                         uint32_t *key) {
    if (strncmp(label, "FAMILY", 6) == 0) {
        key[0] = 0xFFFFFFFFu;
        return 1;
    }
    if (strncmp(label, "HANDSHAKE", 9) == 0) {
        key[0] = 0x1F91D;
        return 1;
    }
    if (strncmp(label, "COUPLE WITH HEART", 17) == 0) {
        key[0] = 0x1F491;
        return 1;
    }
    if (strcmp(label, "KISS") == 0 || strncmp(label, "KISS:", 5) == 0) {
        key[0] = 0x1F48F;
        return 1;
    }
    if (strstr(label, " HOLDING HANDS")) {
        key[0] = 0xFFFFFFFDu;
        return 1;
    }
    if (strncmp(label, "KEYCAP:", 7) == 0) {
        key[0] = 0x20E3;
        return 1;
    }
    if (strncmp(label, "FLAG:", 5) == 0) {
        key[0] = 0xFFFFFFFEu;
        return 1;
    }
    size_t length = 0;
    for (size_t index = 0; index < count; index++) {
        uint32_t point = points[index];
        if (point == 0xFE0F || point == 0xFE0E || is_tone(point)) {
            continue;
        }
        if (point == 0x200D && index + 1 < count) {
            uint32_t next = points[index + 1];
            if (next == 0x2640 || next == 0x2642 || next == 0x27A1 ||
                (next >= 0x1F9B0 && next <= 0x1F9B3)) {
                index++;
                continue;
            }
        }
        if (point == 0x1F468 || point == 0x1F469) {
            point = 0x1F9D1;
        } else if (point == 0x1F466 || point == 0x1F467) {
            point = 0x1F9D2;
        } else if (point == 0x1F474 || point == 0x1F475) {
            point = 0x1F9D3;
        }
        key[length++] = point;
    }
    return length;
}

/** Append an owned alternative once, preserving exact sequence identity. */
static bool append_variant(CharactersEmojiVariants *variants, char *text, const char *label) {
    if (!text) {
        return false;
    }
    for (size_t index = 0; index < variants->count; index++) {
        if (strcmp(variants->items[index].text, text) == 0) {
            free(text);
            return true;
        }
    }
    variants->items[variants->count++] = (struct Variant){ text, label };
    return true;
}

/** Allocate one self-contained collection, with selected canonical identity first. */
CharactersEmojiVariants *characters_emoji_variants_new(const char *text) {
    uint32_t points[MAX_POINTS];
    size_t count = decode(text, points);
    size_t selected = count ? resolve(points, count) : SIZE_MAX;
    if (selected == SIZE_MAX) {
        return NULL;
    }
    char *canonical = characters_unicode_emoji_text(selected);
    count = decode(canonical, points);
    if (!count) {
        free(canonical);
        return NULL;
    }
    uint32_t key[MAX_POINTS];
    const char *label = characters_unicode_emoji_label(selected);
    size_t key_count = family_key(points, count, label, key);
    CharactersEmojiVariants *variants = calloc(1, sizeof(*variants));
    if (!variants) {
        free(canonical);
        return NULL;
    }
    variants->items = calloc(characters_unicode_emoji_count() + 3, sizeof(struct Variant));
    if (!variants->items) {
        free(canonical);
        characters_emoji_variants_free(variants);
        return NULL;
    }
    bool success = append_variant(variants, canonical, label);
    if (success && count == 2 && points[1] == 0xFE0F) {
        uint32_t presentation[] = { points[0], 0xFE0E };
        success = append_variant(variants, characters_unicode_encode(presentation, 2),
                                 "Text presentation") &&
                  append_variant(variants, characters_unicode_encode(points, 1),
                                 "Default presentation");
    }
    for (size_t index = 0; success && index < characters_unicode_emoji_count(); index++) {
        if (index == selected) {
            continue;
        }
        char *candidate = characters_unicode_emoji_text(index);
        uint32_t candidate_points[MAX_POINTS];
        uint32_t candidate_key[MAX_POINTS];
        size_t candidate_count = decode(candidate, candidate_points);
        const char *candidate_label = characters_unicode_emoji_label(index);
        if (!candidate_count) {
            free(candidate);
            success = false;
            break;
        }
        size_t candidate_length = family_key(candidate_points, candidate_count,
                                             candidate_label, candidate_key);
        if (candidate_length == key_count &&
            memcmp(candidate_key, key, key_count * sizeof(*key)) == 0) {
            success = append_variant(variants, candidate, candidate_label);
        } else {
            free(candidate);
        }
    }
    if (!success) {
        characters_emoji_variants_free(variants);
        return NULL;
    }
    return variants;
}

/** Release all owned strings and storage, including partially built collections. */
void characters_emoji_variants_free(CharactersEmojiVariants *variants) {
    if (variants) {
        for (size_t index = 0; index < variants->count; index++) {
            free(variants->items[index].text);
        }
        free(variants->items);
        free(variants);
    }
}

/** Expose only initialized alternatives. */
size_t characters_emoji_variants_count(const CharactersEmojiVariants *variants) {
    return variants ? variants->count : 0;
}

/** Borrow the exact text for the lifetime of the collection. */
const char *characters_emoji_variants_text(const CharactersEmojiVariants *variants, size_t index) {
    return index < characters_emoji_variants_count(variants) ? variants->items[index].text : NULL;
}

/** Borrow the catalog or presentation label for a valid alternative. */
const char *characters_emoji_variants_label(const CharactersEmojiVariants *variants, size_t index) {
    return index < characters_emoji_variants_count(variants) ? variants->items[index].label : NULL;
}
