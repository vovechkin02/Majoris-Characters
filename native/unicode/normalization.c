#include "normalization.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/utf16.h>

#include "scalar-properties.h"

/** Bound intermediate allocations to the maximum expanded input size. */
#define MAX_INPUT_UNITS (1024 * 1024)
#define MAX_EXPANDED_POINTS (MAX_INPUT_UNITS * 18u)

/** A growable scalar vector owned by one normalization operation. */
struct PointBuffer {
    uint32_t *items;
    size_t length;
    size_t capacity;
};

/** Append one scalar, rejecting growth beyond the bounded decomposition size. */
static bool append_point(struct PointBuffer *buffer, uint32_t point) {
    if (buffer->length == buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity * 2 : 64;
        if (capacity > MAX_EXPANDED_POINTS) {
            capacity = MAX_EXPANDED_POINTS;
        }
        if (capacity <= buffer->length) {
            return false;
        }
        uint32_t *items = realloc(buffer->items, capacity * sizeof(*items));
        if (!items) {
            return false;
        }
        buffer->items = items;
        buffer->capacity = capacity;
    }
    buffer->items[buffer->length++] = point;
    return true;
}

/** Decompose one scalar using immutable ICU 16 mappings plus the Unicode 17 delta. */
static bool append_decomposed(struct PointBuffer *buffer, UChar32 point,
                              const UNormalizer2 *normalizer, bool compatibility) {
    if (!characters_scalar_category((uint32_t)point)) {
        return append_point(buffer, (uint32_t)point);
    }
    if (compatibility && point == 0xA7F1) {
        return append_point(buffer, 0x0053);
    }
    UErrorCode error = U_ZERO_ERROR;
    UChar decomposition[64];
    int32_t length = unorm2_getDecomposition(normalizer, point, decomposition, 64, &error);
    if (U_FAILURE(error)) {
        return false;
    }
    if (length < 0) {
        return append_point(buffer, (uint32_t)point);
    }
    for (int32_t offset = 0; offset < length;) {
        UChar32 component;
        U16_NEXT(decomposition, offset, length, component);
        if (!append_point(buffer, (uint32_t)component)) {
            return false;
        }
    }
    return true;
}

/** Stable counting sort for one run of nonstarters, retaining equal-class order. */
static void sort_marks(uint32_t *items, size_t count, uint32_t *scratch) {
    size_t positions[256] = { 0 };
    for (size_t index = 0; index < count; index++) {
        positions[characters_scalar_combining_class(items[index])]++;
    }
    size_t offset = 0;
    for (size_t index = 0; index < 256; index++) {
        size_t size = positions[index];
        positions[index] = offset;
        offset += size;
    }
    for (size_t index = 0; index < count; index++) {
        unsigned combining = characters_scalar_combining_class(items[index]);
        scratch[positions[combining]++] = items[index];
    }
    memcpy(items, scratch, count * sizeof(*items));
}

/** Canonically order marks using Unicode 17 combining classes, without quadratic sorting. */
static bool reorder(struct PointBuffer *buffer) {
    uint32_t *scratch = NULL;
    size_t capacity = 0;
    for (size_t start = 0; start < buffer->length;) {
        if (!characters_scalar_combining_class(buffer->items[start])) {
            start++;
            continue;
        }
        size_t end = start;
        unsigned previous = 0;
        bool ordered = true;
        while (end < buffer->length) {
            unsigned combining = characters_scalar_combining_class(buffer->items[end]);
            if (!combining) {
                break;
            }
            if (combining < previous) {
                ordered = false;
            }
            previous = combining;
            end++;
        }
        size_t count = end - start;
        if (!ordered) {
            if (count > capacity) {
                uint32_t *replacement = realloc(scratch, count * sizeof(*scratch));
                if (!replacement) {
                    free(scratch);
                    return false;
                }
                scratch = replacement;
                capacity = count;
            }
            sort_marks(buffer->items + start, count, scratch);
        }
        start = end;
    }
    free(scratch);
    return true;
}

