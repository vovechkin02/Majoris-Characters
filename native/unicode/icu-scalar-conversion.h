#ifndef CHARACTERS_ICU_SCALAR_CONVERSION_H
#define CHARACTERS_ICU_SCALAR_CONVERSION_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <unicode/utf16.h>
#include <unicode/utf8.h>

#include "../scalar-validity.h"

/**
 * Decode a NUL-terminated UTF-8 sequence into caller-owned scalar storage.
 * Reject malformed or oversized input without allocating. Return zero for
 * empty or invalid input; partial output must not be used on failure.
 */
static inline size_t characters_icu_decode_utf8(const char *text, uint32_t *points,
                                                size_t capacity) {
    if (!text || !points || !capacity || capacity > INT32_MAX / 4) {
        return 0;
    }
    size_t bytes = strnlen(text, capacity * 4 + 1);
    if (bytes > capacity * 4) {
        return 0;
    }
    int32_t length = (int32_t)bytes;
    size_t count = 0;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT((const uint8_t *)text, offset, length, point);
        if (point < 0 || count == capacity) {
            return 0;
        }
        points[count++] = (uint32_t)point;
    }
    return count;
}

/**
 * Decode explicit-length UTF-16 into caller-owned scalars, including NUL.
 * Reject isolated surrogates and insufficient capacity. Return zero for an
 * empty or invalid input; partial output must not be used on failure.
 */
static inline size_t characters_icu_decode_utf16(const UChar *units, int32_t length,
                                                 uint32_t *points, size_t capacity) {
    if (!units || length <= 0 || !points || !capacity) {
        return 0;
    }
    size_t count = 0;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U16_NEXT(units, offset, length, point);
        if (count == capacity || !characters_scalar_is_valid((uint32_t)point)) {
            return 0;
        }
        points[count++] = (uint32_t)point;
    }
    return count;
}

/**
 * Encode exact scalars to caller-owned UTF-16 units, including embedded NUL.
 * Capacity and result count are in code units, not bytes; no terminator is
 * appended. Return zero for empty, invalid or oversized input.
 */
static inline int32_t characters_icu_encode_utf16(const uint32_t *points, size_t count,
                                                  UChar *units, int32_t capacity) {
    if (!points || !count || !units || capacity <= 0 || count > (size_t)capacity) {
        return 0;
    }
    int32_t length = 0;
    for (size_t index = 0; index < count; index++) {
        uint32_t point = points[index];
        if (!characters_scalar_is_valid(point) || U16_LENGTH(point) > capacity - length) {
            return 0;
        }
        U16_APPEND_UNSAFE(units, length, point);
    }
    return length;
}

#endif
