#include "character-inspector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/unorm2.h>
#include <unicode/utf8.h>
#include <unicode/utf16.h>

#include "character-search.h"
#include "emoji-catalog.h"
#include "unicode-engine.h"

/** Maximum number of scalars accepted by the bounded snapshot. */
#define MAX_POINTS CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH
/** Maximum ASCII bytes emitted per scalar, including a separator. */
#define FORMAT_BYTES_PER_POINT 16

/** Immutable scalars, including U+0000, without dependence on UTF-8 C-string storage. */
struct CharactersInspector {
    /** Exact sequence in original order. */
    uint32_t points[MAX_POINTS];
    /** Number of initialized scalars. */
    size_t count;
};

/** Allocate a snapshot only for a nonempty bounded scalar sequence. */
static CharactersInspector *create_snapshot(const uint32_t *points, size_t count) {
    if (!points || !count || count > MAX_POINTS) {
        return NULL;
    }
    CharactersInspector *snapshot = calloc(1, sizeof(*snapshot));
    if (snapshot) {
        memcpy(snapshot->points, points, count * sizeof(*points));
        snapshot->count = count;
    }
    return snapshot;
}

/** Format scalars without converting the underlying text or dropping NUL code points. */
char *characters_inspector_format_points(const uint32_t *points, size_t count,
                                         CharactersInspectorFormat format) {
    if (!points || !count || count > (SIZE_MAX - 1) / FORMAT_BYTES_PER_POINT) {
        return NULL;
    }
    size_t capacity = count * FORMAT_BYTES_PER_POINT + 1;
    char *output = malloc(capacity);
    if (!output) {
        return NULL;
    }
    size_t offset = 0;
    for (size_t index = 0; index < count; index++) {
        unsigned point = points[index];
        if (!characters_unicode_is_scalar(point)) {
            free(output);
            return NULL;
        }
        const char *separator = index ? " " : "";
        char *cursor = output + offset;
        size_t remaining = capacity - offset;
        int written;
        switch (format) {
            case CHARACTERS_INSPECTOR_CODE:
                written = snprintf(cursor, remaining, "%sU+%04X", separator, point);
                break;
            case CHARACTERS_INSPECTOR_DECIMAL:
                written = snprintf(cursor, remaining, "%s%u", separator, point);
                break;
            case CHARACTERS_INSPECTOR_UTF8: {
                uint8_t bytes[4];
                int32_t length = 0;
                U8_APPEND_UNSAFE(bytes, length, point);
                written = 0;
                for (int32_t byte = 0; byte < length; byte++) {
                    written += snprintf(cursor + written, remaining - (size_t)written,
                                        "%s%02X", index || byte ? " " : "", bytes[byte]);
                }
                break;
            }
            case CHARACTERS_INSPECTOR_UTF16:
                if (point <= 0xFFFF) {
                    written = snprintf(cursor, remaining, "%s%04X", separator, point);
                } else {
                    written = snprintf(cursor, remaining, "%s%04X %04X", separator,
                                       (unsigned)U16_LEAD(point), (unsigned)U16_TRAIL(point));
                }
                break;
            case CHARACTERS_INSPECTOR_UTF32:
                written = snprintf(cursor, remaining, "%s%08X", separator, point);
                break;
            case CHARACTERS_INSPECTOR_HTML_DECIMAL:
                written = snprintf(cursor, remaining, "&#%u;", point);
                break;
            case CHARACTERS_INSPECTOR_HTML_HEX:
                written = snprintf(cursor, remaining, "&#x%X;", point);
                break;
            case CHARACTERS_INSPECTOR_ESCAPE:
                written = snprintf(cursor, remaining, point <= 0xFFFF ? "\\u%04X" : "\\U%08X",
                                   point);
                break;
            case CHARACTERS_INSPECTOR_CSS:
                written = snprintf(cursor, remaining, "\\%X ", point);
                break;
            default:
                free(output);
                return NULL;
        }
        if (written < 0 || (size_t)written >= remaining) {
            free(output);
            return NULL;
        }
        offset += (size_t)written;
    }
    return output;
}

/** Return the ICU normalization instance for one of the four supported forms. */
static const UNormalizer2 *get_normalizer(const char *form, UErrorCode *error) {
    if (!form) {
        return NULL;
    }
    if (strcmp(form, "NFC") == 0) {
        return unorm2_getNFCInstance(error);
    }
    if (strcmp(form, "NFD") == 0) {
        return unorm2_getNFDInstance(error);
    }
    if (strcmp(form, "NFKC") == 0) {
        return unorm2_getNFKCInstance(error);
    }
    if (strcmp(form, "NFKD") == 0) {
        return unorm2_getNFKDInstance(error);
    }
    return NULL;
}

CharactersInspector *characters_inspector_from_id(uint32_t id) {
    if (id <= 0x10FFFFu && !characters_unicode_is_assigned(id)) {
        return NULL;
    }
    char *code = characters_search_code(id);
    CharactersInspector *snapshot = characters_inspector_from_code(code);
    free(code);
    return snapshot;
}

