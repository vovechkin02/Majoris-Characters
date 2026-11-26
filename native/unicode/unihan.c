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
static bool fold(const char *term, char *output, size_t capacity) {
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

/** Fold one query term using the same bounded ICU operation as the previous scanner. */
char *characters_unihan_fold_text(const char *term) {
    char output[512 * 3 + 1];
    return fold(term, output, sizeof(output)) ? strdup(output) : NULL;
}

/** Match selected immutable folded fields without allocation or prefix interpretation. */
bool characters_unihan_match_fields(uint32_t point, uint32_t fields, const char *term) {
    if (!term || !*term || !(fields & CHARACTERS_HAN_TEXT_FIELDS)) {
        return false;
    }
    const struct HanRecord *row = lookup(point);
    if (!row) {
        return false;
    }
    fields &= CHARACTERS_HAN_TEXT_FIELDS;
    for (size_t field = 0; field < HAN_FIELD_COUNT; field++) {
        if ((fields & (UINT32_C(1) << field)) && row->folded[field] &&
            strstr(han_strings + row->folded[field], term)) {
            return true;
        }
    }
    return false;
}

/** Match an explicit numeric constraint using the same group data as browsing. */
bool characters_unihan_match_group(uint32_t point, bool radical, unsigned group) {
    if (!group || group > (radical ? 214u : 128u)) {
        return false;
    }
    const struct HanRecord *row = lookup(point);
    return row && group_matches(row, radical, group);
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
