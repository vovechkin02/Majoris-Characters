#include "emoji-catalog.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "character-inspector.h"
#include "character-search.h"
#include "unicode-engine.h"

/** Check for an exact string in a variant collection, never a preview substitution. */
static bool has_variant(const CharactersEmojiVariants *variants, const char *expected) {
    for (size_t index = 0; index < characters_emoji_variants_count(variants); index++) {
        if (strcmp(characters_emoji_variants_text(variants, index), expected) == 0) {
            return true;
        }
    }
    return false;
}

/** Require an exact alternative, and ensure all returned strings are unique and labeled. */
static void expect_family(const char *input, const char *expected, size_t minimum) {
    CharactersEmojiVariants *variants = characters_emoji_variants_new(input);
    assert(variants);
    size_t count = characters_emoji_variants_count(variants);
    assert(count >= minimum);
    assert(has_variant(variants, input));
    assert(has_variant(variants, expected));
    for (size_t index = 0; index < count; index++) {
        const char *text = characters_emoji_variants_text(variants, index);
        assert(text && *text && characters_emoji_is_candidate(text));
        assert(characters_emoji_variants_label(variants, index));
        for (size_t previous = 0; previous < index; previous++) {
            assert(strcmp(text, characters_emoji_variants_text(variants, previous)) != 0);
        }
    }
    assert(!characters_emoji_variants_text(variants, count));
    assert(!characters_emoji_variants_label(variants, count));
    characters_emoji_variants_free(variants);
}

/** Check all five skin tones and mixed-tone handshake alternatives. */
static void test_modifiers(void) {
    expect_family("👍", "👍🏿", 6);
    expect_family("👍🏽", "👍🏻", 6);
    expect_family("🤝", "🫱🏻‍🫲🏿", 26);
    expect_family("🫱🏻‍🫲🏿", "🤝🏾", 26);
}

/** Check catalog-supported professions, gender, hair and direction variants. */
static void test_people(void) {
    expect_family("👩‍💻", "🧑🏽‍💻", 18);
    expect_family("👨", "👩‍🦰", 12);
    expect_family("🚶", "🚶🏽‍♀️‍➡️", 18);
    expect_family("👪", "🧑‍🧑‍🧒", 20);
    expect_family("💑", "👩🏻‍❤️‍👩🏿", 26);
    expect_family("💏", "👨🏿‍❤️‍💋‍👨🏻", 26);
    expect_family("👫", "🧑🏻‍🤝‍🧑🏿", 26);
}

/** Check flags, keycaps and standardized text/emoji presentation pairs. */
static void test_presentation(void) {
    expect_family("🇦🇺", "🇯🇵", 200);
    expect_family("1️⃣", "#️⃣", 12);
    expect_family("©️", "©︎", 3);
    expect_family("©︎", "©️", 3);
    expect_family("©", "©️", 3);
    CharactersInspector *snapshot = characters_inspector_from_text("©︎");
    assert(snapshot);
    char *display = characters_inspector_display(snapshot);
    assert(display && strcmp(display, "©︎") == 0);
    free(display);
    characters_inspector_free(snapshot);
    assert(!characters_emoji_is_candidate("A"));
    assert(!characters_emoji_variants_new("A️"));
    assert(!characters_emoji_variants_new("👍‍😀"));
    assert(!characters_emoji_variants_new("\xFF"));
    assert(!characters_emoji_variants_new(NULL));
}

/** Check every generated identity and both standardized selectors for all 371 bases. */
static void test_all_presentation_pairs(void) {
    size_t base_count = 0;
    size_t alias_count = characters_emoji_alias_count();
    assert(alias_count == 1819);
    for (size_t index = 0; index < alias_count; index++) {
        const char *text = characters_emoji_alias_text(index);
        assert(text && characters_emoji_alias_index(text) == index);
        assert(characters_emoji_is_candidate(text));
        assert(characters_emoji_alias_label(index));
        char *name = characters_emoji_alias_name(index);
        assert(name && *name);
        free(name);
        size_t length = strlen(text);
        if (length >= 3 && strcmp(text + length - 3, "\xEF\xB8\x8E") == 0) {
            base_count++;
            char *base = strndup(text, length - 3);
            char *emoji = strdup(text);
            assert(base && emoji);
            emoji[length - 1] = (char)0x8F;
            CharactersEmojiVariants *variants = characters_emoji_variants_new(text);
            assert(variants && strcmp(characters_emoji_variants_text(variants, 0), text) == 0);
            assert(has_variant(variants, base));
            assert(has_variant(variants, emoji));
            assert(has_variant(variants, text));
            characters_emoji_variants_free(variants);
            free(base);
            free(emoji);
        }
    }
    assert(base_count == 371);
    assert(!characters_emoji_alias_text(alias_count));
    assert(characters_emoji_alias_index("😀️") == SIZE_MAX);
    assert(!characters_emoji_is_candidate("😀️"));
    assert(!characters_emoji_is_candidate("A︎"));
    assert(!characters_emoji_is_candidate("👁️️‍🗨️"));
}

