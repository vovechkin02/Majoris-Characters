#include "unihan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/uscript.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define MAX_TERM_UNITS 2048

/** Indexed fields shared with the offline generator. */
enum HanField {
    HAN_MANDARIN, HAN_CANTONESE, HAN_JAPANESE_ON, HAN_JAPANESE_KUN, HAN_JAPANESE,
    HAN_KOREAN, HAN_HANGUL, HAN_DEFINITION, HAN_RADICAL, HAN_STROKES,
    HAN_MANDARIN_FALLBACK, HAN_HANGUL_FALLBACK, HAN_FIELD_COUNT
};

/** One scalar's immutable metadata and non-authoritative collation groups. */
struct HanRecord {
    uint32_t point;
    uint16_t radical;
    uint8_t strokes;
    uint32_t values[HAN_FIELD_COUNT];
    uint32_t folded[HAN_FIELD_COUNT];
};

/** Exact property keys, in the same order as HanField. */
static const char *const field_names[] = {
    "kMandarin", "kCantonese", "kJapaneseOn", "kJapaneseKun", "kJapanese", "kKorean",
    "kHangul", "kDefinition", "kRSUnicode", "kTotalStrokes",
    "Mandarin_Fallback", "Hangul_Fallback"
};

#include "generated/unihan.h"

/** Compare a scalar against the sorted record table. */
static int compare_record(const void *key, const void *item) {
    uint32_t point = *(const uint32_t *)key;
    const struct HanRecord *row = item;
    return (point > row->point) - (point < row->point);
}

/** Borrow immutable metadata without allocation; absent scalars have no row. */
static const struct HanRecord *lookup(uint32_t point) {
    return bsearch(&point, han_records, COUNT(han_records), sizeof(han_records[0]),
                   compare_record);
}

/** Return a decimal group label as owned text, or NULL for an unknown group. */
static char *group_text(unsigned value) {
    if (!value) {
        return NULL;
    }
    char text[16];
    snprintf(text, sizeof(text), "%u", value);
    return strdup(text);
}

/** Return an owned property value without substituting a different source. */
char *characters_unihan_property(uint32_t point, const char *property) {
    const struct HanRecord *row = lookup(point);
    if (!row || !property) {
        return NULL;
    }
    for (size_t index = 0; index < COUNT(field_names); index++) {
        if (strcmp(property, field_names[index]) == 0) {
            return row->values[index] ? strdup(han_strings + row->values[index]) : NULL;
        }
    }
    if (strcmp(property, "Radical_Collation") == 0) {
        return group_text(row->radical);
    }
    if (strcmp(property, "Stroke_Collation") == 0) {
        return group_text(row->strokes);
    }
    return NULL;
}

/** Report the official data version separately from the fallback. */
const char *characters_unihan_version(void) {
    return UNIHAN_OFFICIAL_VERSION;
}

/** Normalize bounded reading search text without changing original metadata. */
bool characters_unihan_fold(const char *term, char *output, size_t capacity) {
    if (!term || !output || !capacity || capacity > INT32_MAX) {
        return false;
    }
    UErrorCode status = U_ZERO_ERROR;
    UChar input[MAX_TERM_UNITS];
    UChar folded[MAX_TERM_UNITS];
    UChar decomposed[MAX_TERM_UNITS];
    int32_t length;
    u_strFromUTF8(input, COUNT(input), &length, term, -1, &status);
    if (U_FAILURE(status)) {
        return false;
    }
    length = u_strFoldCase(folded, COUNT(folded), input, length, U_FOLD_CASE_DEFAULT, &status);
    const UNormalizer2 *normalizer = unorm2_getNFDInstance(&status);
    if (U_FAILURE(status)) {
        return false;
    }
    length = unorm2_normalize(normalizer, folded, length, decomposed, COUNT(decomposed), &status);
    if (U_FAILURE(status)) {
        return false;
    }
    int32_t used = 0;
    bool latin = false;
    for (int32_t index = 0; index < length;) {
        UChar32 point;
        U16_NEXT(decomposed, index, length, point);
        int type = u_charType(point);
        if (type != U_NON_SPACING_MARK && type != U_COMBINING_SPACING_MARK &&
            type != U_ENCLOSING_MARK) {
            UErrorCode script_status = U_ZERO_ERROR;
            latin = uscript_getScript(point, &script_status) == USCRIPT_LATIN &&
                    U_SUCCESS(script_status);
        }
        if (latin && (point == 0x0300 || point == 0x0301 ||
                      point == 0x0304 || point == 0x030C)) {
            continue;
        }
        U16_APPEND_UNSAFE(folded, used, point);
    }
    u_strToUTF8(output, (int32_t)capacity, &length, folded, used, &status);
    return U_SUCCESS(status) && length > 0 && (size_t)length < capacity;
}

