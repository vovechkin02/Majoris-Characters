#include "icu-adapter.h"

#include <stdlib.h>
#include <string.h>

#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/utext.h>
#include <unicode/utf16.h>

#include "normalization.h"
#include "scalar-properties.h"
#include "unicode-engine.h"

/** Maximum input length for inspector scalar operations. */
#define MAX_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH

/** Count extended grapheme boundaries in a validated UTF-8 input. */
int characters_icu_graphemes(const char *text) {
    if (!text || strnlen(text, 16384 * 4 + 1) > 16384 * 4) {
        return -1;
    }
    UErrorCode error = U_ZERO_ERROR;
    UText source = UTEXT_INITIALIZER;
    utext_openUTF8(&source, text, -1, &error);
    UBreakIterator *iterator = ubrk_open(UBRK_CHARACTER, "root", NULL, 0, &error);
    if (U_SUCCESS(error)) {
        ubrk_setUText(iterator, &source, &error);
    }
    int result = -1;
    if (U_SUCCESS(error)) {
        result = 0;
        ubrk_first(iterator);
        while (ubrk_next(iterator) != UBRK_DONE) {
            result++;
        }
    }
    if (iterator) {
        ubrk_close(iterator);
    }
    utext_close(&source);
    return result;
}

/** Extract the immediate ICU decomposition into caller-owned scalar storage. */
size_t characters_icu_decomposition(uint32_t point, uint32_t *points) {
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
    size_t count = 0;
    for (int32_t offset = 0; offset < length;) {
        UChar32 component;
        U16_NEXT(units, offset, length, component);
        if (count == MAX_POINTS || !characters_unicode_is_scalar((uint32_t)component)) {
            return 0;
        }
        points[count++] = (uint32_t)component;
    }
    return count;
}

/** Bridge length-aware UTF-16 normalization to owned scalar storage. */
uint32_t *characters_icu_normalize_points(const uint32_t *input_points, int input_count,
                                         const char *form, int *output_count) {
    if (!output_count) {
        return NULL;
    }
    *output_count = 0;
    if (!input_points || input_count <= 0 || input_count > MAX_POINTS || !form) {
        return NULL;
    }
    UChar input[MAX_POINTS * 2];
    int32_t length = 0;
    for (int index = 0; index < input_count; index++) {
        uint32_t point = input_points[index];
        if (!characters_unicode_is_assigned(point)) {
            return NULL;
        }
        U16_APPEND_UNSAFE(input, length, point);
    }
    int32_t actual = 0;
    UChar *normalized = characters_normalize_utf16(input, length, form, &actual);
    if (!normalized || actual <= 0) {
        free(normalized);
        return NULL;
    }
    uint32_t *points = malloc((size_t)actual * sizeof(*points));
    if (!points) {
        free(normalized);
        return NULL;
    }
    int count = 0;
    for (int32_t offset = 0; offset < actual;) {
        UChar32 point;
        U16_NEXT(normalized, offset, actual, point);
        points[count++] = (uint32_t)point;
    }
    free(normalized);
    *output_count = count;
    return points;
}

