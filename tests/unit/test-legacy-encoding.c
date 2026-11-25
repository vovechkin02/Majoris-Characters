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

/** Every advertised encoding has a nonempty sorted unique scalar repertoire. */
static void test_catalog(void) {
    assert(characters_legacy_count() == 77);
    for (size_t index = 0; index < characters_legacy_count(); index++) {
        CharactersLegacy *table = characters_legacy_load(index);
        assert(table);
        size_t length = 0;
        uint32_t *points = characters_legacy_points(table, &length);
        assert(points && length > 0 && length <= 0x110000u);
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

/** Returns the stable encoding index used by independent stream conversion. */
static size_t encoding_index(const char *name) {
    for (size_t index = 0; index < characters_legacy_count(); index++) {
        if (strcmp(characters_legacy_name(index), name) == 0) {
            return index;
        }
    }
    assert(!"Missing encoding");
    return SIZE_MAX;
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
    expect_mapping("EBCDIC 037", "C1", 'A');
    expect_mapping("GB18030-2022", "94 39 FC 36", 0x1F600);
    expect_mapping("GB18030-2022", "90 30 81 30", 0x10000);
    expect_mapping("EUC-JP", "8F A2 AF", 0x02D8);
    expect_mapping("EUC-TW", "8E A2 A1 A1", 0x4E42);
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

/** Validates a byte stream including any initial-state shifts against exact scalar output. */
static void expect_sequence(size_t encoding, const char *hex, const uint32_t *expected,
                            size_t count) {
    size_t length = 0;
    uint32_t *points = characters_legacy_decode_sequence(encoding, hex, &length);
    assert(points && length == count && memcmp(points, expected, count * sizeof(*points)) == 0);
    free(points);
}

/** Stateful and contextual input is reset per conversion; canonical output round-trips exactly. */
static void test_contextual(void) {
    size_t japanese = encoding_index("ISO-2022-JP");
    size_t chinese = encoding_index("HZ-GB-2312");
    size_t indic = encoding_index("ISCII Devanagari");
    assert(characters_legacy_is_contextual(japanese));
    assert(!characters_legacy_is_contextual(encoding_index("GB18030-2022")));
    uint32_t hiragana[] = { 0x3042 };
    uint32_t han[] = { 0x4E2D };
    uint32_t virama[] = { 0x094D, 0x200C };
    uint32_t joiner[] = { 0x094D, 0x200D };
    uint32_t mixed[] = { 'A', 0x3042, 'B' };
    expect_sequence(japanese, "1B 24 42 24 22 1B 28 42", hiragana, 1);
    expect_sequence(japanese, "41 1B 24 42 24 22 1B 28 42 42", mixed, 3);
    expect_sequence(chinese, "7E 7B 56 50 7E 7D", han, 1);
    expect_sequence(indic, "E8 E8", virama, 2);
    expect_sequence(indic, "E8 E9", joiner, 2);
    char *encoded = characters_legacy_encode_sequence(japanese, mixed, 3);
    assert(encoded);
    expect_sequence(japanese, encoded, mixed, 3);
    free(encoded);
    encoded = characters_legacy_encode_sequence(indic, virama, 2);
    assert(encoded);
    expect_sequence(indic, encoded, virama, 2);
    free(encoded);
    CharactersLegacy *table = characters_legacy_load(japanese);
    assert(table && characters_legacy_decode(table, "1B244224221B2842") == 0x3042);
    encoded = characters_legacy_bytes(table, 0x3042);
    assert(encoded);
    expect_sequence(japanese, encoded, hiragana, 1);
    free(encoded);
    characters_legacy_free(table);
    size_t length = 99;
    assert(!characters_legacy_decode_sequence(japanese, "1B 24", &length) && !length);
    assert(!characters_legacy_decode_sequence(japanese, "1B 24 42 24", &length) && !length);
    assert(!characters_legacy_decode_sequence(japanese, "1B 24 42", &length) && !length);
    uint32_t dollar[] = { '$', '"' };
    expect_sequence(japanese, "24 22", dollar, 2);
}

/** Strict sequence conversion preserves NUL and rejects malformed or lossy input. */
static void test_sequence_limits(void) {
    size_t ascii = encoding_index("US-ASCII");
    size_t gb = encoding_index("GB18030-2022");
    uint32_t mixed[] = { 'A', 0, 0x1F600 };
    expect_sequence(gb, "41 00 94 39 FC 36", mixed, 3);
    char *encoded = characters_legacy_encode_sequence(gb, mixed, 3);
    assert(encoded && strcmp(encoded, "41 00 94 39 FC 36") == 0);
    free(encoded);
    assert(!characters_legacy_encode_sequence(ascii, mixed, 3));
    uint32_t invalid[] = { 0xD800, 0x110000 };
    assert(!characters_legacy_encode_sequence(gb, invalid, 2));
    uint32_t points[33];
    for (size_t index = 0; index < 33; index++) {
        points[index] = 'A';
    }
    assert(!characters_legacy_encode_sequence(ascii, points, 33));
    assert(!characters_legacy_encode_sequence(ascii, points, 0));
    char hex[259];
    memset(hex, '4', sizeof(hex) - 1);
    hex[sizeof(hex) - 1] = '\0';
    size_t length = 10;
    assert(!characters_legacy_decode_sequence(ascii, hex, &length) && !length);
    hex[66] = '\0';
    assert(!characters_legacy_decode_sequence(ascii, hex, &length) && !length);
    hex[64] = '\0';
    uint32_t *decoded = characters_legacy_decode_sequence(ascii, hex, &length);
    assert(decoded && length == 32 && decoded[31] == 'D');
    free(decoded);
    assert(!characters_legacy_decode_sequence(gb, "94 39 FC", &length) && !length);
    assert(!characters_legacy_decode_sequence(ascii, "41  42", &length) && !length);
    assert(!characters_legacy_decode_sequence(ascii, "41 ", &length) && !length);
    assert(!characters_legacy_decode_sequence(SIZE_MAX, "41", &length) && !length);
    assert(!characters_legacy_encode_sequence(SIZE_MAX, points, 1));
    assert(!characters_legacy_decode_sequence(ascii, "41", NULL));
    uint32_t supplementary[32];
    for (size_t index = 0; index < 32; index++) {
        supplementary[index] = 0x1F600;
    }
    encoded = characters_legacy_encode_sequence(gb, supplementary, 32);
    assert(encoded && strlen(encoded) == 383);
    expect_sequence(gb, encoded, supplementary, 32);
    free(encoded);
}

/** GB18030 covers supplementary and unassigned scalars without exhausting grid search limits. */
static void test_large_repertoire(void) {
    CharactersLegacy *table = load("GB18030-2022");
    assert(table);
    size_t length = 0;
    uint32_t *points = characters_legacy_points(table, &length);
    assert(points && length > 1000000);
    CharactersSearch *job = characters_search_new_points(points, length, -1, "U+1F600");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0x1F600);
    characters_search_free(job);
    job = characters_search_new_points(points, length, -1, "");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == length);
    characters_search_free(job);
    free(points);
    characters_legacy_free(table);
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
    test_contextual();
    test_sequence_limits();
    test_large_repertoire();
    test_repertoire();
    puts("7 legacy encoding and repertoire contract groups passed");
    return 0;
}