/** Test exact minimally qualified forms without silently substituting selectors. */
static void test_qualification(void) {
    expect_family("👁‍🗨", "👁️‍🗨️", 4);
    expect_family("🚶‍♀‍➡", "🚶‍♀️‍➡️", 18);
    expect_family("1⃣", "1️⃣", 12);
    expect_family("⌚︎", "⌚️", 3);
    expect_family("#︎", "#️", 3);
    CharactersEmojiVariants *variants = characters_emoji_variants_new("🚶‍♀‍➡");
    assert(variants && strcmp(characters_emoji_variants_text(variants, 0), "🚶‍♀‍➡") == 0);
    characters_emoji_variants_free(variants);
    CharactersInspector *snapshot = characters_inspector_from_text("⌚︎");
    assert(snapshot);
    char *name = characters_inspector_name(snapshot);
    assert(name && strstr(name, "WATCH") && strstr(name, "Text presentation"));
    free(name);
    characters_inspector_free(snapshot);
    uint32_t watch[] = { 0x231A };
    char **related = characters_unicode_related(watch, 1);
    assert(related);
    bool text_found = false;
    bool emoji_found = false;
    for (size_t index = 0; related[index]; index++) {
        text_found |= strcmp(related[index], "⌚︎") == 0;
        emoji_found |= strcmp(related[index], "⌚️") == 0;
    }
    assert(text_found && emoji_found);
    for (size_t index = 0; related[index]; index++) {
        free(related[index]);
    }
    free(related);
}

/** Test localized, case-folded annotation search through the production search worker. */
static void expect_search(const char *locale, const char *query, const char *expected) {
    CharactersSearch *job = characters_search_new(CHARACTERS_SEARCH_EMOJI, -1, -1, query);
    assert(job);
    characters_search_set_locale(job, locale);
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    bool found = false;
    for (size_t index = 0; index < characters_search_count(job); index++) {
        char *text = characters_search_text(characters_search_id(job, index));
        found |= text && strcmp(text, expected) == 0;
        free(text);
    }
    assert(found);
    characters_search_free(job);
}

/** Localized annotations extend English labels without changing exact code-point queries. */
static void test_annotations(void) {
    expect_search("en_US.UTF-8", "cheery", "😀");
    expect_search("fr_CA.UTF-8", "ÉMOTICÔNE", "😀");
    expect_search("FR-ca.UTF-8", "ÉMOTICÔNE", "😀");
    expect_search("en", "sasquatch", "🫈");
    expect_search("en", "marine", "🫍");
    expect_search("en", "mudslide", "🛘");
    expect_search("en", "jazz", "🪊");
    expect_search("en", "loot", "🪎");
    expect_search("en", "brawl", "🫯");
    expect_search("en", "bloated", "🫪");
    expect_search("en", "ballet", "🧑🏽‍🩰");
    expect_search("fr_FR", "joyeux dents", "😀");
    expect_search("zh_Hant_TW", "猫", "🐈");
    expect_search("unknown", "cheery", "😀");
    expect_search("fr", "U+1F600", "😀");
    expect_search("fr", "👩‍💻", "👩‍💻");
    CharactersSearch *job = characters_search_new(CHARACTERS_SEARCH_EMOJI, -1, -1, "U+1F600");
    assert(job);
    characters_search_set_locale(job, "fr");
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1);
    characters_search_free(job);
}

/** Validate exact alias search and catalog category restrictions in both browsing scopes. */
static void test_exact_alias_search(void) {
    const char *queries[] = { "U+231A U+FE0E", "⌚️", "👁‍🗨", "U+0031 U+20E3" };
    const char *expected[] = { "⌚︎", "⌚️", "👁‍🗨", "1⃣" };
    for (size_t index = 0; index < sizeof(queries) / sizeof(queries[0]); index++) {
        for (int scope = 0; scope < 2; scope++) {
            CharactersSearch *job = characters_search_new(
                scope ? CHARACTERS_SEARCH_EMOJI : CHARACTERS_SEARCH_ALL, -1, -1, queries[index]);
            assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
            assert(characters_search_count(job) == 1);
            uint32_t id = characters_search_id(job, 0);
            char *text = characters_search_text(id);
            char *name = characters_search_name(id);
            assert(text && strcmp(text, expected[index]) == 0 && name);
            free(text);
            free(name);
            characters_search_free(job);
        }
    }
    CharactersSearch *job = characters_search_new(CHARACTERS_SEARCH_EMOJI, 8, -1, "⌚︎");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Run the focused Stage 6 contracts without a graphical session. */
int main(void) {
    test_modifiers();
    test_people();
    test_presentation();
    test_annotations();
    test_all_presentation_pairs();
    test_qualification();
    test_exact_alias_search();
    puts("7 emoji variation and annotation contract groups passed");
    return 0;
}