CharactersInspector *characters_inspector_from_text(const char *text) {
    if (!text || strnlen(text, MAX_POINTS * 4 + 1) > MAX_POINTS * 4) {
        return NULL;
    }
    int32_t length = (int32_t)strlen(text);
    uint32_t points[MAX_POINTS];
    size_t count = 0;
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT((const uint8_t *)text, offset, length, point);
        if (point < 0 || count == MAX_POINTS) {
            return NULL;
        }
        points[count++] = (uint32_t)point;
    }
    return create_snapshot(points, count);
}

CharactersInspector *characters_inspector_from_code(const char *code) {
    uint32_t points[MAX_POINTS];
    size_t count = characters_search_decode_code(code, points);
    return create_snapshot(points, count);
}

void characters_inspector_free(CharactersInspector *snapshot) {
    free(snapshot);
}

size_t characters_inspector_count(const CharactersInspector *snapshot) {
    return snapshot ? snapshot->count : 0;
}

uint32_t characters_inspector_point(const CharactersInspector *snapshot, size_t index) {
    return snapshot && index < snapshot->count ? snapshot->points[index] : UINT32_MAX;
}

char *characters_inspector_name(const CharactersInspector *snapshot) {
    return snapshot ? characters_unicode_sequence_name(snapshot->points, snapshot->count) : NULL;
}

char *characters_inspector_text(const CharactersInspector *snapshot) {
    return snapshot ? characters_unicode_encode(snapshot->points, snapshot->count) : NULL;
}

char *characters_inspector_display(const CharactersInspector *snapshot) {
    if (!snapshot) {
        return NULL;
    }
    uint32_t first = snapshot->points[0];
    if (snapshot->count == 1 && characters_unicode_is_assigned(first)) {
        if (u_hasBinaryProperty((UChar32)first, UCHAR_DEFAULT_IGNORABLE_CODE_POINT)) {
            return strdup("·");
        }
        return characters_search_display(first);
    }
    // Render known emoji sequences intact; escape invisible controls in arbitrary text previews.
    char *emoji_text = characters_inspector_text(snapshot);
    if (characters_emoji_is_candidate(emoji_text)) {
        return emoji_text;
    }
    free(emoji_text);
    uint8_t buffer[(MAX_POINTS + 1) * 4 + 1];
    int32_t offset = 0;
    int8_t category = u_charType((UChar32)first);
    if (category == U_NON_SPACING_MARK || category == U_COMBINING_SPACING_MARK ||
        category == U_ENCLOSING_MARK) {
        U8_APPEND_UNSAFE(buffer, offset, 0x25CC);
    }
    for (size_t index = 0; index < snapshot->count; index++) {
        uint32_t point = snapshot->points[index];
        category = u_charType((UChar32)point);
        if (category == U_CONTROL_CHAR || category == U_FORMAT_CHAR ||
            category == U_LINE_SEPARATOR || category == U_PARAGRAPH_SEPARATOR ||
            category == U_SPACE_SEPARATOR || u_hasBinaryProperty((UChar32)point,
                                                                 UCHAR_DEFAULT_IGNORABLE_CODE_POINT)) {
            point = 0x00B7;
        }
        U8_APPEND_UNSAFE(buffer, offset, point);
    }
    buffer[offset] = 0;
    return strdup((const char *)buffer);
}

char *characters_inspector_format(const CharactersInspector *snapshot,
                                  CharactersInspectorFormat format) {
    return snapshot ? characters_inspector_format_points(snapshot->points, snapshot->count, format) : NULL;
}

char *characters_inspector_normalize(const CharactersInspector *snapshot, const char *form) {
    if (!snapshot) {
        return NULL;
    }
    UChar input[MAX_POINTS * 2];
    int32_t length = 0;
    for (size_t index = 0; index < snapshot->count; index++) {
        uint32_t point = snapshot->points[index];
        // Do not claim normalization for catalog characters beyond the loaded ICU repertoire.
        if (u_charType((UChar32)point) == U_UNASSIGNED) {
            return NULL;
        }
        U16_APPEND_UNSAFE(input, length, point);
    }
    UErrorCode error = U_ZERO_ERROR;
    const UNormalizer2 *normalizer = get_normalizer(form, &error);
    if (!normalizer || U_FAILURE(error)) {
        return NULL;
    }
    int32_t needed = unorm2_normalize(normalizer, input, length, NULL, 0, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR || needed <= 0) {
        return NULL;
    }
    UChar *normalized = malloc((size_t)needed * sizeof(*normalized));
    uint32_t *points = malloc((size_t)needed * sizeof(*points));
    if (!normalized || !points) {
        free(normalized);
        free(points);
        return NULL;
    }
    error = U_ZERO_ERROR;
    int32_t actual = unorm2_normalize(normalizer, input, length, normalized, needed, &error);
    size_t count = 0;
    if (U_SUCCESS(error) && actual == needed) {
        for (int32_t offset = 0; offset < actual;) {
            UChar32 point;
            U16_NEXT(normalized, offset, actual, point);
            points[count++] = (uint32_t)point;
        }
    }
    char *result = characters_inspector_format_points(points, count, CHARACTERS_INSPECTOR_CODE);
    free(normalized);
    free(points);
    return result;
}

char **characters_inspector_related(const CharactersInspector *snapshot) {
    return snapshot ? characters_unicode_related(snapshot->points, snapshot->count) : NULL;
}
