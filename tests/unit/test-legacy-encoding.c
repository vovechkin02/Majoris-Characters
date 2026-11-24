#include "character-search.h"
#include "legacy-encoding.h"
#include "unicode-engine.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Finds a stable test encoding by its exact label, independent of dropdown order. */
static CharactersLegacy *load(const char *name) {
    for (size_t index = 0; index < characters_legacy_count(); index++) {
        if (strcmp(characters_legacy_name(index), name) == 0) {
            return characters_legacy_load(index);
        }
    }
    return NULL;
}

/** Every advertised encoding has a complete sorted unique scalar repertoire. */
static void test_catalog(void) {
    assert(characters_legacy_count() == 35);
    for (size_t index = 0; index < characters_legacy_count(); index++) {
        CharactersLegacy *table = characters_legacy_load(index);
        assert(table);
        size_t length = 0;
        uint32_t *points = characters_legacy_points(table, &length);
        assert(points && length >= 128 && length <= 65792);
        for (size_t position = 0; position < length; position++) {
            assert(characters_unicode_is_scalar(points[position]));
            if (position) {
                assert(points[position] > points[position - 1]);
            }
        }
        printf("%s: %zu distinct scalars\n", characters_legacy_name(index), length);
        free(points);
        characters_legacy_free(table);
    }
    assert(!characters_legacy_name(SIZE_MAX));
    assert(!characters_legacy_load(SIZE_MAX));
}

/** A known byte sequence decodes exactly and appears in the reverse byte alternatives. */
static void expect_mapping(const char *encoding, const char *hex, uint32_t point) {
    CharactersLegacy *table = load(encoding);
    assert(table);
    assert(characters_legacy_decode(table, hex) == point);
    char *bytes = characters_legacy_bytes(table, point);
    assert(bytes && strstr(bytes, hex));
    free(bytes);
    characters_legacy_free(table);
}

/** Checks representative Western, Cyrillic, Japanese, Chinese, and Korean mappings. */
static void test_mappings(void) {
    expect_mapping("US-ASCII", "00", 0);
    expect_mapping("US-ASCII", "41", 'A');
    expect_mapping("ISO-8859-15", "A4", 0x20AC);
    expect_mapping("Windows-1252", "80", 0x20AC);
    expect_mapping("Windows-1251", "C0", 0x0410);
    expect_mapping("Shift JIS (Windows-932)", "82 A0", 0x3042);
    expect_mapping("GBK (Windows-936)", "D6 D0", 0x4E2D);
    expect_mapping("Korean (Windows-949)", "B0 A1", 0xAC00);
    expect_mapping("Big5 (Windows-950)", "A4 A4", 0x4E2D);
}

/** Rejects malformed bytes, unmapped inputs, lead bytes and multiple decoded scalars. */
static void test_invalid(void) {
    CharactersLegacy *ascii = load("US-ASCII");
    assert(ascii);
    const char *queries[] = { "", "A", "GG", "FF", "41 42", "0041", "0x41", " 41",
                              "41 ", "41  42", "414243" };
    for (size_t index = 0; index < sizeof(queries) / sizeof(queries[0]); index++) {
        assert(characters_legacy_decode(ascii, queries[index]) == UINT32_MAX);
    }
    assert(!characters_legacy_bytes(ascii, 0x20AC));
    assert(!characters_legacy_bytes(ascii, 0xD800));
    assert(!characters_legacy_bytes(ascii, UINT32_MAX));
    characters_legacy_free(ascii);
    CharactersLegacy *sjis = load("Shift JIS (Windows-932)");
    assert(sjis);
    assert(characters_legacy_decode(sjis, "82") == UINT32_MAX);
    assert(characters_legacy_decode(sjis, "82A0") == 0x3042);
    assert(characters_legacy_decode(sjis, "41 42") == UINT32_MAX);
    characters_legacy_free(sjis);
}

/** Searches copied mapped scalars, never broadening the repertoire or adding emoji sequences. */
static void test_repertoire(void) {
    uint32_t points[] = { 0, 'A', 'A', 0x0378, 0x20AC, 0xE000 };
    CharactersSearch *job = characters_search_new_points(points, 6, -1, "");
    assert(job);
    points[1] = 'Z';
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 5);
    assert(characters_search_id(job, 1) == 'A');
    assert(characters_search_id(job, 2) == 0x0378);
    char *name = characters_search_name(0x0378);
    char *code = characters_search_code(0x0378);
    assert(name && strcmp(name, "Unassigned Character") == 0);
    assert(code && strcmp(code, "U+0378") == 0);
    free(name);
    free(code);
    characters_search_free(job);
    job = characters_search_new_points(points, 6, -1, "U+20AC");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0x20AC);
    characters_search_free(job);
    job = characters_search_new_points(points, 6, -1, "U+1F600");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
    job = characters_search_new_points(points, 6, 7, "");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0xE000);
    characters_search_free(job);
    job = characters_search_new_points(NULL, 0, -1, "");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
    points[0] = 0xD800;
    assert(!characters_search_new_points(points, 6, -1, ""));
    assert(!characters_search_new_points(NULL, 1, -1, ""));
    points[0] = 0;
    job = characters_search_new_points(points, 6, -1, "");
    assert(job);
    characters_search_cancel(job);
    assert(characters_search_run(job) == CHARACTERS_SEARCH_CANCELED);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Runs focused legacy byte and exact repertoire contracts. */
int main(void) {
    test_catalog();
    test_mappings();
    test_invalid();
    test_repertoire();
    puts("4 legacy encoding and repertoire contract groups passed");
    return 0;
}
