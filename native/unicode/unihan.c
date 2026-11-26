#include "unihan.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/uscript.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>

#include "../allocation.h"

#define MAX_TERM_UNITS 2048

/** Indexed fields shared with the offline generator. */
enum HanField {
    HAN_MANDARIN, HAN_CANTONESE, HAN_JAPANESE_ON, HAN_JAPANESE_KUN, HAN_JAPANESE,
    HAN_KOREAN, HAN_HANGUL, HAN_DEFINITION, HAN_RADICAL, HAN_STROKES,
    HAN_MANDARIN_FALLBACK, HAN_HANGUL_FALLBACK, HAN_FIELD_COUNT
};

/** One scalar's immutable metadata and non-authoritative collation groups. */
struct HanRecord {
    gunichar point;
    guint16 radical;
    guint8 strokes;
    guint32 values[HAN_FIELD_COUNT];
    guint32 folded[HAN_FIELD_COUNT];
};

/** Exact property keys, in the same order as HanField. */
static const gchar *const field_names[] = {
    "kMandarin", "kCantonese", "kJapaneseOn", "kJapaneseKun", "kJapanese", "kKorean",
    "kHangul", "kDefinition", "kRSUnicode", "kTotalStrokes",
    "Mandarin_Fallback", "Hangul_Fallback"
};

#include "generated/unihan.h"

/** Compare a scalar against the sorted record table. */
static gint compare_record(gconstpointer key, gconstpointer item)
{
    gunichar point = *(const guint32 *)key;
    const struct HanRecord *row = item;
    return (point > row->point) - (point < row->point);
}

/** Borrow immutable metadata without allocation; absent scalars have no row. */
static const struct HanRecord *lookup(gunichar point)
{
    return bsearch(&point, han_records, G_N_ELEMENTS(han_records), sizeof(han_records[0]),
                   compare_record);
}

/** Return a decimal group label as owned text, or NULL for an unknown group. */
static gchar *group_text(guint value)
{
    if (!value) {
        return NULL;
    }
    gchar text[16];
    g_snprintf(text, sizeof(text), "%u", value);
    return characters_try_strdup(text);
}

/** Return an owned property value without substituting a different source. */
gchar *characters_unihan_property(gunichar point, const gchar *property)
{
    const struct HanRecord *row = lookup(point);
    if (!row || !property) {
        return NULL;
    }
    for (gsize index = 0; index < G_N_ELEMENTS(field_names); index++) {
        if (strcmp(property, field_names[index]) == 0) {
            return row->values[index] ? characters_try_strdup(han_strings + row->values[index]) : NULL;
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
const gchar *characters_unihan_version(void)
{
    return UNIHAN_OFFICIAL_VERSION;
}

/** Normalize bounded reading search text without changing original metadata. */
static gboolean fold(const gchar *term, gchar *output, gsize capacity)
{
    if (!term || !output || !capacity || capacity > INT32_MAX) {
        return FALSE;
    }
    UErrorCode status = U_ZERO_ERROR;
    UChar input[MAX_TERM_UNITS];
    UChar folded[MAX_TERM_UNITS];
    UChar decomposed[MAX_TERM_UNITS];
    int32_t length;
    u_strFromUTF8(input, G_N_ELEMENTS(input), &length, term, -1, &status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    length = u_strFoldCase(folded, G_N_ELEMENTS(folded), input, length, U_FOLD_CASE_DEFAULT, &status);
    const UNormalizer2 *normalizer = unorm2_getNFDInstance(&status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    length = unorm2_normalize(normalizer, folded, length, decomposed, G_N_ELEMENTS(decomposed), &status);
    if (U_FAILURE(status)) {
        return FALSE;
    }
    int32_t used = 0;
    gboolean latin = FALSE;
    for (int32_t index = 0; index < length;) {
        UChar32 point;
        U16_NEXT(decomposed, index, length, point);
        gint type = u_charType(point);
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
    return U_SUCCESS(status) && length > 0 && (gsize)length < capacity;
}

/** Match one numeric group in the formal whitespace-separated property values. */
static gboolean formal_group(const gchar *values, gboolean radical, guint group)
{
    while (*values) {
        gchar *end;
        gulong value = strtoul(values, &end, 10);
        if (end != values && value == group &&
            (radical ? (*end == '.' || *end == '\'') : (*end == ' ' || !*end))) {
            return TRUE;
        }
        const gchar *next = strchr(values, ' ');
        if (!next) {
            break;
        }
        values = next + 1;
    }
    return FALSE;
}

/** Apply formal grouping when present, otherwise the explicitly labeled collation fallback. */
static gboolean group_matches(const struct HanRecord *row, gboolean radical, guint group)
{
    guint32 offset = row->values[radical ? HAN_RADICAL : HAN_STROKES];
    if (offset) {
        return !group || formal_group(han_strings + offset, radical, group);
    }
    guint fallback = radical ? row->radical : row->strokes;
    return fallback && (!group || fallback == group);
}

/** Fold one query term using the same bounded ICU operation as the previous scanner. */
gchar *characters_unihan_fold_text(const gchar *term)
{
    gchar output[512 * 3 + 1];
    return fold(term, output, sizeof(output)) ? characters_try_strdup(output) : NULL;
}

/** Match selected immutable folded fields without allocation or prefix interpretation. */
gboolean characters_unihan_match_fields(gunichar point, guint32 fields, const gchar *term)
{
    if (!term || !*term || !(fields & CHARACTERS_HAN_TEXT_FIELDS)) {
        return FALSE;
    }
    const struct HanRecord *row = lookup(point);
    if (!row) {
        return FALSE;
    }
    fields &= CHARACTERS_HAN_TEXT_FIELDS;
    for (gsize field = 0; field < HAN_FIELD_COUNT; field++) {
        if ((fields & (UINT32_C(1) << field)) && row->folded[field] &&
            strstr(han_strings + row->folded[field], term)) {
            return TRUE;
        }
    }
    return FALSE;
}

/** Match an explicit numeric constraint using the same group data as browsing. */
gboolean characters_unihan_match_group(gunichar point, gboolean radical, guint group)
{
    if (!group || group > (radical ? 214u : 128u)) {
        return FALSE;
    }
    const struct HanRecord *row = lookup(point);
    return row && group_matches(row, radical, group);
}

/** Return the number of group filter slots. */
gsize characters_unihan_group_count(gboolean radical)
{
    return radical ? G_N_ELEMENTS(radical_labels) : G_N_ELEMENTS(stroke_labels);
}

/** Borrow the immutable display label of a group. */
const gchar *characters_unihan_group_name(gboolean radical, gsize index)
{
    if (index >= characters_unihan_group_count(radical)) {
        return NULL;
    }
    return radical ? radical_labels[index] : stroke_labels[index];
}

/** Match a valid filter slot against the formal or fallback group. */
gboolean characters_unihan_group_matches(gunichar point, gboolean radical, gint index)
{
    if (index < -1 || (index >= 0 && (gsize)index >= characters_unihan_group_count(radical))) {
        return FALSE;
    }
    const struct HanRecord *row = lookup(point);
    guint group = index < 0 ? 0 : radical ? (guint)index + 1 : stroke_values[index];
    return row && group_matches(row, radical, group);
}
