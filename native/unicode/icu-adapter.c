#include "icu-adapter.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/ustring.h>

#include "../allocation.h"
#include "icu-scalar-conversion.h"
#include "normalization.h"
#include "scalar-properties.h"
#include "unicode-algorithms.h"
#include "unicode-engine.h"

/** Maximum input length for inspector scalar operations. */
#define MAX_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
/** Maximum UTF-8 input size for text normalization, in bytes. */
#define MAX_TEXT_BYTES (1024 * 1024)

/** Count extended grapheme boundaries in a validated UTF-8 input. */
gint characters_icu_graphemes(const gchar *text)
{
    return characters_unicode_graphemes(text);
}

/** Extract the immediate ICU decomposition into caller-owned scalar storage. */
gsize characters_icu_decomposition(gunichar point, gunichar *points)
{
    if (!points || !characters_unicode_is_assigned(point) ||
        characters_scalar_decomposition_type(point) == U_DT_NONE) {
        return 0;
    }
    // Unicode 17 adds this compatibility mapping after ICU 76's Unicode 16 data.
    if (point == 0xA7F1) {
        points[0] = 0x0053;
        return 1;
    }
    UErrorCode error = U_ZERO_ERROR;
    const UNormalizer2 *normalizer = unorm2_getNFKCInstance(&error);
    if (U_FAILURE(error)) {
        return 0;
    }
    UChar units[MAX_POINTS * 2];
    int32_t length = unorm2_getRawDecomposition(normalizer, (UChar32)point, units,
                                                MAX_POINTS * 2, &error);
    if (U_FAILURE(error) || length <= 0) {
        return 0;
    }
    return characters_icu_decode_utf16(units, length, points, MAX_POINTS);
}

/** Return the Unicode 17 simple uppercase mapping for a valid scalar. */
gunichar characters_icu_uppercase(gunichar point)
{
    return characters_unicode_simple_case(point, CHARACTERS_CASE_UPPER);
}

/** Return the Unicode 17 simple lowercase mapping for a valid scalar. */
gunichar characters_icu_lowercase(gunichar point)
{
    return characters_unicode_simple_case(point, CHARACTERS_CASE_LOWER);
}

/** Return the Unicode 17 simple titlecase mapping for a valid scalar. */
gunichar characters_icu_titlecase(gunichar point)
{
    return characters_unicode_simple_case(point, CHARACTERS_CASE_TITLE);
}

/** Return the ICU bidirectional mirror, or the input when no mapping exists. */
gunichar characters_icu_mirror(gunichar point)
{
    return characters_scalar_is_valid(point) ? (gunichar)u_charMirror((UChar32)point) : point;
}

/** Read recursive canonical decomposition without compatibility mappings. */
gsize characters_icu_canonical_decomposition(gunichar point, gunichar *points)
{
    if (!points || !characters_scalar_is_valid(point)) {
        return 0;
    }
    UErrorCode error = U_ZERO_ERROR;
    const UNormalizer2 *normalizer = unorm2_getNFDInstance(&error);
    if (U_FAILURE(error)) {
        return 0;
    }
    UChar units[MAX_POINTS * 2];
    int32_t length = unorm2_getDecomposition(normalizer, (UChar32)point, units,
                                             G_N_ELEMENTS(units), &error);
    if (U_FAILURE(error) || length <= 0) {
        return 0;
    }
    return characters_icu_decode_utf16(units, length, points, MAX_POINTS);
}

/** Bridge length-aware UTF-16 normalization to owned scalar storage. */
gunichar *characters_icu_normalize_points(const gunichar *input_points, gint input_count,
                                         const gchar *form, gint *output_count)
{
    if (!output_count) {
        return NULL;
    }
    *output_count = 0;
    if (!input_points || input_count <= 0 || input_count > MAX_POINTS || !form) {
        return NULL;
    }
    UChar input[MAX_POINTS * 2];
    for (gint index = 0; index < input_count; index++) {
        if (!characters_unicode_is_assigned(input_points[index])) {
            return NULL;
        }
    }
    int32_t length = characters_icu_encode_utf16(input_points, (gsize)input_count,
                                                 input, MAX_POINTS * 2);
    if (!length) {
        return NULL;
    }
    int32_t actual = 0;
    UChar *normalized = characters_normalize_utf16(input, length, form, &actual);
    if (!normalized || actual <= 0) {
        g_free(normalized);
        return NULL;
    }
    gunichar *points = g_try_malloc_n((gsize)actual, sizeof(*points));
    if (!points) {
        g_free(normalized);
        return NULL;
    }
    gsize count = characters_icu_decode_utf16(normalized, actual, points, (gsize)actual);
    g_free(normalized);
    if (!count) {
        g_free(points);
        return NULL;
    }
    *output_count = (gint)count;
    return points;
}

/** Convert a valid bounded UTF-16 buffer to newly allocated NUL-terminated UTF-8. */
static gchar *from_utf16(const UChar *text, int32_t length)
{
    UErrorCode error = U_ZERO_ERROR;
    int32_t bytes = 0;
    u_strToUTF8(NULL, 0, &bytes, text, length, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error)) {
        return NULL;
    }
    gchar *result = g_try_malloc((gsize)bytes + 1);
    if (!result) {
        return NULL;
    }
    error = U_ZERO_ERROR;
    u_strToUTF8(result, bytes + 1, NULL, text, length, &error);
    if (U_FAILURE(error)) {
        g_free(result);
        return NULL;
    }
    return result;
}

/** Encode an exact bounded catalog sequence; NUL is not representable as C text. */
gchar *characters_unicode_encode(const gunichar *points, gsize length)
{
    if (!points || !length || length > MAX_POINTS) {
        return NULL;
    }
    guint8 text[MAX_POINTS * 4 + 1];
    int32_t offset = 0;
    for (gsize index = 0; index < length; index++) {
        if (!points[index] || !characters_scalar_is_valid(points[index])) {
            return NULL;
        }
        U8_APPEND_UNSAFE(text, offset, points[index]);
    }
    text[offset] = 0;
    return characters_try_strdup((const gchar *)text);
}

/** Normalize bounded UTF-8 using the same engine as explicit scalar normalization. */
gchar *characters_unicode_normalize(const gchar *text, const gchar *form)
{
    if (!text || strnlen(text, MAX_TEXT_BYTES + 1) > MAX_TEXT_BYTES) {
        return NULL;
    }
    UErrorCode error = U_ZERO_ERROR;
    int32_t length = 0;
    u_strFromUTF8(NULL, 0, &length, text, -1, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error)) {
        return NULL;
    }
    UChar *input = g_try_malloc_n(((gsize)length + 1), sizeof(*input));
    if (!input) {
        return NULL;
    }
    error = U_ZERO_ERROR;
    u_strFromUTF8(input, length + 1, NULL, text, -1, &error);
    if (U_FAILURE(error)) {
        g_free(input);
        return NULL;
    }
    int32_t needed = 0;
    UChar *output = characters_normalize_utf16(input, length, form, &needed);
    g_free(input);
    gchar *result = output ? from_utf16(output, needed) : NULL;
    g_free(output);
    return result;
}
