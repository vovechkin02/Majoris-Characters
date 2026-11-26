#include "icu-adapter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "icu-scalar-conversion.h"
#include "scalar-properties.h"
#include "unicode-engine.h"

/** Verify the ICU boundary preserves the exact scalar sequence, including NUL. */
static void test_normalize_points(void) {
    guint32 source[] = { 0x65, 0x301, 0, 0xE9 };
    gint length = 0;
    gunichar *points = characters_icu_normalize_points(source, 4, "NFC", &length);
    assert(points && length == 3);
    assert(points[0] == 0xE9 && points[1] == 0 && points[2] == 0xE9);
    g_free(points);
    points = characters_icu_normalize_points(source, 4, "NFD", &length);
    assert(points && length == 5);
    assert(points[0] == 0x65 && points[1] == 0x301 && points[2] == 0);
    assert(points[3] == 0x65 && points[4] == 0x301);
    g_free(points);
    assert(!characters_icu_normalize_points(source, 4, "invalid", &length));
    assert(length == 0);
    guint32 invalid[] = { 0x0378 };
    assert(!characters_icu_normalize_points(invalid, 1, "NFC", &length));
    assert(length == 0);
    guint32 new_points[] = { 0x61, 0x301, 0x1ADD, 0, 0xA7F1 };
    points = characters_icu_normalize_points(new_points, 5, "NFKD", &length);
    assert(points && length == 5);
    assert(points[0] == 0x61 && points[1] == 0x1ADD && points[2] == 0x301);
    assert(points[3] == 0 && points[4] == 0x53);
    g_free(points);
}

/** Check every direct decomposition without depending on display or string formatting. */
static void test_decomposition(void) {
    gunichar points[CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH];
    assert(characters_icu_decomposition(0x1FA, points) == 2);
    assert(points[0] == 0xC5 && points[1] == 0x301);
    assert(characters_icu_decomposition(0xAC01, points) == 2);
    assert(points[0] == 0xAC00 && points[1] == 0x11A8);
    assert(characters_icu_decomposition(0xA7F1, points) == 1 && points[0] == 0x53);
    assert(!characters_icu_decomposition(0x41, points));
    assert(!characters_icu_decomposition(0xD800, points));
    assert(!characters_icu_decomposition(0x110000, points));
    gsize mappings = 0;
    for (gunichar point = 0; point <= 0x10FFFF; point++) {
        if (!characters_scalar_decomposition_type(point)) {
            continue;
        }
        gsize count = characters_icu_decomposition(point, points);
        assert(count > 0 && count <= CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH);
        for (gsize index = 0; index < count; index++) {
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

/** Exercise the shared bounded decoder and encoder without a Vala runtime. */
static void test_scalar_conversion(void) {
    gunichar points[32];
    UChar units[64];
    const guint32 source[] = { 0, 0x41, 0x1F600, 0x10FFFF, 0x0378 };
    assert(characters_icu_encode_utf16(source, 5, units, 64) == 7);
    assert(characters_icu_decode_utf16(units, 7, points, 32) == 5);
    assert(!memcmp(points, source, sizeof(source)));
    assert(!characters_icu_decode_utf16(units, 7, points, 4));
    assert(!characters_icu_encode_utf16(source, 5, units, 6));
    const guint32 invalid[] = { 0xD800, 0x110000 };
    assert(!characters_icu_encode_utf16(invalid, 2, units, 64));
    const UChar isolated_lead[] = { 0xD800 };
    const UChar isolated_trail[] = { 0xDC00 };
    assert(!characters_icu_decode_utf16(isolated_lead, 1, points, 32));
    assert(!characters_icu_decode_utf16(isolated_trail, 1, points, 32));
    assert(characters_icu_decode_utf8("A😀", points, 2) == 2);
    assert(points[0] == 0x41 && points[1] == 0x1F600);
    assert(!characters_icu_decode_utf8("AA", points, 1));
    assert(!characters_icu_decode_utf8("\xC0\x80", points, 32));
    assert(!characters_icu_decode_utf8("\xED\xA0\x80", points, 32));
    assert(!characters_icu_decode_utf8("\xF4\x90\x80\x80", points, 32));
    assert(!characters_icu_decode_utf8("\xF0\x9F", points, 32));
    assert(!characters_icu_decode_utf8("", points, 32));
    assert(!characters_icu_decode_utf8(NULL, points, 32));
    assert(!characters_icu_decode_utf8("A", NULL, 32));
    assert(!characters_icu_decode_utf8("A", points, 0));
    gchar text[129];
    memset(text, 'A', 33);
    text[33] = 0;
    assert(!characters_icu_decode_utf8(text, points, 32));
    text[32] = 0;
    assert(characters_icu_decode_utf8(text, points, 32) == 32);
    assert(characters_scalar_is_valid(0) && characters_scalar_is_valid(0x0378));
    assert(characters_scalar_is_valid(0x10FFFF));
    assert(!characters_scalar_is_valid(0xD800) && !characters_scalar_is_valid(0xDFFF));
    assert(!characters_scalar_is_valid(0x110000));
}

/** Run the focused native ICU boundary checks. */
int main(void) {
    test_normalize_points();
    test_decomposition();
    test_graphemes();
    test_scalar_conversion();
    puts("4 ICU adapter contract groups passed");
    return 0;
}
