#include "legacy-encoding.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/ucnv.h>
#include <unicode/ucnv_err.h>
#include <unicode/utf16.h>

#include "unicode-engine.h"

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define SINGLE_LIMIT 256u
#define DOUBLE_LIMIT 65536u

/** Describes an explicitly supported stateless, one- or two-byte encoding. */
struct Encoding {
    /** Display label, also the stable encoding identity. */
    const char *name;
    /** ICU converter alias or canonical name, with deliberate Windows mappings. */
    const char *converter;
    /** Maximum encoded bytes for one scalar, never more than two. */
    unsigned width;
};

/** Supported character sets; stateful and longer encodings are intentionally excluded. */
static const struct Encoding encodings[] = {
    { "US-ASCII", "US-ASCII", 1 },
    { "ISO-8859-1", "ISO-8859-1", 1 },
    { "ISO-8859-2", "ISO-8859-2", 1 },
    { "ISO-8859-3", "ISO-8859-3", 1 },
    { "ISO-8859-4", "ISO-8859-4", 1 },
    { "ISO-8859-5", "ISO-8859-5", 1 },
    { "ISO-8859-6", "ISO-8859-6", 1 },
    { "ISO-8859-7", "ISO-8859-7", 1 },
    { "ISO-8859-8", "ISO-8859-8", 1 },
    { "ISO-8859-9", "ISO-8859-9", 1 },
    { "ISO-8859-10", "ISO-8859-10", 1 },
    { "ISO-8859-13", "ISO-8859-13", 1 },
    { "ISO-8859-14", "ISO-8859-14", 1 },
    { "ISO-8859-15", "ISO-8859-15", 1 },
    { "Windows-1250", "windows-1250", 1 },
    { "Windows-1251", "windows-1251", 1 },
    { "Windows-1252", "windows-1252", 1 },
    { "Windows-1253", "windows-1253", 1 },
    { "Windows-1254", "windows-1254", 1 },
    { "Windows-1255", "windows-1255", 1 },
    { "Windows-1256", "windows-1256", 1 },
    { "Windows-1257", "windows-1257", 1 },
    { "Windows-1258", "windows-1258", 1 },
    { "Windows-874", "windows-874-2000", 1 },
    { "IBM 437", "ibm-437", 1 },
    { "IBM 850", "ibm-850", 1 },
    { "IBM 852", "ibm-852", 1 },
    { "IBM 866", "ibm-866", 1 },
    { "KOI8-R", "KOI8-R", 1 },
    { "KOI8-U", "KOI8-U", 1 },
    { "Macintosh Roman", "macintosh", 1 },
    { "Shift JIS (Windows-932)", "windows-31j", 2 },
    { "GBK (Windows-936)", "windows-936-2000", 2 },
    { "Korean (Windows-949)", "windows-949-2000", 2 },
    { "Big5 (Windows-950)", "windows-950-2000", 2 },
};

/** Associates one exact single- or double-byte input with its decoded scalar. */
struct Mapping {
    /** Decoded Unicode scalar, including NUL. */
    uint32_t point;
    /** 0–255 for a single byte; 256 plus the big-endian value for two bytes. */
    uint32_t key;
};

/** Owns a bounded mapping vector sorted by scalar and then encoded byte identity. */
struct CharactersLegacy {
    /** Initialized mapping entries. */
    struct Mapping *entries;
    /** Number of entries, including alternate encodings of the same scalar. */
    size_t count;
};

/** Convert exactly one input sequence, rejecting invalid, truncated and multi-scalar output. */
static uint32_t decode(UConverter *converter, uint32_t key) {
    char bytes[2];
    int32_t length;
    if (key < SINGLE_LIMIT) {
        bytes[0] = (char)key;
        length = 1;
    } else {
        uint32_t value = key - SINGLE_LIMIT;
        bytes[0] = (char)(value >> 8);
        bytes[1] = (char)value;
        length = 2;
    }
    UChar output[4];
    UErrorCode error = U_ZERO_ERROR;
    int32_t count = ucnv_toUChars(converter, output, COUNT(output), bytes, length, &error);
    if (U_FAILURE(error) || count < 1 || count > 2) {
        return UINT32_MAX;
    }
    int32_t offset = 0;
    UChar32 point;
    U16_NEXT(output, offset, count, point);
    return offset == count && point >= 0 && characters_unicode_is_scalar((uint32_t)point) ?
        (uint32_t)point : UINT32_MAX;
}

