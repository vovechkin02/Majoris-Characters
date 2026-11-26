#include "legacy-encoding.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>
#include <unicode/uset.h>

#include "icu-scalar-conversion.h"
#include "unicode-engine.h"

/** Maximum temporary mapping entries, including independent aliases. */
#define MAX_MAPPINGS 2000000u
/** Shared inspector scalar limit. */
#define MAX_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
/** Maximum stream byte length. */
#define MAX_BYTES CHARACTERS_LEGACY_MAX_BYTES

/** Encoded-unit grammar; contextual converters use standalone round-trip scalars. */
enum MappingKind {
    /** Enumerate independent one-byte units. */
    SINGLE,
    /** Enumerate independent one- and two-byte units. */
    DOUBLE,
    /** Include JIS X 0212 three-byte units. */
    EUC_JP,
    /** Include GB18030 four-byte units. */
    GB18030,
    /** Include CNS 11643 four-byte plane units. */
    EUC_TW,
    /** Enumerate independently round-trippable scalars from a contextual converter. */
    CONTEXTUAL
};

/** Describes a supported ICU encoding and the repertoire enumeration strategy. */
struct Encoding {
    /** Stable display label. */
    const gchar *name;
    /** ICU converter alias or canonical name. */
    const gchar *converter;
    /** Grammar for independent mappings, or contextual repertoire mode. */
    enum MappingKind kind;
};

/** Explicit encodings supported by the Debian 13 ICU baseline. */
static const struct Encoding encodings[] = {
    { "US-ASCII", "US-ASCII", SINGLE },
    { "ISO-8859-1", "ISO-8859-1", SINGLE },
    { "ISO-8859-2", "ISO-8859-2", SINGLE },
    { "ISO-8859-3", "ISO-8859-3", SINGLE },
    { "ISO-8859-4", "ISO-8859-4", SINGLE },
    { "ISO-8859-5", "ISO-8859-5", SINGLE },
    { "ISO-8859-6", "ISO-8859-6", SINGLE },
    { "ISO-8859-7", "ISO-8859-7", SINGLE },
    { "ISO-8859-8", "ISO-8859-8", SINGLE },
    { "ISO-8859-9", "ISO-8859-9", SINGLE },
    { "ISO-8859-10", "ISO-8859-10", SINGLE },
    { "ISO-8859-13", "ISO-8859-13", SINGLE },
    { "ISO-8859-14", "ISO-8859-14", SINGLE },
    { "ISO-8859-15", "ISO-8859-15", SINGLE },
    { "Windows-1250", "windows-1250", SINGLE },
    { "Windows-1251", "windows-1251", SINGLE },
    { "Windows-1252", "windows-1252", SINGLE },
    { "Windows-1253", "windows-1253", SINGLE },
    { "Windows-1254", "windows-1254", SINGLE },
    { "Windows-1255", "windows-1255", SINGLE },
    { "Windows-1256", "windows-1256", SINGLE },
    { "Windows-1257", "windows-1257", SINGLE },
    { "Windows-1258", "windows-1258", SINGLE },
    { "Windows-874", "windows-874-2000", SINGLE },
    { "IBM 437", "ibm-437", SINGLE },
    { "IBM 850", "ibm-850", SINGLE },
    { "IBM 852", "ibm-852", SINGLE },
    { "IBM 866", "ibm-866", SINGLE },
    { "KOI8-R", "KOI8-R", SINGLE },
    { "KOI8-U", "KOI8-U", SINGLE },
    { "Macintosh Roman", "macintosh", SINGLE },
    { "Shift JIS (Windows-932)", "windows-31j", DOUBLE },
    { "GBK (Windows-936)", "windows-936-2000", DOUBLE },
    { "Korean (Windows-949)", "windows-949-2000", DOUBLE },
    { "Big5 (Windows-950)", "windows-950-2000", DOUBLE },
    { "ISO-8859-11", "ISO-8859-11", SINGLE },
    { "IBM 720", "ibm-720", SINGLE },
    { "IBM 737", "ibm-737", SINGLE },
    { "IBM 775", "ibm-775", SINGLE },
    { "IBM 855", "ibm-855", SINGLE },
    { "IBM 857", "ibm-857", SINGLE },
    { "IBM 858", "ibm-858", SINGLE },
    { "IBM 860", "ibm-860", SINGLE },
    { "IBM 861", "ibm-861", SINGLE },
    { "IBM 862", "ibm-862", SINGLE },
    { "IBM 863", "ibm-863", SINGLE },
    { "IBM 864", "ibm-864", SINGLE },
    { "IBM 865", "ibm-865", SINGLE },
    { "IBM 869", "ibm-869", SINGLE },
    { "IBM 1125", "ibm-1125", SINGLE },
    { "EBCDIC 037", "ibm-37", SINGLE },
    { "EBCDIC 500", "ibm-500", SINGLE },
    { "EBCDIC 1047", "ibm-1047", SINGLE },
    { "EBCDIC 1140", "ibm-1140", SINGLE },
    { "Macintosh Cyrillic", "x-mac-cyrillic", SINGLE },
    { "GB2312 (EUC-CN)", "GB2312", DOUBLE },
    { "EUC-KR", "EUC-KR", DOUBLE },
    { "Big5-HKSCS (ICU)", "Big5-HKSCS", DOUBLE },
    { "EUC-JP", "EUC-JP", EUC_JP },
    { "GB18030-2022", "gb18030-2022", GB18030 },
    { "EUC-TW", "EUC-TW", EUC_TW },
    { "ISO-2022-JP", "ISO-2022-JP", CONTEXTUAL },
    { "ISO-2022-JP-1", "ISO-2022-JP-1", CONTEXTUAL },
    { "ISO-2022-JP-2", "ISO-2022-JP-2", CONTEXTUAL },
    { "ISO-2022-KR", "ISO-2022-KR", CONTEXTUAL },
    { "ISO-2022-CN", "ISO-2022-CN", CONTEXTUAL },
    { "ISO-2022-CN-EXT", "ISO-2022-CN-EXT", CONTEXTUAL },
    { "HZ-GB-2312", "HZ", CONTEXTUAL },
    { "ISCII Devanagari", "ISCII,version=0", CONTEXTUAL },
    { "ISCII Bengali", "ISCII,version=1", CONTEXTUAL },
    { "ISCII Punjabi", "ISCII,version=2", CONTEXTUAL },
    { "ISCII Gujarati", "ISCII,version=3", CONTEXTUAL },
    { "ISCII Oriya", "ISCII,version=4", CONTEXTUAL },
    { "ISCII Tamil", "ISCII,version=5", CONTEXTUAL },
    { "ISCII Telugu", "ISCII,version=6", CONTEXTUAL },
    { "ISCII Kannada", "ISCII,version=7", CONTEXTUAL },
    { "ISCII Malayalam", "ISCII,version=8", CONTEXTUAL },
};

