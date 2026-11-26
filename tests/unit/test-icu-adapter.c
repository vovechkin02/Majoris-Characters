#include "icu-adapter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "scalar-properties.h"
#include "unicode-engine.h"

/** Verify the ICU boundary preserves the exact scalar sequence, including NUL. */
static void test_normalize_points(void) {
    uint32_t source[] = { 0x65, 0x301, 0, 0xE9 };
    int length = 0;
    uint32_t *points = characters_icu_normalize_points(source, 4, "NFC", &length);
    assert(points && length == 3);
    assert(points[0] == 0xE9 && points[1] == 0 && points[2] == 0xE9);
    free(points);
    points = characters_icu_normalize_points(source, 4, "NFD", &length);
    assert(points && length == 5);
    assert(points[0] == 0x65 && points[1] == 0x301 && points[2] == 0);
    assert(points[3] == 0x65 && points[4] == 0x301);
    free(points);
    assert(!characters_icu_normalize_points(source, 4, "invalid", &length));
    assert(length == 0);
    uint32_t invalid[] = { 0x0378 };
    assert(!characters_icu_normalize_points(invalid, 1, "NFC", &length));
    assert(length == 0);
    uint32_t new_points[] = { 0x61, 0x301, 0x1ADD, 0, 0xA7F1 };
    points = characters_icu_normalize_points(new_points, 5, "NFKD", &length);
    assert(points && length == 5);
    assert(points[0] == 0x61 && points[1] == 0x1ADD && points[2] == 0x301);
    assert(points[3] == 0 && points[4] == 0x53);
    free(points);
}

/** Check every direct decomposition without depending on display or string formatting. */
static void test_decomposition(void) {
    uint32_t points[CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH];
    assert(characters_icu_decomposition(0x1FA, points) == 2);
    assert(points[0] == 0xC5 && points[1] == 0x301);
    assert(characters_icu_decomposition(0xAC01, points) == 2);
    assert(points[0] == 0xAC00 && points[1] == 0x11A8);
    assert(characters_icu_decomposition(0xA7F1, points) == 1 && points[0] == 0x53);
    assert(!characters_icu_decomposition(0x41, points));
    assert(!characters_icu_decomposition(0xD800, points));
    assert(!characters_icu_decomposition(0x110000, points));
    size_t mappings = 0;
    for (uint32_t point = 0; point <= 0x10FFFF; point++) {
        if (!characters_scalar_decomposition_type(point)) {
            continue;
        }
        size_t count = characters_icu_decomposition(point, points);
        assert(count > 0 && count <= CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH);
        for (size_t index = 0; index < count; index++) {
            assert(characters_unicode_is_scalar(points[index]));
        }
        mappings++;
    }
    assert(mappings > 10000);
    printf("Direct decomposition mappings: %zu\n", mappings);
}

/** Check ICU grapheme segmentation independently of the Vala input validation policy. */
static void test_graphemes(void) {
    assert(characters_icu_graphemes("") == 0);
    assert(characters_icu_graphemes("A\r\n") == 2);
    assert(characters_icu_graphemes("e\xCC\x81") == 1);
    assert(characters_icu_graphemes("👩🏽‍💻🇦🇺1️⃣©️") == 4);
    assert(characters_icu_graphemes(NULL) == -1);
}

/** Run the focused native ICU boundary checks. */
int main(void) {
    test_normalize_points();
    test_decomposition();
    test_graphemes();
    puts("3 ICU adapter contract groups passed");
    return 0;
}
