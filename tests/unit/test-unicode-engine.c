#include "unicode-engine.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/utf8.h>

/** Compare a newly allocated value and release it after the assertion. */
static void expect_text(char *actual, const char *expected) {
    assert(actual && strcmp(actual, expected) == 0);
    free(actual);
}

/** Check scalar validity, names, aliases, assignment and metadata version boundaries. */
static void test_scalar_metadata(void) {
    assert(characters_unicode_is_scalar(0));
    assert(characters_unicode_is_scalar(0x10FFFF));
    assert(!characters_unicode_is_scalar(0xD800));
    assert(!characters_unicode_is_scalar(0x110000));
    assert(characters_unicode_is_assigned(0xE000));
    assert(!characters_unicode_is_assigned(0x0378));
    assert(!characters_unicode_is_assigned(0xDFFF));
    assert(!characters_unicode_name(0));
    assert(!characters_unicode_name(0xE000));
    assert(!characters_unicode_name(0x0378));
    expect_text(characters_unicode_name(0x1F600), "GRINNING FACE");
    expect_text(characters_unicode_name(0x4E00), "CJK UNIFIED IDEOGRAPH-4E00");
    expect_text(characters_unicode_name(0xAC00), "HANGUL SYLLABLE GA");
    char *aliases = characters_unicode_aliases(0);
    assert(aliases && strstr(aliases, "NULL") && strstr(aliases, "NUL"));
    free(aliases);
    expect_text(characters_unicode_property('A', "General_Category"), "Lu");
    expect_text(characters_unicode_property('A', "Script"), "Latn");
    expect_text(characters_unicode_property('A', "Age"), "1.1");
    expect_text(characters_unicode_property(0x301, "Canonical_Combining_Class"), "230");
    expect_text(characters_unicode_property(0x1F600, "Plane"), "1");
    expect_text(characters_unicode_property('(', "Bidi_Mirrored"), "Y");
    expect_text(characters_unicode_property(0x5D0, "Bidi_Class"), "R");
    char *scripts = characters_unicode_property(0x30FC, "Script_Extensions");
    assert(scripts && strstr(scripts, "Hira") && strstr(scripts, "Kana"));
    free(scripts);
    assert(!characters_unicode_property(0x0378, "Script"));
    assert(!characters_unicode_property('A', "invalid"));
    assert(!characters_unicode_property(0xD800, "Plane"));
    char *version = characters_unicode_properties_version();
    assert(version && *version);
    printf("Property data: %s; catalog: %s; supplement: %s\n", version,
           characters_unicode_catalog_version(), characters_unicode_supplement_version());
    free(version);
}

/** Check ordered, nonoverlapping blocks and both inclusive endpoints. */
static void test_blocks(void) {
    size_t count = characters_unicode_block_count();
    assert(count > 300);
    for (size_t index = 0; index < count; index++) {
        uint32_t start = characters_unicode_block_start(index);
        uint32_t end = characters_unicode_block_end(index);
        const char *name = characters_unicode_block_name(index);
        assert(start <= end && end <= 0x10FFFF && name);
        assert(strcmp(name, characters_unicode_find_block(start)) == 0);
        assert(strcmp(name, characters_unicode_find_block(end)) == 0);
        if (index) {
            assert(start > characters_unicode_block_end(index - 1));
        }
    }
    assert(!characters_unicode_block_name(count));
    assert(characters_unicode_block_start(count) == UINT32_MAX);
    assert(!characters_unicode_find_block(0x110000));
    printf("Blocks: %zu\n", count);
}

/** Convert a short catalog string to numeric scalars for round-trip tests. */
static size_t decode(const char *text, uint32_t points[32]) {
    size_t count = 0;
    int32_t length = (int32_t)strlen(text);
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT(text, offset, length, point);
        assert(point >= 0 && count < 32);
        points[count++] = (uint32_t)point;
    }
    return count;
}