/** Associates a scalar with an exact byte unit (length in high bits, value in low bits). */
struct Mapping {
    /** Decoded scalar, including NUL. */
    gunichar point;
    /** Packed bytes; zero denotes a contextual canonical mapping generated on demand. */
    guint64 key;
};

/** Owns a bounded sorted mapping vector and its immutable encoding identity. */
struct CharactersLegacy {
    /** Initialized mapping entries. */
    struct Mapping *entries;
    /** Number of entries, including independent byte alternatives. */
    gsize count;
    /** Allocated entries, used only during construction. */
    gsize capacity;
    /** Stable catalog index used for exact conversion. */
    gsize encoding;
};

/** Open a fresh converter with strict callbacks in both directions. */
static UConverter *open_converter(gsize index)
{
    if (index >= G_N_ELEMENTS(encodings)) {
        return NULL;
    }
    UErrorCode error = U_ZERO_ERROR;
    UConverter *converter = ucnv_open(encodings[index].converter, &error);
    if (U_FAILURE(error) || !converter) {
        return NULL;
    }
    ucnv_setFallback(converter, FALSE);
    ucnv_setToUCallBack(converter, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &error);
    ucnv_setFromUCallBack(converter, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, &error);
    if (U_FAILURE(error)) {
        ucnv_close(converter);
        return NULL;
    }
    return converter;
}

/** Decode a complete independent input from initial state, preserving exact scalar order. */
static gsize decode_points(UConverter *converter, const gchar *bytes, gsize length,
                            gunichar points[MAX_POINTS])
{
    UChar output[MAX_POINTS * 2];
    UErrorCode error = U_ZERO_ERROR;
    int32_t count = ucnv_toUChars(converter, output, G_N_ELEMENTS(output), bytes,
                                 (int32_t)length, &error);
    if (U_FAILURE(error) || count <= 0) {
        return 0;
    }
    return characters_icu_decode_utf16(output, count, points, MAX_POINTS);
}

/** Encode explicit scalars with strict callbacks, without imposing round-trip policy. */
static gsize encode_points(UConverter *converter, const gunichar *points, gsize length,
                            gchar bytes[MAX_BYTES])
{
    if (!points || !length || length > MAX_POINTS) {
        return 0;
    }
    UChar input[MAX_POINTS * 2];
    int32_t used = characters_icu_encode_utf16(points, length, input, G_N_ELEMENTS(input));
    if (!used) {
        return 0;
    }
    UErrorCode error = U_ZERO_ERROR;
    int32_t count = ucnv_fromUChars(converter, bytes, MAX_BYTES, input, used, &error);
    if (U_FAILURE(error) || count <= 0) {
        return 0;
    }
    return (gsize)count;
}

