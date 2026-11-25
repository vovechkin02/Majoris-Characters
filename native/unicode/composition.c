#include "composition.h"

#include <stdlib.h>
#include <string.h>

#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>

#include "unicode-engine.h"

/** Maximum UTF-8 bytes for the bounded scalar repertoire. */
#define MAX_BYTES (CHARACTERS_COMPOSITION_MAX_POINTS * 4)

/** Validate and optionally decode scalars; reject malformed input and oversized sequences. */
static bool validate_text(const char *text, uint32_t *points, size_t *count,
                          bool require_properties) {
    if (!text || strnlen(text, MAX_BYTES + 1) > MAX_BYTES) {
        return false;
    }
    int32_t length = (int32_t)strlen(text);
    *count = 0;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT((const uint8_t *)text, offset, length, point);
        if (point < 0 || *count == CHARACTERS_COMPOSITION_MAX_POINTS ||
            (require_properties && !characters_unicode_is_assigned((uint32_t)point))) {
            return false;
        }
        if (points) {
            points[*count] = (uint32_t)point;
        }
        (*count)++;
    }
    return true;
}

/** Count extended grapheme boundaries in a validated UTF-8 input. */
int characters_composition_graphemes(const char *text) {
    size_t count;
    if (!validate_text(text, NULL, &count, false)) {
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

/** Reuse the inspector formatter after decoding a bounded sequence. */
char *characters_composition_format(const char *text, CharactersInspectorFormat format) {
    uint32_t *points = malloc(CHARACTERS_COMPOSITION_MAX_POINTS * sizeof(*points));
    if (!points) {
        return NULL;
    }
    size_t count;
    char *result = NULL;
    if (validate_text(text, points, &count, false)) {
        result = characters_inspector_format_points(points, count, format);
    }
    free(points);
    return result;
}

/** Normalize the whole input and reject any output outside the same bound. */
char *characters_composition_normalize(const char *text, const char *form) {
    size_t count;
    if (!validate_text(text, NULL, &count, true)) {
        return NULL;
    }
    char *result = characters_unicode_normalize(text, form);
    if (!validate_text(result, NULL, &count, true)) {
        free(result);
        return NULL;
    }
    return result;
}
