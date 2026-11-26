#include "normalization.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/utf16.h>

#include "../scalar-validity.h"
#include "scalar-properties.h"

/** Bound intermediate allocations to the maximum expanded input size. */
#define MAX_INPUT_UNITS (1024 * 1024)
#define MAX_EXPANDED_POINTS (MAX_INPUT_UNITS * 18u)

/** A growable scalar vector owned by one normalization operation. */
struct PointBuffer {
    gunichar *items;
    gsize length;
    gsize capacity;
};

/** Append one scalar, rejecting growth beyond the bounded decomposition size. */
static gboolean append_point(struct PointBuffer *buffer, gunichar point)
{
    if (buffer->length == buffer->capacity) {
        gsize capacity = buffer->capacity ? buffer->capacity * 2 : 64;
        if (capacity > MAX_EXPANDED_POINTS) {
            capacity = MAX_EXPANDED_POINTS;
        }
        if (capacity <= buffer->length) {
            return FALSE;
        }
        gunichar *items = g_try_realloc_n(buffer->items, capacity, sizeof(*items));
        if (!items) {
            return FALSE;
        }
        buffer->items = items;
        buffer->capacity = capacity;
    }
    buffer->items[buffer->length++] = point;
    return TRUE;
}

/** Decompose one scalar using immutable ICU 16 mappings plus the Unicode 17 delta. */
static gboolean append_decomposed(struct PointBuffer *buffer, UChar32 point,
                              const UNormalizer2 *normalizer, gboolean compatibility)
{
    if (!characters_scalar_category((guint32)point)) {
        return append_point(buffer, (guint32)point);
    }
    if (compatibility && point == 0xA7F1) {
        return append_point(buffer, 0x0053);
    }
    UErrorCode error = U_ZERO_ERROR;
    UChar decomposition[64];
    int32_t length = unorm2_getDecomposition(normalizer, point, decomposition, 64, &error);
    if (U_FAILURE(error)) {
        return FALSE;
    }
    if (length < 0) {
        return append_point(buffer, (guint32)point);
    }
    for (int32_t offset = 0; offset < length;) {
        UChar32 component;
        U16_NEXT(decomposition, offset, length, component);
        if (!append_point(buffer, (guint32)component)) {
            return FALSE;
        }
    }
    return TRUE;
}

/** Stable counting sort for one run of nonstarters, retaining equal-class order. */
static void sort_marks(gunichar *items, gsize count, gunichar *scratch)
{
    gsize positions[256] = { 0 };
    for (gsize index = 0; index < count; index++) {
        positions[characters_scalar_combining_class(items[index])]++;
    }
    gsize offset = 0;
    for (gsize index = 0; index < 256; index++) {
        gsize size = positions[index];
        positions[index] = offset;
        offset += size;
    }
    for (gsize index = 0; index < count; index++) {
        guint combining = characters_scalar_combining_class(items[index]);
        scratch[positions[combining]++] = items[index];
    }
    memcpy(items, scratch, count * sizeof(*items));
}

/** Canonically order marks using Unicode 17 combining classes, without quadratic sorting. */
static gboolean reorder(struct PointBuffer *buffer)
{
    gunichar *scratch = NULL;
    gsize capacity = 0;
    for (gsize start = 0; start < buffer->length;) {
        if (!characters_scalar_combining_class(buffer->items[start])) {
            start++;
            continue;
        }
        gsize end = start;
        guint previous = 0;
        gboolean ordered = TRUE;
        while (end < buffer->length) {
            guint combining = characters_scalar_combining_class(buffer->items[end]);
            if (!combining) {
                break;
            }
            if (combining < previous) {
                ordered = FALSE;
            }
            previous = combining;
            end++;
        }
        gsize count = end - start;
        if (!ordered) {
            if (count > capacity) {
                gunichar *replacement = g_try_realloc_n(scratch, count, sizeof(*scratch));
                if (!replacement) {
                    g_free(scratch);
                    return FALSE;
                }
                scratch = replacement;
                capacity = count;
            }
            sort_marks(buffer->items + start, count, scratch);
        }
        start = end;
    }
    g_free(scratch);
    return TRUE;
}

/** Compose unblocked canonical pairs in place; Unicode 17 adds no canonical mappings. */
static void compose(struct PointBuffer *buffer, const UNormalizer2 *normalizer)
{
    gsize starter = G_MAXSIZE;
    gsize written = 0;
    guint previous = 0;
    for (gsize index = 0; index < buffer->length; index++) {
        gunichar point = buffer->items[index];
        guint combining = characters_scalar_combining_class(point);
        if (starter != G_MAXSIZE && (previous == 0 || previous < combining)) {
            UChar32 composite = unorm2_composePair(normalizer,
                                                   (UChar32)buffer->items[starter],
                                                   (UChar32)point);
            if (composite >= 0 && characters_scalar_category((guint32)composite)) {
                buffer->items[starter] = (guint32)composite;
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
                            const UNormalizer2 *normalizer, int32_t *output_length)
{
    UErrorCode error = U_ZERO_ERROR;
    int32_t needed = unorm2_normalize(normalizer, input, length, NULL, 0, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error)) {
        return NULL;
    }
    UChar *output = g_try_malloc_n(((gsize)needed + 1), sizeof(*output));
    if (!output) {
        return NULL;
    }
    error = U_ZERO_ERROR;
    *output_length = unorm2_normalize(normalizer, input, length, output, needed + 1, &error);
    if (U_FAILURE(error)) {
        g_free(output);
        return NULL;
    }
    return output;
}

/** Share normalization across the UTF-8 engine and length-aware inspector. */
UChar *characters_normalize_utf16(const UChar *input, int32_t length,
                                  const gchar *form, int32_t *output_length)
{
    if (!input || !form || !output_length || length < 0 || length > MAX_INPUT_UNITS) {
        return NULL;
    }
    *output_length = 0;
    gboolean compatibility = strcmp(form, "NFKC") == 0 || strcmp(form, "NFKD") == 0;
    gboolean composed = strcmp(form, "NFC") == 0 || strcmp(form, "NFKC") == 0;
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
    gboolean needs_snapshot = FALSE;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U16_NEXT(input, offset, length, point);
        if (!characters_scalar_is_valid((guint32)point)) {
            return NULL;
        }
        if ((u_charType(point) == U_UNASSIGNED) !=
            (characters_scalar_category((guint32)point) == U_UNASSIGNED)) {
            needs_snapshot = TRUE;
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
            g_free(buffer.items);
            return NULL;
        }
    }
    if (!reorder(&buffer)) {
        g_free(buffer.items);
        return NULL;
    }
    if (composed) {
        compose(&buffer, composer);
    }
    UChar *output = g_try_malloc_n((buffer.length * 2 + 1), sizeof(*output));
    if (output) {
        for (gsize index = 0; index < buffer.length; index++) {
            U16_APPEND_UNSAFE(output, *output_length, buffer.items[index]);
        }
        output[*output_length] = 0;
    }
    g_free(buffer.items);
    return output;
}
