#include "character-inspector.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "character-search.h"
#include "unicode-engine.h"

/** Check an owned value and release it once the expected representation is verified. */
static void expect_text(char *actual, const char *expected) {
    assert(actual && strcmp(actual, expected) == 0);
    free(actual);
}

/** Verify exact scalar encodings, supplementary planes, and numeric reference formatting. */
static void test_encodings(void) {
    CharactersInspector *snapshot = characters_inspector_from_text("Aé😀");
    assert(snapshot && characters_inspector_count(snapshot) == 3);
    assert(characters_inspector_point(snapshot, 0) == 0x41);
    assert(characters_inspector_point(snapshot, 2) == 0x1F600);
    assert(characters_inspector_point(snapshot, 3) == UINT32_MAX);
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_CODE),
                "U+0041 U+00E9 U+1F600");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_DECIMAL),
                "65 233 128512");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_UTF8),
                "41 C3 A9 F0 9F 98 80");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_UTF16),
                "0041 00E9 D83D DE00");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_UTF32),
                "00000041 000000E9 0001F600");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_HTML_DECIMAL),
                "&#65;&#233;&#128512;");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_HTML_HEX),
                "&#x41;&#xE9;&#x1F600;");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_ESCAPE),
                "\\u0041\\u00E9\\U0001F600");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_CSS),
                "\\41 \\E9 \\1F600 ");
    expect_text(characters_inspector_text(snapshot), "Aé😀");
    assert(!characters_inspector_format(snapshot, (CharactersInspectorFormat)99));
    characters_inspector_free(snapshot);
}

/** Verify exact emoji and named sequence identities after their original search job is freed. */
static void test_catalog_identity(void) {
    CharactersSearch *job = characters_search_new(CHARACTERS_SEARCH_ALL, -1, -1,
                                                   "U+1F469 U+200D U+1F4BB");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1);
    CharactersInspector *snapshot = characters_inspector_from_id(characters_search_id(job, 0));
    characters_search_free(job);
    assert(snapshot && characters_inspector_count(snapshot) == 3);
    expect_text(characters_inspector_name(snapshot), "WOMAN TECHNOLOGIST");
    expect_text(characters_inspector_display(snapshot), "👩‍💻");
    expect_text(characters_inspector_text(snapshot), "👩‍💻");
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_CODE),
                "U+1F469 U+200D U+1F4BB");
    char **related = characters_inspector_related(snapshot);
    assert(related);
    bool woman = false;
    bool computer = false;
    for (size_t index = 0; related[index]; index++) {
        woman |= strcmp(related[index], "👩") == 0;
        computer |= strcmp(related[index], "💻") == 0;
        free(related[index]);
    }
    free(related);
    assert(woman && computer);
    characters_inspector_free(snapshot);

    size_t total = characters_unicode_emoji_count() + characters_unicode_named_count();
    for (size_t index = 0; index < total; index++) {
        char *text = index < characters_unicode_emoji_count() ?
            characters_unicode_emoji_text(index) :
            characters_unicode_named_text(index - characters_unicode_emoji_count());
        snapshot = characters_inspector_from_text(text);
        assert(snapshot);
        expect_text(characters_inspector_text(snapshot), text);
        char *code = characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_CODE);
        CharactersInspector *roundtrip = characters_inspector_from_code(code);
        assert(roundtrip);
        expect_text(characters_inspector_text(roundtrip), text);
        free(code);
        free(text);
        characters_inspector_free(roundtrip);
        characters_inspector_free(snapshot);
    }
}

/** Preserve controls and combining marks, while preventing preview substitution in copy data. */
static void test_nonprinting(void) {
    CharactersInspector *snapshot = characters_inspector_from_id(0);
    assert(snapshot && characters_inspector_count(snapshot) == 1);
    expect_text(characters_inspector_display(snapshot), "·");
    assert(!characters_inspector_text(snapshot));
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_UTF8), "00");
    expect_text(characters_inspector_normalize(snapshot, "NFC"), "U+0000");
    characters_inspector_free(snapshot);

    snapshot = characters_inspector_from_code("U+0065 U+0301 U+0000 U+00E9");
    assert(snapshot && !characters_inspector_text(snapshot));
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_UTF8),
                "65 CC 81 00 C3 A9");
    expect_text(characters_inspector_normalize(snapshot, "NFC"), "U+00E9 U+0000 U+00E9");
    expect_text(characters_inspector_normalize(snapshot, "NFD"),
                "U+0065 U+0301 U+0000 U+0065 U+0301");
    assert(!characters_inspector_related(snapshot));
    characters_inspector_free(snapshot);

    snapshot = characters_inspector_from_text("\n\t ");
    assert(snapshot);
    expect_text(characters_inspector_display(snapshot), "···");
    expect_text(characters_inspector_text(snapshot), "\n\t ");
    characters_inspector_free(snapshot);

    snapshot = characters_inspector_from_text("\xCC\x81");
    assert(snapshot);
    expect_text(characters_inspector_display(snapshot), "◌́");
    expect_text(characters_inspector_text(snapshot), "\xCC\x81");
    characters_inspector_free(snapshot);
}