/** Compose unblocked canonical pairs in place; Unicode 17 adds no canonical mappings. */
static void compose(struct PointBuffer *buffer, const UNormalizer2 *normalizer) {
    size_t starter = SIZE_MAX;
    size_t written = 0;
    unsigned previous = 0;
    for (size_t index = 0; index < buffer->length; index++) {
        uint32_t point = buffer->items[index];
        unsigned combining = characters_scalar_combining_class(point);
        if (starter != SIZE_MAX && (previous == 0 || previous < combining)) {
            UChar32 composite = unorm2_composePair(normalizer,
                                                   (UChar32)buffer->items[starter],
                                                   (UChar32)point);
            if (composite >= 0 && characters_scalar_category((uint32_t)composite)) {
                buffer->items[starter] = (uint32_t)composite;
                continue;
            }
        }
        if (!combining) {
            starter = written;
        }
        buffer->items[written++] = point;
        previous = combining;
    }
    buffer->length = written;
}

/** Retain the ICU fast path for input entirely within its assigned repertoire. */
static UChar *normalize_icu(const UChar *input, int32_t length,
                            const UNormalizer2 *normalizer, int32_t *output_length) {
    UErrorCode error = U_ZERO_ERROR;
    int32_t needed = unorm2_normalize(normalizer, input, length, NULL, 0, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error)) {
        return NULL;
    }
    UChar *output = malloc(((size_t)needed + 1) * sizeof(*output));
    if (!output) {
        return NULL;
    }
    error = U_ZERO_ERROR;
    *output_length = unorm2_normalize(normalizer, input, length, output, needed + 1, &error);
    if (U_FAILURE(error)) {
        free(output);
        return NULL;
    }
    return output;
}

/** Share normalization across the UTF-8 engine and length-aware inspector. */
UChar *characters_normalize_utf16(const UChar *input, int32_t length,
                                  const char *form, int32_t *output_length) {
    if (!input || !form || !output_length || length < 0 || length > MAX_INPUT_UNITS) {
        return NULL;
    }
    *output_length = 0;
    bool compatibility = strcmp(form, "NFKC") == 0 || strcmp(form, "NFKD") == 0;
    bool composed = strcmp(form, "NFC") == 0 || strcmp(form, "NFKC") == 0;
    if (!compatibility && !composed && strcmp(form, "NFD") != 0) {
        return NULL;
    }
    UErrorCode error = U_ZERO_ERROR;
    const UNormalizer2 *decomposer = compatibility ? unorm2_getNFKDInstance(&error) :
                                      unorm2_getNFDInstance(&error);
    const UNormalizer2 *composer = compatibility ? unorm2_getNFKCInstance(&error) :
                                    unorm2_getNFCInstance(&error);
    if (U_FAILURE(error)) {
        return NULL;
    }
    bool needs_snapshot = false;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U16_NEXT(input, offset, length, point);
        if (point < 0 || (point >= 0xD800 && point <= 0xDFFF)) {
            return NULL;
        }
        if ((u_charType(point) == U_UNASSIGNED) !=
            (characters_scalar_category((uint32_t)point) == U_UNASSIGNED)) {
            needs_snapshot = true;
        }
    }
    if (!needs_snapshot) {
        return normalize_icu(input, length, composed ? composer : decomposer, output_length);
    }
    struct PointBuffer buffer = { 0 };
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U16_NEXT(input, offset, length, point);
        if (!append_decomposed(&buffer, point, decomposer, compatibility)) {
            free(buffer.items);
            return NULL;
        }
    }
    if (!reorder(&buffer)) {
        free(buffer.items);
        return NULL;
    }
    if (composed) {
        compose(&buffer, composer);
    }
    UChar *output = malloc((buffer.length * 2 + 1) * sizeof(*output));
    if (output) {
        for (size_t index = 0; index < buffer.length; index++) {
            U16_APPEND_UNSAFE(output, *output_length, buffer.items[index]);
        }
        output[*output_length] = 0;
    }
    free(buffer.items);
    return output;
}