/** Verify standalone contextual repertoire membership without allocating intermediate arrays. */
static gboolean round_trip_scalar(UConverter *converter, gunichar point)
{
    gchar bytes[MAX_BYTES];
    gsize count = encode_points(converter, &point, 1, bytes);
    guint32 decoded[MAX_POINTS];
    return count && decode_points(converter, bytes, count, decoded) == 1 && decoded[0] == point;
}

/** Keep alternatives contiguous, ordered by encoded length then unsigned byte value. */
static gint compare_mappings(gconstpointer left, gconstpointer right)
{
    const struct Mapping *a = left;
    const struct Mapping *b = right;
    if (a->point != b->point) {
        return (a->point > b->point) - (a->point < b->point);
    }
    return (a->key > b->key) - (a->key < b->key);
}

/** Find the first entry whose scalar is at least point. */
static gsize lower_bound(const CharactersLegacy *table, gunichar point)
{
    gsize low = 0;
    gsize high = table->count;
    while (low < high) {
        gsize middle = low + (high - low) / 2;
        if (table->entries[middle].point < point) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

/** Grow the temporary vector with a hard limit; allocation failures abort publication. */
static gboolean append_mapping(CharactersLegacy *table, gunichar point, guint64 key)
{
    if (table->count == table->capacity) {
        gsize capacity = table->capacity ? table->capacity * 2 : 256;
        if (capacity > MAX_MAPPINGS) {
            capacity = MAX_MAPPINGS;
        }
        if (capacity == table->capacity) {
            return FALSE;
        }
        struct Mapping *entries = g_try_realloc_n(table->entries, capacity, sizeof(*entries));
        if (!entries) {
            return FALSE;
        }
        table->entries = entries;
        table->capacity = capacity;
    }
    table->entries[table->count++] = (struct Mapping){ point, key };
    return TRUE;
}

/** Add one independent encoded unit if it decodes to exactly one scalar. */
static gboolean add_unit(CharactersLegacy *table, UConverter *converter, guint32 value,
                     guint length)
{
    gchar bytes[4];
    for (guint index = 0; index < length; index++) {
        bytes[index] = (gchar)(value >> (8 * (length - index - 1)));
    }
    gunichar points[MAX_POINTS];
    if (decode_points(converter, bytes, length, points) != 1) {
        return TRUE;
    }
    return append_mapping(table, points[0], ((guint64)length << 32) | value);
}

/** Enumerate all single/double units, then the legal longer byte grammars only. */
static gboolean enumerate_units(CharactersLegacy *table, UConverter *converter,
                            enum MappingKind kind)
{
    for (guint32 value = 0; value < 256; value++) {
        if (!add_unit(table, converter, value, 1)) {
            return FALSE;
        }
    }
    if (kind == SINGLE) {
        return TRUE;
    }
    for (guint32 value = 0; value < 65536; value++) {
        if (!add_unit(table, converter, value, 2)) {
            return FALSE;
        }
    }
    if (kind == EUC_JP) {
        for (guint32 row = 0xA1; row <= 0xFE; row++) {
            for (guint32 cell = 0xA1; cell <= 0xFE; cell++) {
                if (!add_unit(table, converter, 0x8F0000 | row << 8 | cell, 3)) {
                    return FALSE;
                }
            }
        }
    } else if (kind == GB18030) {
        for (guint32 lead = 0x81; lead <= 0xFE; lead++) {
            for (guint32 second = 0x30; second <= 0x39; second++) {
                for (guint32 third = 0x81; third <= 0xFE; third++) {
                    for (guint32 last = 0x30; last <= 0x39; last++) {
                        guint32 value = lead << 24 | second << 16 | third << 8 | last;
                        if (!add_unit(table, converter, value, 4)) {
                            return FALSE;
                        }
                    }
                }
            }
        }
    } else if (kind == EUC_TW) {
        for (guint32 plane = 0xA1; plane <= 0xB0; plane++) {
            for (guint32 row = 0xA1; row <= 0xFE; row++) {
                for (guint32 cell = 0xA1; cell <= 0xFE; cell++) {
                    guint32 value = 0x8E000000 | plane << 16 | row << 8 | cell;
                    if (!add_unit(table, converter, value, 4)) {
                        return FALSE;
                    }
                }
            }
        }
    }
    return TRUE;
}

/** Contextual encodings expose strictly round-trippable standalone scalars, not arbitrary streams. */
static gboolean enumerate_contextual(CharactersLegacy *table, UConverter *converter)
{
    USet *set = uset_openEmpty();
    if (!set) {
        return FALSE;
    }
    UErrorCode error = U_ZERO_ERROR;
    ucnv_getUnicodeSet(converter, set, UCNV_ROUNDTRIP_SET, &error);
    gboolean success = U_SUCCESS(error);
    for (int32_t range = 0; success && range < uset_getRangeCount(set); range++) {
        UChar32 start, end;
        uset_getItem(set, range, &start, &end, NULL, 0, &error);
        if (U_FAILURE(error)) {
            success = FALSE;
            break;
        }
        for (UChar32 value = start; value <= end; value++) {
            gunichar point = (guint32)value;
            if (round_trip_scalar(converter, point) && !append_mapping(table, point, 0)) {
                success = FALSE;
                break;
            }
        }
    }
    uset_close(set);
    return success;
}

gsize characters_legacy_count(void)
{
    return G_N_ELEMENTS(encodings);
}

const gchar *characters_legacy_name(gsize index)
{
    return index < G_N_ELEMENTS(encodings) ? encodings[index].name : NULL;
}

gboolean characters_legacy_is_contextual(gsize index)
{
    return index < G_N_ELEMENTS(encodings) && encodings[index].kind == CONTEXTUAL;
}

CharactersLegacy *characters_legacy_load(gsize index)
{
    UConverter *converter = open_converter(index);
    if (!converter) {
        return NULL;
    }
    CharactersLegacy *table = g_try_new0(CharactersLegacy, 1);
    if (table) {
        table->encoding = index;
        enum MappingKind kind = encodings[index].kind;
        gboolean success = kind == CONTEXTUAL ? enumerate_contextual(table, converter) :
            enumerate_units(table, converter, kind);
        if (!success) {
            characters_legacy_free(table);
            table = NULL;
        } else if (table->count) {
            qsort(table->entries, table->count, sizeof(*table->entries), compare_mappings);
        }
    }
    ucnv_close(converter);
    return table;
}

void characters_legacy_free(CharactersLegacy *table)
{
    if (table) {
        g_free(table->entries);
        g_free(table);
    }
}

gunichar *characters_legacy_points(const CharactersLegacy *table, gsize *length)
{
    if (!length) {
        return NULL;
    }
    *length = 0;
    if (!table) {
        return NULL;
    }
    gunichar *points = g_try_malloc_n((table->count ? table->count : 1), sizeof(*points));
    if (!points) {
        return NULL;
    }
    for (gsize index = 0; index < table->count; index++) {
        gunichar point = table->entries[index].point;
        if (!*length || points[*length - 1] != point) {
            points[(*length)++] = point;
        }
    }
    return points;
}

gunichar *characters_legacy_decode_raw(gsize encoding, const guint8 *bytes, gsize count,
                                       gsize *length)
{
    if (!length) {
        return NULL;
    }
    *length = 0;
    if (!bytes || !count || count > MAX_BYTES) {
        return NULL;
    }
    UConverter *converter = open_converter(encoding);
    if (!converter) {
        return NULL;
    }
    guint32 decoded[MAX_POINTS];
    gsize used = decode_points(converter, (const gchar *)bytes, count, decoded);
    ucnv_close(converter);
    if (!used) {
        return NULL;
    }
    gunichar *points = g_try_malloc_n(used, sizeof(*points));
    if (points) {
        memcpy(points, decoded, used * sizeof(*points));
        *length = used;
    }
    return points;
}

guint8 *characters_legacy_encode_raw(gsize encoding, const gunichar *points, gsize length,
                                     gsize *count)
{
    if (!count) {
        return NULL;
    }
    *count = 0;
    UConverter *converter = open_converter(encoding);
    if (!converter) {
        return NULL;
    }
    gchar bytes[MAX_BYTES];
    gsize used = encode_points(converter, points, length, bytes);
    ucnv_close(converter);
    if (!used) {
        return NULL;
    }
    guint8 *output = g_try_malloc(used);
    if (output) {
        memcpy(output, bytes, used);
        *count = used;
    }
    return output;
}

gsize characters_legacy_encoding_index(const CharactersLegacy *table)
{
    return table ? table->encoding : G_MAXSIZE;
}

gboolean characters_legacy_contains(const CharactersLegacy *table, gunichar point)
{
    if (!table || !characters_unicode_is_scalar(point)) {
        return FALSE;
    }
    gsize index = lower_bound(table, point);
    return index < table->count && table->entries[index].point == point;
}

guint64 *characters_legacy_keys(const CharactersLegacy *table, gunichar point, gsize *length)
{
    if (!length) {
        return NULL;
    }
    *length = 0;
    if (!table || !characters_unicode_is_scalar(point) ||
        characters_legacy_is_contextual(table->encoding)) {
        return NULL;
    }
    gsize first = lower_bound(table, point);
    gsize end = lower_bound(table, point + 1);
    if (first == end) {
        return NULL;
    }
    guint64 *keys = g_try_malloc_n((end - first), sizeof(*keys));
    if (keys) {
        for (gsize index = first; index < end; index++) {
            keys[index - first] = table->entries[index].key;
        }
        *length = end - first;
    }
    return keys;
}
