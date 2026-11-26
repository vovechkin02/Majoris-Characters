#include "legacy-encoding.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>
#include <unicode/uset.h>
#include <unicode/utf16.h>

#include "unicode-engine.h"

/** Count elements in a compile-time array. */
#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
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
    const char *name;
    /** ICU converter alias or canonical name. */
    const char *converter;
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
    uint32_t point;
    /** Packed bytes; zero denotes a contextual canonical mapping generated on demand. */
    uint64_t key;
};

/** Owns a bounded sorted mapping vector and its immutable encoding identity. */
struct CharactersLegacy {
    /** Initialized mapping entries. */
    struct Mapping *entries;
    /** Number of entries, including independent byte alternatives. */
    size_t count;
    /** Allocated entries, used only during construction. */
    size_t capacity;
    /** Stable catalog index used for exact conversion. */
    size_t encoding;
};

/** Open a fresh converter with strict callbacks in both directions. */
static UConverter *open_converter(size_t index) {
    if (index >= COUNT(encodings)) {
        return NULL;
    }
    UErrorCode error = U_ZERO_ERROR;
    UConverter *converter = ucnv_open(encodings[index].converter, &error);
    if (U_FAILURE(error) || !converter) {
        return NULL;
    }
    ucnv_setFallback(converter, false);
    ucnv_setToUCallBack(converter, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &error);
    ucnv_setFromUCallBack(converter, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, &error);
    if (U_FAILURE(error)) {
        ucnv_close(converter);
        return NULL;
    }
    return converter;
}

/** Decode a complete independent input from initial state, preserving exact scalar order. */
static size_t decode_points(UConverter *converter, const char *bytes, size_t length,
                            uint32_t points[MAX_POINTS]) {
    UChar output[MAX_POINTS * 2];
    UErrorCode error = U_ZERO_ERROR;
    int32_t count = ucnv_toUChars(converter, output, COUNT(output), bytes,
                                 (int32_t)length, &error);
    if (U_FAILURE(error) || count <= 0) {
        return 0;
    }
    int32_t offset = 0;
    size_t used = 0;
    while (offset < count) {
        UChar32 point;
        U16_NEXT(output, offset, count, point);
        if (used == MAX_POINTS || point < 0 || !characters_unicode_is_scalar((uint32_t)point)) {
            return 0;
        }
        points[used++] = (uint32_t)point;
    }
    return used;
}

/** Encode explicit scalars with strict callbacks, without imposing round-trip policy. */
static size_t encode_points(UConverter *converter, const uint32_t *points, size_t length,
                            char bytes[MAX_BYTES]) {
    if (!points || !length || length > MAX_POINTS) {
        return 0;
    }
    UChar input[MAX_POINTS * 2];
    int32_t used = 0;
    for (size_t index = 0; index < length; index++) {
        if (!characters_unicode_is_scalar(points[index])) {
            return 0;
        }
        U16_APPEND_UNSAFE(input, used, points[index]);
    }
    UErrorCode error = U_ZERO_ERROR;
    int32_t count = ucnv_fromUChars(converter, bytes, MAX_BYTES, input, used, &error);
    if (U_FAILURE(error) || count <= 0) {
        return 0;
    }
    return (size_t)count;
}

/** Verify standalone contextual repertoire membership without allocating intermediate arrays. */
static bool round_trip_scalar(UConverter *converter, uint32_t point) {
    char bytes[MAX_BYTES];
    size_t count = encode_points(converter, &point, 1, bytes);
    uint32_t decoded[MAX_POINTS];
    return count && decode_points(converter, bytes, count, decoded) == 1 && decoded[0] == point;
}

/** Keep alternatives contiguous, ordered by encoded length then unsigned byte value. */
static int compare_mappings(const void *left, const void *right) {
    const struct Mapping *a = left;
    const struct Mapping *b = right;
    if (a->point != b->point) {
        return (a->point > b->point) - (a->point < b->point);
    }
    return (a->key > b->key) - (a->key < b->key);
}

