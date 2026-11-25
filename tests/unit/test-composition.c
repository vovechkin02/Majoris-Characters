#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "composition.h"

/** Compares an owned output without leaking it between assertions. */
static void expect_text(char *actual, const char *expected) {
    assert(actual != NULL);
    assert(strcmp(actual, expected) == 0);
    free(actual);
}

/** Counts grapheme clusters independently of scalar and byte length. */
static void test_graphemes(void) {
    assert(characters_composition_graphemes("") == 0);
    assert(characters_composition_graphemes("A\r\n") == 2);
    assert(characters_composition_graphemes("e\xCC\x81") == 1);
    assert(characters_composition_graphemes("👩🏽‍💻🇦🇺1️⃣©️") == 4);
    assert(characters_composition_graphemes("שלום عربي") == 9);
}

/** Preserves exact code points, separators, selectors and supplementary characters. */
static void test_formats(void) {
    const char *text = "A😀\xCC\x81\n";
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_CODE),
                "U+0041 U+1F600 U+0301 U+000A");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_DECIMAL),
                "65 128512 769 10");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_UTF8),
                "41 F0 9F 98 80 CC 81 0A");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_UTF16),
                "0041 D83D DE00 0301 000A");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_UTF32),
                "00000041 0001F600 00000301 0000000A");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_HTML_DECIMAL),
                "&#65;&#128512;&#769;&#10;");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_HTML_HEX),
                "&#x41;&#x1F600;&#x301;&#xA;");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_ESCAPE),
                "\\u0041\\U0001F600\\u0301\\u000A");
    expect_text(characters_composition_format(text, CHARACTERS_INSPECTOR_CSS),
                "\\41 \\1F600 \\301 \\A ");
    expect_text(characters_composition_format("©️", CHARACTERS_INSPECTOR_CODE),
                "U+00A9 U+FE0F");
}

/** Normalizes across adjacent characters only when explicitly requested. */
static void test_normalization(void) {
    expect_text(characters_composition_normalize("e\xCC\x81👩‍💻", "NFC"), "é👩‍💻");
    expect_text(characters_composition_normalize("é", "NFD"), "e\xCC\x81");
    expect_text(characters_composition_normalize("①ﬁ", "NFKC"), "1fi");
    expect_text(characters_composition_normalize("①é", "NFKD"), "1e\xCC\x81");
    expect_text(characters_composition_normalize("", "NFC"), "");
    expect_text(characters_composition_normalize("\uA7F1\U000323B0", "NFKC"),
                "S\U000323B0");
    expect_text(characters_composition_normalize("a\u0301\u1ADD", "NFD"),
                "a\u1ADD\u0301");
    assert(characters_composition_normalize("\xCD\xB8", "NFC") == NULL);
    assert(characters_composition_normalize("A", "invalid") == NULL);
}

/** Rejects malformed and oversized input while retaining long valid compositions. */
static void test_limits(void) {
    const char *invalid[] = { NULL, "\xED\xA0\x80", "\xC0\xAF", "\xF4\x90\x80\x80", "\xE2" };
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); index++) {
        assert(characters_composition_graphemes(invalid[index]) == -1);
        assert(characters_composition_format(invalid[index], CHARACTERS_INSPECTOR_CODE) == NULL);
        assert(characters_composition_normalize(invalid[index], "NFC") == NULL);
    }
    assert(characters_composition_format("", CHARACTERS_INSPECTOR_CODE) == NULL);
    assert(characters_composition_format("A", (CharactersInspectorFormat)999) == NULL);
    char *text = malloc(CHARACTERS_COMPOSITION_MAX_POINTS + 2);
    assert(text != NULL);
    memset(text, 'A', CHARACTERS_COMPOSITION_MAX_POINTS + 1);
    text[CHARACTERS_COMPOSITION_MAX_POINTS] = 0;
    assert(characters_composition_graphemes(text) == CHARACTERS_COMPOSITION_MAX_POINTS);
    char *output = characters_composition_format(text, CHARACTERS_INSPECTOR_CODE);
    assert(output != NULL && strlen(output) == CHARACTERS_COMPOSITION_MAX_POINTS * 7 - 1);
    free(output);
    expect_text(characters_composition_normalize(text, "NFC"), text);
    text[CHARACTERS_COMPOSITION_MAX_POINTS] = 'A';
    text[CHARACTERS_COMPOSITION_MAX_POINTS + 1] = 0;
    assert(characters_composition_graphemes(text) == -1);
    assert(characters_composition_format(text, CHARACTERS_INSPECTOR_CODE) == NULL);
    assert(characters_composition_normalize(text, "NFC") == NULL);
    free(text);
    size_t count = CHARACTERS_COMPOSITION_MAX_POINTS;
    text = malloc(count * 2 + 1);
    assert(text != NULL);
    for (size_t index = 0; index < count; index++) {
        memcpy(text + index * 2, "é", 2);
    }
    text[count * 2] = 0;
    assert(characters_composition_normalize(text, "NFD") == NULL);
    free(text);
    uint32_t points[] = { 0xD800 };
    assert(characters_inspector_format_points(points, 1, CHARACTERS_INSPECTOR_UTF8) == NULL);
}

/** Runs focused headless composition contract tests. */
int main(void) {
    test_graphemes();
    test_formats();
    test_normalization();
    test_limits();
    puts("Composition: 4 groups passed");
    return 0;
}
