#include "emoji-catalog.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "search-contract-tests.h"
#include "unicode-engine.h"

/** Check every generated identity and both standardized selectors for all 371 bases. */
static void test_all_presentation_pairs(void) {
    gsize base_count = 0;
    gsize alias_count = characters_emoji_alias_count();
    assert(alias_count == 1819);
    for (gsize index = 0; index < alias_count; index++) {
        const gchar *text = characters_emoji_alias_text(index);
        assert(text && characters_emoji_alias_index(text) == index);
        assert(characters_emoji_is_candidate(text));
        assert(characters_emoji_alias_label(index));
        gchar *name = characters_emoji_alias_name(index);
        assert(name && *name);
        g_free(name);
        gsize length = strlen(text);
        if (length >= 3 && strcmp(text + length - 3, "\xEF\xB8\x8E") == 0) {
            base_count++;
        }
    }
    assert(base_count == 371);
    assert(!characters_emoji_alias_text(alias_count));
    assert(characters_emoji_alias_index("😀️") == G_MAXSIZE);
    assert(!characters_emoji_is_candidate("😀️"));
    assert(!characters_emoji_is_candidate("A︎"));
    assert(!characters_emoji_is_candidate("👁️️‍🗨️"));
}

/** Test exact minimally qualified forms without silently substituting selectors. */
static void test_qualification(void) {
    guint32 watch[] = { 0x231A };
    gint count = 0;
    gchar **related = characters_relationships_collect(watch, 1, &count);
    assert(related);
    gboolean text_found = FALSE;
    gboolean emoji_found = FALSE;
    for (gint index = 0; index < count; index++) {
        text_found |= strcmp(related[index], "⌚︎") == 0;
        emoji_found |= strcmp(related[index], "⌚️") == 0;
        g_free(related[index]);
    }
    assert(text_found && emoji_found);
    g_free(related);
}

/** Test localized, case-folded annotation search through the production search worker. */
static void expect_search(const gchar *locale, const gchar *query, const gchar *expected) {
    CharactersSearch *job = characters_query_parser_create(CHARACTERS_SEARCH_EMOJI, -1, -1, query);
    assert(job);
    characters_search_set_locale(job, locale);
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    gboolean found = FALSE;
    for (gsize index = 0; index < characters_search_count(job); index++) {
        gchar *text = characters_search_text(characters_search_id(job, index));
        found |= text && strcmp(text, expected) == 0;
        g_free(text);
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
    CharactersSearch *job = characters_query_parser_create(CHARACTERS_SEARCH_EMOJI, -1, -1, "U+1F600");
    assert(job);
    characters_search_set_locale(job, "fr");
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1);
    characters_search_free(job);
}

/** Validate exact alias search and catalog category restrictions in both browsing scopes. */
static void test_exact_alias_search(void) {
    const gchar *queries[] = { "U+231A U+FE0E", "⌚️", "👁‍🗨", "U+0031 U+20E3" };
    const gchar *expected[] = { "⌚︎", "⌚️", "👁‍🗨", "1⃣" };
    for (gsize index = 0; index < sizeof(queries) / sizeof(queries[0]); index++) {
        for (gint scope = 0; scope < 2; scope++) {
            CharactersSearch *job = characters_query_parser_create(
                scope ? CHARACTERS_SEARCH_EMOJI : CHARACTERS_SEARCH_ALL, -1, -1, queries[index]);
            assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
            assert(characters_search_count(job) == 1);
            guint32 id = characters_search_id(job, 0);
            gchar *text = characters_search_text(id);
            gchar *name = characters_search_name(id);
            assert(text && strcmp(text, expected[index]) == 0 && name);
            g_free(text);
            g_free(name);
            characters_search_free(job);
        }
    }
    CharactersSearch *job = characters_query_parser_create(CHARACTERS_SEARCH_EMOJI, 8, -1, "⌚︎");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Run the focused Stage 6 contracts without a graphical session. */
gint characters_run_contract_tests(void) {
    test_annotations();
    test_all_presentation_pairs();
    test_qualification();
    test_exact_alias_search();
    puts("4 native emoji data and annotation contract groups passed");
    return 0;
}