/** Check all four normalization forms, expansion, and unavailable property handling. */
static void test_normalization(void) {
    CharactersInspector *snapshot = characters_inspector_from_text("é①");
    assert(snapshot);
    expect_text(characters_inspector_normalize(snapshot, "NFC"), "U+00E9 U+2460");
    expect_text(characters_inspector_normalize(snapshot, "NFD"), "U+0065 U+0301 U+2460");
    expect_text(characters_inspector_normalize(snapshot, "NFKC"), "U+00E9 U+0031");
    expect_text(characters_inspector_normalize(snapshot, "NFKD"), "U+0065 U+0301 U+0031");
    assert(!characters_inspector_normalize(snapshot, "invalid"));
    assert(!characters_inspector_normalize(snapshot, NULL));
    characters_inspector_free(snapshot);

    char code[32 * 8 + 1] = "";
    for (size_t index = 0; index < 32; index++) {
        strcat(code, "U+FB03 ");
    }
    snapshot = characters_inspector_from_code(code);
    assert(snapshot && characters_inspector_count(snapshot) == 32);
    char *normalized = characters_inspector_normalize(snapshot, "NFKD");
    assert(normalized && strlen(normalized) == 96 * 7 - 1);
    free(normalized);
    characters_inspector_free(snapshot);

    snapshot = characters_inspector_from_code("U+0378");
    assert(snapshot && !characters_inspector_name(snapshot));
    assert(!characters_inspector_normalize(snapshot, "NFC"));
    expect_text(characters_inspector_format(snapshot, CHARACTERS_INSPECTOR_CODE), "U+0378");
    characters_inspector_free(snapshot);
}

/** Reject malformed UTF-8, surrogates, invalid numeric tokens, and oversized sequences. */
static void test_invalid_input(void) {
    const char *invalid[] = { "", "U+", "U+D800", "U+110000", "U+0041 garbage",
                              "U+-1", "dec:1114112", "U+FFFFFFFF", "U+0041, U+0042" };
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); index++) {
        assert(!characters_inspector_from_code(invalid[index]));
    }
    assert(!characters_inspector_from_code(NULL));
    assert(!characters_inspector_from_text(NULL));
    assert(!characters_inspector_from_text(""));
    assert(!characters_inspector_from_text("\xED\xA0\x80"));
    assert(!characters_inspector_from_text("\xC0\x80"));
    assert(!characters_inspector_from_text("012345678901234567890123456789012"));
    assert(!characters_inspector_from_id(UINT32_MAX));
    assert(!characters_inspector_from_id(0xD800));
    assert(!characters_inspector_from_id(0x0378));
    assert(characters_inspector_count(NULL) == 0);
    assert(characters_inspector_point(NULL, 0) == UINT32_MAX);
    assert(!characters_inspector_display(NULL));
    assert(!characters_inspector_format(NULL, CHARACTERS_INSPECTOR_CODE));
    assert(!characters_inspector_normalize(NULL, "NFC"));
    assert(!characters_inspector_related(NULL));
    characters_inspector_free(NULL);
    CharactersInspector *snapshot = characters_inspector_from_code("  u+0041\t0x42 dec:67\n");
    assert(snapshot);
    expect_text(characters_inspector_text(snapshot), "ABC");
    characters_inspector_free(snapshot);
}

/** Runs the native inspection contracts with assertions enabled. */
int main(void) {
    test_encodings();
    test_catalog_identity();
    test_nonprinting();
    test_normalization();
    test_invalid_input();
    puts("5 character inspector contract groups passed");
    return 0;
}