/** Keep alternate byte encodings contiguous and ordered by length then byte value. */
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

/** Convert only ASCII hexadecimal digits; do not accept signs or locale-dependent digits. */
static int hex_digit(char digit) {
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    if (digit >= 'A' && digit <= 'F') {
        return digit - 'A' + 10;
    }
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    return -1;
}

size_t characters_legacy_count(void) {
    return COUNT(encodings);
}

const char *characters_legacy_name(size_t index) {
    return index < COUNT(encodings) ? encodings[index].name : NULL;
}

CharactersLegacy *characters_legacy_load(size_t index) {
    if (index >= COUNT(encodings)) {
        return NULL;
    }
    const struct Encoding *encoding = &encodings[index];
    UErrorCode error = U_ZERO_ERROR;
    UConverter *converter = ucnv_open(encoding->converter, &error);
    if (U_FAILURE(error) || !converter) {
        return NULL;
    }
    ucnv_setFallback(converter, false);
    ucnv_setToUCallBack(converter, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &error);
    size_t limit = SINGLE_LIMIT + (encoding->width == 2 ? DOUBLE_LIMIT : 0);
    CharactersLegacy *table = U_SUCCESS(error) ? calloc(1, sizeof(*table)) : NULL;
    if (table) {
        table->entries = malloc(limit * sizeof(*table->entries));
        if (!table->entries) {
            characters_legacy_free(table);
            table = NULL;
        }
    }
    if (table) {
        for (uint32_t key = 0; key < limit; key++) {
            uint32_t point = decode(converter, key);
            if (point != UINT32_MAX) {
                table->entries[table->count++] = (struct Mapping){ point, key };
            }
        }
        qsort(table->entries, table->count, sizeof(*table->entries), compare_mappings);
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

uint32_t characters_legacy_decode(const CharactersLegacy *table, const char *hex) {
    if (!table || !hex) {
        return UINT32_MAX;
    }
    size_t length = strnlen(hex, 6);
    if (length != 2 && length != 4 && !(length == 5 && hex[2] == ' ')) {
        return UINT32_MAX;
    }
    uint32_t key = 0;
    for (size_t index = 0; index < length; index++) {
        if (length == 5 && index == 2) {
            continue;
        }
        int digit = hex_digit(hex[index]);
        if (digit < 0) {
            return UINT32_MAX;
        }
        key = key * 16u + (uint32_t)digit;
    }
    if (length != 2) {
        key += SINGLE_LIMIT;
    }
    for (size_t index = 0; index < table->count; index++) {
        if (table->entries[index].key == key) {
            return table->entries[index].point;
        }
    }
    return UINT32_MAX;
}

char *characters_legacy_bytes(const CharactersLegacy *table, uint32_t point) {
    if (!table || !characters_unicode_is_scalar(point)) {
        return NULL;
    }
    size_t first = lower_bound(table, point);
    size_t end = lower_bound(table, point + 1);
    if (first == end) {
        return NULL;
    }
    size_t capacity = (end - first) * 7 + 1;
    char *text = malloc(capacity);
    if (!text) {
        return NULL;
    }
    size_t offset = 0;
    for (size_t index = first; index < end; index++) {
        uint32_t key = table->entries[index].key;
        const char *separator = index == first ? "" : ", ";
        if (key < SINGLE_LIMIT) {
            offset += (size_t)snprintf(text + offset, capacity - offset, "%s%02X",
                                      separator, (unsigned)key);
        } else {
            uint32_t value = key - SINGLE_LIMIT;
            offset += (size_t)snprintf(text + offset, capacity - offset, "%s%02X %02X",
                                      separator, (unsigned)(value >> 8), (unsigned)(value & 255));
        }
    }
    return text;
}