/** Match one numeric group in the formal whitespace-separated property values. */
static bool formal_group(const char *values, bool radical, unsigned group) {
    while (*values) {
        char *end;
        unsigned long value = strtoul(values, &end, 10);
        if (end != values && value == group &&
            (radical ? (*end == '.' || *end == '\'') : (*end == ' ' || !*end))) {
            return true;
        }
        const char *next = strchr(values, ' ');
        if (!next) {
            break;
        }
        values = next + 1;
    }
    return false;
}

/** Apply formal grouping when present, otherwise the explicitly labeled collation fallback. */
static bool group_matches(const struct HanRecord *row, bool radical, unsigned group) {
    uint32_t offset = row->values[radical ? HAN_RADICAL : HAN_STROKES];
    if (offset) {
        return !group || formal_group(han_strings + offset, radical, group);
    }
    unsigned fallback = radical ? row->radical : row->strokes;
    return fallback && (!group || fallback == group);
}

/** Supported explicit query prefixes; Japanese and Korean each span multiple fields. */
static const char *const prefixes[] = {
    "mandarin:", "cantonese:", "japanese:", "korean:", "definition:",
    "radical:", "strokes:"
};

/** Locate an explicit field prefix; -1 for ordinary metadata terms. */
static int prefix_index(const char *term) {
    for (size_t index = 0; index < COUNT(prefixes); index++) {
        if (strncmp(term, prefixes[index], strlen(prefixes[index])) == 0) {
            return (int)index;
        }
    }
    return -1;
}

/** Recognize a supported Han-only query prefix. */
bool characters_unihan_is_field_query(const char *term) {
    return term && prefix_index(term) >= 0;
}

/** Match an individual field using its precomputed folded text. */
static bool field_matches(const struct HanRecord *row, size_t field, const char *term) {
    return row->folded[field] && strstr(han_strings + row->folded[field], term) != NULL;
}

/** Apply a field-specific or ordinary folded metadata query. */
bool characters_unihan_matches(uint32_t point, const char *term) {
    if (!term || !*term) {
        return false;
    }
    const struct HanRecord *row = lookup(point);
    if (!row) {
        return false;
    }
    int prefix = prefix_index(term);
    if (prefix >= 0) {
        term += strlen(prefixes[prefix]);
        if (!*term) {
            return false;
        }
    }
    switch (prefix) {
        case 0:
            return field_matches(row, HAN_MANDARIN, term) ||
                   field_matches(row, HAN_MANDARIN_FALLBACK, term);
        case 1:
            return field_matches(row, HAN_CANTONESE, term);
        case 2:
            return field_matches(row, HAN_JAPANESE_ON, term) ||
                   field_matches(row, HAN_JAPANESE_KUN, term) ||
                   field_matches(row, HAN_JAPANESE, term);
        case 3:
            return field_matches(row, HAN_KOREAN, term) || field_matches(row, HAN_HANGUL, term) ||
                   field_matches(row, HAN_HANGUL_FALLBACK, term);
        case 4:
            return field_matches(row, HAN_DEFINITION, term);
        case 5: case 6: {
            char *end;
            if (*term < '0' || *term > '9') {
                return false;
            }
            unsigned long group = strtoul(term, &end, 10);
            return !*end && group > 0 && group <= (prefix == 5 ? 214u : 128u) &&
                   group_matches(row, prefix == 5, (unsigned)group);
        }
        default:
            for (size_t field = 0; field < HAN_FIELD_COUNT; field++) {
                if (field != HAN_RADICAL && field != HAN_STROKES &&
                    field_matches(row, field, term)) {
                    return true;
                }
            }
            return false;
    }
}

/** Return the number of group filter slots. */
size_t characters_unihan_group_count(bool radical) {
    return radical ? COUNT(radical_labels) : COUNT(stroke_labels);
}

/** Borrow the immutable display label of a group. */
const char *characters_unihan_group_name(bool radical, size_t index) {
    if (index >= characters_unihan_group_count(radical)) {
        return NULL;
    }
    return radical ? radical_labels[index] : stroke_labels[index];
}

/** Match a valid filter slot against the formal or fallback group. */
bool characters_unihan_group_matches(uint32_t point, bool radical, int index) {
    if (index < -1 || (index >= 0 && (size_t)index >= characters_unihan_group_count(radical))) {
        return false;
    }
    const struct HanRecord *row = lookup(point);
    unsigned group = index < 0 ? 0 : radical ? (unsigned)index + 1 : stroke_values[index];
    return row && group_matches(row, radical, group);
}
