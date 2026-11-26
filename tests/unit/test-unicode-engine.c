#include "unicode-engine.h"
#include "scalar-properties.h"
#include "unihan.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>
#include <unicode/utf8.h>

#include <stdint.h>

#include <glib.h>

/** Compare a newly allocated value and release it after the assertion. */
static void expect_text(gchar *actual, const gchar *expected) {
    assert(actual && strcmp(actual, expected) == 0);
    g_free(actual);
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
    gchar *aliases = characters_unicode_aliases(0);
    assert(aliases && strstr(aliases, "NULL") && strstr(aliases, "NUL"));
    g_free(aliases);
    assert(strcmp(characters_scalar_text('A', CHARACTERS_SCALAR_TEXT_CATEGORY), "Lu") == 0);
    assert(strcmp(characters_scalar_text('A', CHARACTERS_SCALAR_TEXT_SCRIPT), "Latn") == 0);
    assert(strcmp(characters_unicode_script_name("Latn"), "Latin") == 0);
    assert(strcmp(characters_unicode_script_name("Hira"), "Hiragana") == 0);
    assert(strcmp(characters_unicode_script_name("Berf"), "Beria_Erfe") == 0);
    assert(!characters_unicode_script_name("invalid"));
    assert(!characters_unicode_script_name(NULL));
    assert(characters_scalar_age('A') == 11);
    assert(characters_scalar_combining_class(0x301) == 230);
    assert(characters_scalar_boolean_flags('(') & 1u);
    assert(strcmp(characters_scalar_text(0x5D0, CHARACTERS_SCALAR_TEXT_BIDI), "R") == 0);
    const gchar *scripts = characters_scalar_text(0x30FC, CHARACTERS_SCALAR_TEXT_EXTENSIONS);
    assert(scripts && strstr(scripts, "Hira") && strstr(scripts, "Kana"));
    assert(!characters_scalar_text(0x0378, CHARACTERS_SCALAR_TEXT_SCRIPT));
    assert(characters_scalar_age(0x0378) == -1);
    assert(!characters_scalar_property_alias("invalid"));
    assert(strcmp(characters_scalar_property_alias("gc"), "General_Category") == 0);
    assert(!characters_scalar_text('A', (CharactersScalarText)99));
    assert(characters_scalar_age(0xD800) == -1);
    assert(characters_scalar_boolean_flags('A') == 0);
    assert(!characters_scalar_text(0xD800, CHARACTERS_SCALAR_TEXT_CATEGORY));
    assert(strcmp(characters_scalar_property_alias("bidi-m"), "Bidi_Mirrored") == 0);
    assert(!characters_scalar_property_alias("ccc"));
    assert(!characters_scalar_property_alias(NULL));
    const gchar *reading = characters_unihan_value(0x4E2D,
                                                   CHARACTERS_HAN_VALUE_MANDARIN_FALLBACK);
    assert(reading && strstr(reading, "zhōng"));
    assert(strcmp(characters_unihan_value(0x4E2D, CHARACTERS_HAN_VALUE_HANGUL_FALLBACK),
                  "중") == 0);
    assert(characters_unihan_collation(0x4E2D, TRUE) == 2);
    assert(characters_unihan_collation(0x4E2D, FALSE) == 4);
    assert(!characters_unihan_value(0x4E2D, (CharactersHanValue)-1));
    assert(!characters_unihan_value(0x4E2D, CHARACTERS_HAN_VALUE_COUNT));
    assert(!characters_unihan_value('A', CHARACTERS_HAN_VALUE_MANDARIN_FALLBACK));
    assert(characters_unihan_collation('A', TRUE) == 0);
    gchar *version = characters_unicode_properties_version();
    assert(version && *version);
    printf("Property data: %s; catalog: %s; supplement: %s\n", version,
           characters_unicode_catalog_version(), characters_unicode_supplement_version());
    g_free(version);
}

