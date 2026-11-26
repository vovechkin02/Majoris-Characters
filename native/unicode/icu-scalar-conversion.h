#ifndef CHARACTERS_ICU_SCALAR_CONVERSION_H
#define CHARACTERS_ICU_SCALAR_CONVERSION_H

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <glib.h>

#include <unicode/utf16.h>
#include <unicode/utf8.h>

#include "../scalar-validity.h"

/**
 * Decode a NUL-terminated UTF-8 sequence into caller-owned scalar storage.
 * Reject malformed or oversized input without allocating. Return zero for
 * empty or invalid input; partial output must not be used on failure.
 */
static inline gsize characters_icu_decode_utf8(const gchar *text, gunichar *points,
                                                gsize capacity)
{
    if (!text || !points || !capacity || capacity > INT32_MAX / 4) {
        return 0;
    }
    gsize bytes = strnlen(text, capacity * 4 + 1);
    if (bytes > capacity * 4) {
        return 0;
    }
    int32_t length = (int32_t)bytes;
    gsize count = 0;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT((const guint8 *)text, offset, length, point);
        if (point < 0 || count == capacity) {
            return 0;
        }
        points[count++] = (guint32)point;
    }
    return count;
}

/**
 * Decode explicit-length UTF-16 into caller-owned scalars, including NUL.
 * Reject isolated surrogates and insufficient capacity. Return zero for an
 * empty or invalid input; partial output must not be used on failure.
 */
static inline gsize characters_icu_decode_utf16(const UChar *units, int32_t length,
                                                 gunichar *points, gsize capacity)
{
    if (!units || length <= 0 || !points || !capacity) {
        return 0;
    }
    gsize count = 0;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U16_NEXT(units, offset, length, point);
        if (count == capacity || !characters_scalar_is_valid((guint32)point)) {
            return 0;
        }
        points[count++] = (guint32)point;
    }
    return count;
}

/**
 * Encode exact scalars to caller-owned UTF-16 units, including embedded NUL.
 * Capacity and result count are in code units, not bytes; no terminator is
 * appended. Return zero for empty, invalid or oversized input.
 */
static inline int32_t characters_icu_encode_utf16(const gunichar *points, gsize count,
                                                  UChar *units, int32_t capacity)
{
    if (!points || !count || !units || capacity <= 0 || count > (gsize)capacity) {
        return 0;
    }
    int32_t length = 0;
    for (gsize index = 0; index < count; index++) {
        gunichar point = points[index];
        if (!characters_scalar_is_valid(point) || U16_LENGTH(point) > capacity - length) {
            return 0;
        }
        U16_APPEND_UNSAFE(units, length, point);
    }
    return length;
}

#endif