/** Verify every emoji and named sequence, group membership and invalid indexes. */
static void test_catalogs(void) {
    size_t count = characters_unicode_emoji_count();
    assert(count > 3900);
    bool *seen = calloc(count, sizeof(*seen));
    assert(seen);
    size_t group_count = characters_unicode_emoji_group_count();
    assert(group_count == 9);
    for (size_t group = 0; group < group_count; group++) {
        assert(characters_unicode_emoji_group_name(group));
        for (size_t index = 0; index < characters_unicode_emoji_group_size(group); index++) {
            size_t position = characters_unicode_emoji_group_index(group, index);
            assert(position < count && !seen[position]);
            seen[position] = true;
        }
    }
    for (size_t index = 0; index < count; index++) {
        assert(seen[index]);
        char *text = characters_unicode_emoji_text(index);
        const char *label = characters_unicode_emoji_label(index);
        assert(text && label && *label);
        uint32_t points[32];
        size_t length = decode(text, points);
        expect_text(characters_unicode_encode(points, length), text);
        if (length > 1) {
            char *name = characters_unicode_sequence_name(points, length);
            assert(name && *name);
            free(name);
        }
        free(text);
    }
    free(seen);
    assert(!characters_unicode_emoji_text(count));
    assert(characters_unicode_emoji_group_index(group_count, 0) == SIZE_MAX);
    size_t named_count = characters_unicode_named_count();
    assert(named_count > 400);
    for (size_t index = 0; index < named_count; index++) {
        char *text = characters_unicode_named_text(index);
        const char *name = characters_unicode_named_name(index);
        assert(text && name);
        uint32_t points[32];
        size_t length = decode(text, points);
        expect_text(characters_unicode_sequence_name(points, length), name);
        free(text);
    }
    assert(!characters_unicode_named_name(named_count));
    printf("Emoji: %zu; named sequences: %zu\n", count, named_count);
}

/** Check exact sequence encoding, normalization forms and malformed input. */
static void test_encoding(void) {
    uint32_t points[] = { 0x1F469, 0x200D, 0x1F4BB };
    expect_text(characters_unicode_encode(points, 3), "👩‍💻");
    expect_text(characters_unicode_sequence_name(points, 3), "WOMAN TECHNOLOGIST");
    expect_text(characters_unicode_normalize("e\xCC\x81", "NFC"), "é");
    expect_text(characters_unicode_normalize("é", "NFD"), "e\xCC\x81");
    expect_text(characters_unicode_normalize("ﬀ", "NFKC"), "ff");
    expect_text(characters_unicode_normalize("ﬀ", "NFKD"), "ff");
    expect_text(characters_unicode_normalize("", "NFC"), "");
    assert(!characters_unicode_normalize("\xFF", "NFC"));
    assert(!characters_unicode_normalize("abc", "invalid"));
    assert(!characters_unicode_encode(points, 33));
    assert(!characters_unicode_encode(NULL, 1));
    uint32_t invalid[] = { 0xD800 };
    assert(!characters_unicode_encode(invalid, 1));
    uint32_t nul[] = { 0 };
    assert(!characters_unicode_encode(nul, 1));
    uint32_t embedded[] = { 'a', 0 };
    assert(!characters_unicode_related(embedded, 2));
}

/** Assert membership, sorted uniqueness and input exclusion, then release results. */
static void expect_related(uint32_t point, const char *expected) {
    char **items = characters_unicode_related(&point, 1);
    assert(items);
    char *original = characters_unicode_encode(&point, 1);
    bool found = false;
    for (size_t index = 0; items[index]; index++) {
        assert(strcmp(original, items[index]) != 0);
        if (index) {
            assert(strcmp(items[index - 1], items[index]) < 0);
        }
        found |= strcmp(items[index], expected) == 0;
    }
    assert(found);
    for (size_t index = 0; items[index]; index++) {
        free(items[index]);
    }
    free(items);
    free(original);
}

/** Verify case, mirror, canonical, Hangul and confusable relations. */
static void test_relationships(void) {
    expect_related('a', "A");
    expect_related('(', ")");
    expect_related('e', "é");
    expect_related(0xE9, "e");
    expect_related(0xAC00, "ᄀ");
    expect_related('A', "Α");
    expect_related(0xA9, "©️");
    uint32_t invalid = 0x110000;
    assert(!characters_unicode_related(&invalid, 1));
}

/** Run the headless native contract suite and report the catalog sizes. */
int main(void) {
    test_scalar_metadata();
    test_blocks();
    test_catalogs();
    test_encoding();
    test_relationships();
    puts("Five native Unicode test groups passed.");
    return 0;
}