/** Find the first entry whose scalar is at least point. */
static size_t lower_bound(const CharactersLegacy *table, uint32_t point) {
    size_t low = 0;
    size_t high = table->count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (table->entries[middle].point < point) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

/** Grow the temporary vector with a hard limit; allocation failures abort publication. */
static bool append_mapping(CharactersLegacy *table, uint32_t point, uint64_t key) {
    if (table->count == table->capacity) {
        size_t capacity = table->capacity ? table->capacity * 2 : 256;
        if (capacity > MAX_MAPPINGS) {
            capacity = MAX_MAPPINGS;
        }
        if (capacity == table->capacity) {
            return false;
        }
        struct Mapping *entries = realloc(table->entries, capacity * sizeof(*entries));
        if (!entries) {
            return false;
        }
        table->entries = entries;
        table->capacity = capacity;
    }
    table->entries[table->count++] = (struct Mapping){ point, key };
    return true;
}

/** Add one independent encoded unit if it decodes to exactly one scalar. */
static bool add_unit(CharactersLegacy *table, UConverter *converter, uint32_t value,
                     unsigned length) {
    char bytes[4];
    for (unsigned index = 0; index < length; index++) {
        bytes[index] = (char)(value >> (8 * (length - index - 1)));
    }
    uint32_t points[MAX_POINTS];
    if (decode_points(converter, bytes, length, points) != 1) {
        return true;
    }
    return append_mapping(table, points[0], ((uint64_t)length << 32) | value);
}

/** Enumerate all single/double units, then the legal longer byte grammars only. */
static bool enumerate_units(CharactersLegacy *table, UConverter *converter,
                            enum MappingKind kind) {
    for (uint32_t value = 0; value < 256; value++) {
        if (!add_unit(table, converter, value, 1)) {
            return false;
        }
    }
    if (kind == SINGLE) {
        return true;
    }
    for (uint32_t value = 0; value < 65536; value++) {
        if (!add_unit(table, converter, value, 2)) {
            return false;
        }
    }
    if (kind == EUC_JP) {
        for (uint32_t row = 0xA1; row <= 0xFE; row++) {
            for (uint32_t cell = 0xA1; cell <= 0xFE; cell++) {
                if (!add_unit(table, converter, 0x8F0000 | row << 8 | cell, 3)) {
                    return false;
                }
            }
        }
    } else if (kind == GB18030) {
        for (uint32_t lead = 0x81; lead <= 0xFE; lead++) {
            for (uint32_t second = 0x30; second <= 0x39; second++) {
                for (uint32_t third = 0x81; third <= 0xFE; third++) {
                    for (uint32_t last = 0x30; last <= 0x39; last++) {
                        uint32_t value = lead << 24 | second << 16 | third << 8 | last;
                        if (!add_unit(table, converter, value, 4)) {
                            return false;
                        }
                    }
                }
            }
        }
    } else if (kind == EUC_TW) {
        for (uint32_t plane = 0xA1; plane <= 0xB0; plane++) {
            for (uint32_t row = 0xA1; row <= 0xFE; row++) {
                for (uint32_t cell = 0xA1; cell <= 0xFE; cell++) {
                    uint32_t value = 0x8E000000 | plane << 16 | row << 8 | cell;
                    if (!add_unit(table, converter, value, 4)) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

/** Contextual encodings expose strictly round-trippable standalone scalars, not arbitrary streams. */
static bool enumerate_contextual(CharactersLegacy *table, UConverter *converter) {
    USet *set = uset_openEmpty();
    if (!set) {
        return false;
    }
    UErrorCode error = U_ZERO_ERROR;
    ucnv_getUnicodeSet(converter, set, UCNV_ROUNDTRIP_SET, &error);
    bool success = U_SUCCESS(error);
    for (int32_t range = 0; success && range < uset_getRangeCount(set); range++) {
        UChar32 start, end;
        uset_getItem(set, range, &start, &end, NULL, 0, &error);
        if (U_FAILURE(error)) {
            success = false;
            break;
        }
        for (UChar32 value = start; value <= end; value++) {
            uint32_t point = (uint32_t)value;
            if (round_trip_scalar(converter, point) && !append_mapping(table, point, 0)) {
                success = false;
                break;
            }
        }
    }
    uset_close(set);
    return success;
}

size_t characters_legacy_count(void) {
    return COUNT(encodings);
}

const char *characters_legacy_name(size_t index) {
    return index < COUNT(encodings) ? encodings[index].name : NULL;
}

bool characters_legacy_is_contextual(size_t index) {
    return index < COUNT(encodings) && encodings[index].kind == CONTEXTUAL;
}

CharactersLegacy *characters_legacy_load(size_t index) {
    UConverter *converter = open_converter(index);
    if (!converter) {
        return NULL;
    }
    CharactersLegacy *table = calloc(1, sizeof(*table));
    if (table) {
        table->encoding = index;
        enum MappingKind kind = encodings[index].kind;
        bool success = kind == CONTEXTUAL ? enumerate_contextual(table, converter) :
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

void characters_legacy_free(CharactersLegacy *table) {
    if (table) {
        free(table->entries);
        free(table);
    }
}

uint32_t *characters_legacy_points(const CharactersLegacy *table, size_t *length) {
    if (!length) {
        return NULL;
    }
    *length = 0;
    if (!table) {
        return NULL;
    }
    uint32_t *points = malloc((table->count ? table->count : 1) * sizeof(*points));
    if (!points) {
        return NULL;
    }
    for (size_t index = 0; index < table->count; index++) {
        uint32_t point = table->entries[index].point;
        if (!*length || points[*length - 1] != point) {
            points[(*length)++] = point;
        }
    }
    return points;
}

uint32_t *characters_legacy_decode_raw(size_t encoding, const uint8_t *bytes, size_t count,
                                       size_t *length) {
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
    uint32_t decoded[MAX_POINTS];
    size_t used = decode_points(converter, (const char *)bytes, count, decoded);
    ucnv_close(converter);
    if (!used) {
        return NULL;
    }
    uint32_t *points = malloc(used * sizeof(*points));
    if (points) {
        memcpy(points, decoded, used * sizeof(*points));
        *length = used;
    }
    return points;
}

uint8_t *characters_legacy_encode_raw(size_t encoding, const uint32_t *points, size_t length,
                                     size_t *count) {
    if (!count) {
        return NULL;
    }
    *count = 0;
    UConverter *converter = open_converter(encoding);
    if (!converter) {
        return NULL;
    }
    char bytes[MAX_BYTES];
    size_t used = encode_points(converter, points, length, bytes);
    ucnv_close(converter);
    if (!used) {
        return NULL;
    }
    uint8_t *output = malloc(used);
    if (output) {
        memcpy(output, bytes, used);
        *count = used;
    }
    return output;
}

size_t characters_legacy_encoding_index(const CharactersLegacy *table) {
    return table ? table->encoding : SIZE_MAX;
}

bool characters_legacy_contains(const CharactersLegacy *table, uint32_t point) {
    if (!table || !characters_unicode_is_scalar(point)) {
        return false;
    }
    size_t index = lower_bound(table, point);
    return index < table->count && table->entries[index].point == point;
}

uint64_t *characters_legacy_keys(const CharactersLegacy *table, uint32_t point, size_t *length) {
    if (!length) {
        return NULL;
    }
    *length = 0;
    if (!table || !characters_unicode_is_scalar(point) ||
        characters_legacy_is_contextual(table->encoding)) {
        return NULL;
    }
    size_t first = lower_bound(table, point);
    size_t end = lower_bound(table, point + 1);
    if (first == end) {
        return NULL;
    }
    uint64_t *keys = malloc((end - first) * sizeof(*keys));
    if (keys) {
        for (size_t index = first; index < end; index++) {
            keys[index - first] = table->entries[index].key;
        }
        *length = end - first;
    }
    return keys;
}