/** Verify Unicode 17 coverage independently of the installed ICU repertoire. */
static void test_unicode_17(void) {
    expect_text(characters_unicode_properties_version(), "17.0.0");
    assert(strcmp(characters_unicode_catalog_version(), "17.0.0") == 0);
    assert(strcmp(characters_unicode_supplement_version(), "17.0.0") == 0);
    expect_text(characters_unicode_name(0x323B0), "CJK UNIFIED IDEOGRAPH-323B0");
    expect_text(characters_unicode_name(0x187F8), "TANGUT IDEOGRAPH-187F8");
    expect_text(characters_unicode_name(0x1FAEA), "DISTORTED FACE");
    assert(strcmp(characters_scalar_text(0x16EA0, CHARACTERS_SCALAR_TEXT_SCRIPT), "Berf") == 0);
    assert(strcmp(characters_scalar_text(0x16EA0, CHARACTERS_SCALAR_TEXT_CATEGORY), "Lu") == 0);
    assert(characters_scalar_age(0x16EA0) == 170);
    assert(strcmp(characters_scalar_text(0x0295, CHARACTERS_SCALAR_TEXT_CATEGORY), "Lo") == 0);
    assert(characters_scalar_combining_class(0x1ADD) == 220);
    assert(characters_scalar_boolean_flags(0x1FAEA) & 2u);
    expect_text(characters_unicode_aliases(0x16881), "BAMUM LETTER PHASE-B PUNGGAAM");
    expect_text(characters_unicode_aliases(0x12327), "CUNEIFORM SIGN KALAM");
    const gchar *scripts = characters_scalar_text(0x0306, CHARACTERS_SCALAR_TEXT_EXTENSIONS);
    assert(scripts && strstr(scripts, "Tfng"));
    gsize assigned = 0;
    for (gunichar point = 0; point <= 0x10FFFF; point++) {
        if (!characters_unicode_is_assigned(point)) {
            continue;
        }
        assigned++;
        gint category = characters_scalar_category(point);
        if (category != U_CONTROL_CHAR && category != U_PRIVATE_USE_CHAR) {
            gchar *name = characters_unicode_name(point);
            assert(name && *name);
            g_free(name);
        }
    }
    assert(assigned == 297334);
}

/** Check ordered, nonoverlapping blocks and both inclusive endpoints. */
static void test_blocks(void) {
    gsize count = characters_unicode_block_count();
    assert(count > 300);
    for (gsize index = 0; index < count; index++) {
        guint32 start = characters_unicode_block_start(index);
        guint32 end = characters_unicode_block_end(index);
        const gchar *name = characters_unicode_block_name(index);
        assert(start <= end && end <= 0x10FFFF && name);
        assert(strcmp(name, characters_unicode_find_block(start)) == 0);
        assert(strcmp(name, characters_unicode_find_block(end)) == 0);
        if (index) {
            assert(start > characters_unicode_block_end(index - 1));
        }
    }
    assert(!characters_unicode_block_name(count));
    assert(characters_unicode_block_start(count) == G_MAXUINT32);
    assert(!characters_unicode_find_block(0x110000));
    printf("Blocks: %zu\n", count);
}

/** Convert a short catalog string to numeric scalars for round-trip tests. */
static gsize decode(const gchar *text, gunichar points[32]) {
    gsize count = 0;
    int32_t length = (int32_t)strlen(text);
    for (int32_t offset = 0; offset < length;) {
        UChar32 point;
        U8_NEXT(text, offset, length, point);
        assert(point >= 0 && count < 32);
        points[count++] = (guint32)point;
    }
    return count;
}

/** Verify every emoji and named sequence, group membership and invalid indexes. */
static void test_catalogs(void) {
    gsize count = characters_unicode_emoji_count();
    assert(count > 3900);
    gboolean *seen = g_malloc0_n(count, sizeof(*seen));
    assert(seen);
    gsize group_count = characters_unicode_emoji_group_count();
    assert(group_count == 9);
    for (gsize group = 0; group < group_count; group++) {
        assert(characters_unicode_emoji_group_name(group));
        for (gsize index = 0; index < characters_unicode_emoji_group_size(group); index++) {
            gsize position = characters_unicode_emoji_group_index(group, index);
            assert(position < count && !seen[position]);
            seen[position] = TRUE;
        }
    }
    for (gsize index = 0; index < count; index++) {
        assert(seen[index]);
        gchar *text = characters_unicode_emoji_text(index);
        const gchar *label = characters_unicode_emoji_label(index);
        assert(text && label && *label);
        gunichar points[32];
        gsize length = decode(text, points);
        gsize borrowed_count;
        const gunichar *borrowed = characters_unicode_emoji_points(index, &borrowed_count);
        assert(borrowed && borrowed_count == length &&
               memcmp(points, borrowed, length * sizeof(*points)) == 0);
        expect_text(characters_unicode_encode(points, length), text);
        if (length > 1) {
            gchar *name = characters_unicode_catalog_sequence_name(points, length);
            assert(name && *name);
            g_free(name);
        }
        g_free(text);
    }
    g_free(seen);
    assert(!characters_unicode_emoji_text(count));
    assert(characters_unicode_emoji_group_index(group_count, 0) == G_MAXSIZE);
    gsize named_count = characters_unicode_named_count();
    assert(named_count > 400);
    for (gsize index = 0; index < named_count; index++) {
        gchar *text = characters_unicode_named_text(index);
        const gchar *name = characters_unicode_named_name(index);
        assert(text && name);
        gunichar points[32];
        gsize length = decode(text, points);
        gsize borrowed_count;
        const gunichar *borrowed = characters_unicode_named_points(index, &borrowed_count);
        assert(borrowed && borrowed_count == length &&
               memcmp(points, borrowed, length * sizeof(*points)) == 0);
        expect_text(characters_unicode_catalog_sequence_name(points, length), name);
        g_free(text);
    }
    assert(!characters_unicode_named_name(named_count));
    gsize invalid_count = 1;
    assert(!characters_unicode_emoji_points(count, &invalid_count) && !invalid_count);
    invalid_count = 1;
    assert(!characters_unicode_named_points(named_count, &invalid_count) && !invalid_count);
    assert(!characters_unicode_emoji_points(0, NULL));
    assert(!characters_unicode_named_points(0, NULL));
    printf("Emoji: %zu; named sequences: %zu\n", count, named_count);
}

