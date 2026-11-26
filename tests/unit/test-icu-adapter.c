#include "icu-adapter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "icu-scalar-conversion.h"
#include "scalar-properties.h"
#include "unicode-algorithms.h"
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

/** Verify raw simple mappings and recursive canonical (not compatibility) decomposition. */
static void test_relationship_primitives(void)
{
    assert(characters_icu_uppercase('a') == 'A');
    assert(characters_icu_lowercase('A') == 'a');
    assert(characters_icu_uppercase(0x01C5) == 0x01C4);
    assert(characters_icu_lowercase(0x01C5) == 0x01C6);
    assert(characters_icu_titlecase(0x01C4) == 0x01C5);
    assert(characters_icu_mirror('(') == ')');
    assert(characters_icu_mirror('A') == 'A');
    assert(characters_icu_uppercase(0x110000) == 0x110000);
    assert(characters_icu_lowercase(0xD800) == 0xD800);
    assert(characters_icu_titlecase(0xDFFF) == 0xDFFF);
    assert(characters_icu_mirror(0x110000) == 0x110000);
    gunichar points[CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH];
    assert(characters_icu_canonical_decomposition(0x01FA, points) == 3);
    assert(points[0] == 'A' && points[1] == 0x030A && points[2] == 0x0301);
    assert(characters_icu_canonical_decomposition(0xAC01, points) == 3);
    assert(points[0] == 0x1100 && points[1] == 0x1161 && points[2] == 0x11A8);
    assert(characters_icu_canonical_decomposition(0x304C, points) == 2);
    assert(points[0] == 0x304B && points[1] == 0x3099);
    assert(!characters_icu_canonical_decomposition(0x00BC, points));
    assert(!characters_icu_canonical_decomposition(0xA7F1, points));
    assert(!characters_icu_canonical_decomposition(0x0378, points));
    assert(!characters_icu_canonical_decomposition(0xD800, points));
    assert(!characters_icu_canonical_decomposition(0x110000, points));
    assert(!characters_icu_canonical_decomposition(0x01FA, NULL));
}

/** Check ICU grapheme segmentation independently of the Vala input validation policy. */
static void test_graphemes(void) {
    assert(characters_icu_graphemes("") == 0);
    assert(characters_icu_graphemes("A\r\n") == 2);
    assert(characters_icu_graphemes("e\xCC\x81") == 1);
    assert(characters_icu_graphemes("👩🏽‍💻🇦🇺1️⃣©️") == 4);
    assert(characters_icu_graphemes(NULL) == -1);
}

/** Check Unicode 17 grapheme rules, explicit NUL, and malformed input. */
static void test_unicode17_graphemes(void)
{
    const gunichar indic[] = { 0x0915, 0x094D, 0x200D, 0x0915 };
    const gunichar flags[] = { 0x1F1E6, 0x1F1E7, 0x1F1E8, 0x1F1E9, 0x1F1EA };
    const gunichar new_mark[] = { 0x61, 0x1ADD };
    const gunichar controls[] = { 0x61, 0, 0x0301, 0x0D, 0x0A };
    const gunichar hangul[] = { 0x1100, 0x1161, 0x11A8 };
    const gunichar invalid[] = { 0xD800 };
    assert(characters_unicode_graphemes_points(indic, G_N_ELEMENTS(indic)) == 1);
    assert(characters_unicode_graphemes_points(flags, G_N_ELEMENTS(flags)) == 3);
    assert(characters_unicode_graphemes_points(new_mark, G_N_ELEMENTS(new_mark)) == 1);
    assert(characters_unicode_graphemes_points(controls, G_N_ELEMENTS(controls)) == 4);
    assert(characters_unicode_graphemes_points(hangul, G_N_ELEMENTS(hangul)) == 1);
    assert(characters_unicode_graphemes_points(invalid, 1) == -1);
    assert(characters_unicode_graphemes_points(NULL, 0) == 0);
    assert(characters_icu_graphemes("\xED\xA0\x80") == -1);
    assert(characters_icu_graphemes("\xF0\x9F") == -1);
}

/** Verify full folding, Unicode 17 mappings, compatibility, and ignorable removal. */
static void test_unicode17_folding(void)
{
    assert(characters_icu_lowercase(0x16EA0) == 0x16EBB);
    assert(characters_icu_uppercase(0x16EBB) == 0x16EA0);
    assert(characters_icu_titlecase(0xA7CF) == 0xA7CE);
    assert(characters_icu_lowercase(0xA7D4) == 0xA7D5);
    gchar *text = characters_unicode_casefold("Straße I İ ı");
    assert(text && !strcmp(text, "strasse i i\xCC\x87 ı"));
    g_free(text);
    text = characters_unicode_casefold("\U00016EA0\uA7CE\uA7D2");
    assert(text && !strcmp(text, "\U00016EBB\uA7CF\uA7D3"));
    g_free(text);
    text = characters_unicode_nfkc_casefold("ＦＵＬＬ\uA7F1\u200D\uFE0F");
    assert(text && !strcmp(text, "fulls"));
    g_free(text);
    text = characters_unicode_nfkc_casefold("E\u0301");
    assert(text && !strcmp(text, "é"));
    g_free(text);
    text = characters_unicode_nfkc_casefold("\u200D");
    assert(text && !*text);
    g_free(text);
    assert(!characters_unicode_casefold("\xFF"));
    assert(!characters_unicode_nfkc_casefold("\xC0\x80"));
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
    test_relationship_primitives();
    test_graphemes();
    test_scalar_conversion();
    test_unicode17_graphemes();
    test_unicode17_folding();
    puts("7 ICU adapter contract groups passed");
    return 0;
}