/** Check exact sequence encoding, normalization forms and malformed input. */
static void test_encoding(void) {
    gunichar points[] = { 0x1F469, 0x200D, 0x1F4BB };
    expect_text(characters_unicode_encode(points, 3), "👩‍💻");
    expect_text(characters_unicode_catalog_sequence_name(points, 3), "WOMAN TECHNOLOGIST");
    expect_text(characters_unicode_normalize("e\xCC\x81", "NFC"), "é");
    expect_text(characters_unicode_normalize("é", "NFD"), "e\xCC\x81");
    expect_text(characters_unicode_normalize("ﬀ", "NFKC"), "ff");
    expect_text(characters_unicode_normalize("ﬀ", "NFKD"), "ff");
    expect_text(characters_unicode_normalize("", "NFC"), "");
    expect_text(characters_unicode_normalize("\uA7F1", "NFKD"), "S");
    expect_text(characters_unicode_normalize("\uA7F1", "NFC"), "\uA7F1");
    expect_text(characters_unicode_normalize("a\u0301\u1ADD", "NFD"),
                "a\u1ADD\u0301");
    expect_text(characters_unicode_normalize("a\u0301\u1ADD", "NFC"),
                "\u00E1\u1ADD");
    expect_text(characters_unicode_normalize("a\u1ACF\u0301", "NFC"),
                "a\u1ACF\u0301");
    expect_text(characters_unicode_normalize("\U000323B0", "NFKC"), "\U000323B0");
    expect_text(characters_unicode_normalize("\u0378", "NFC"), "\u0378");
    assert(!characters_unicode_normalize("\xFF", "NFC"));
    assert(!characters_unicode_normalize("abc", "invalid"));
    assert(!characters_unicode_encode(points, 33));
    assert(!characters_unicode_encode(NULL, 1));
    guint32 invalid[] = { 0xD800 };
    assert(!characters_unicode_encode(invalid, 1));
    guint32 nul[] = { 0 };
    assert(!characters_unicode_encode(nul, 1));
}

/** Verify exact immutable confusable class ownership, bounds and empty results. */
static void test_confusables(void)
{
    gint count = 0;
    gunichar *items = characters_unicode_confusables('A', &count);
    assert(items && count > 0);
    gboolean found_greek = FALSE;
    gboolean found_original = FALSE;
    for (gint index = 0; index < count; index++) {
        assert(characters_unicode_is_scalar(items[index]));
        found_greek |= items[index] == 0x391;
        found_original |= items[index] == 'A';
    }
    assert(found_greek && found_original);
    items[0] = 0;
    g_free(items);
    items = characters_unicode_confusables('A', &count);
    assert(items && count > 0 && items[0] != 0);
    g_free(items);
    items = characters_unicode_confusables(0x0378, &count);
    assert(items && count == 0);
    g_free(items);
    assert(!characters_unicode_confusables(0x110000, &count));
    assert(count == 0);
    assert(!characters_unicode_confusables(0xD800, &count));
    assert(count == 0);
    assert(!characters_unicode_confusables('A', NULL));
}

/** Verify the range cursor agrees with every scalar's independent category lookup. */
static void test_category_spans(void)
{
    gsize assigned = 0;
    gsize unassigned = 0;
    for (gunichar first = 0; first <= 0x10FFFFu;) {
        if (first == 0xD800u) {
            first = 0xE000u;
        }
        gunichar last;
        gint category = characters_scalar_category_span(first, &last);
        assert(last >= first && last <= 0x10FFFFu);
        for (gunichar point = first; point <= last; point++) {
            assert(characters_unicode_is_scalar(point));
            assert(characters_scalar_category(point) == category);
        }
        if (category == U_UNASSIGNED) {
            unassigned += last - first + 1;
        } else {
            assigned += last - first + 1;
        }
        first = last + 1;
    }
    assert(assigned == 297334 && unassigned == 814730);
    gunichar last;
    assert(characters_scalar_category_span(0xD800u, &last) == U_UNASSIGNED && last == 0xD800u);
    assert(characters_scalar_category_span(0x110000u, &last) == U_UNASSIGNED && last == 0x110000u);
}

/** Run the headless native contract suite and report the catalog sizes. */
int main(void) {
    test_scalar_metadata();
    test_unicode_17();
    test_blocks();
    test_catalogs();
    test_encoding();
    test_confusables();
    test_category_spans();
    puts("Seven native Unicode test groups passed.");
    return 0;
}
