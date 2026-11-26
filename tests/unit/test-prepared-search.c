#include "character-search.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "emoji-catalog.h"
#include "language-collections.h"
#include "scalar-properties.h"
#include "unihan.h"

/** Supply stable fixture operands independently of the production Vala catalog. */
static void configure_fixture_metadata(CharactersSearch *job)
{
    guint32 masks[] = { 0x3E, 0x1C0, 0xE00, 0x30F80000,
                        0x0F000000, 0x7000, 0x18000, 0x20000 };
    gchar *labels[] = { "Letters", "Marks", "Numbers", "Punctuation", "Symbols",
                        "Separators", "Controls & Formatting", "Private Use" };
    gchar *fallbacks[] = { "Noncharacter", "Unassigned Character", "Private Use Character",
                           "Unnamed Character" };
    assert(job && characters_search_set_metadata(job, masks, G_N_ELEMENTS(masks),
        labels, G_N_ELEMENTS(labels), fallbacks, G_N_ELEMENTS(fallbacks)));
}

/** Allocate and prepare fixture metadata for a catalog request. */
static CharactersSearch *fixture_job(CharactersSearchScope scope, gint filter, gint category)
{
    CharactersSearch *job = characters_search_allocate(scope, filter, category);
    configure_fixture_metadata(job);
    return job;
}

/** Allocate and prepare fixture metadata for an explicit scalar repertoire. */
static CharactersSearch *fixture_points(const gunichar *points, gsize length, gint category)
{
    CharactersSearch *job = characters_search_allocate_points(points, length, category);
    configure_fixture_metadata(job);
    return job;
}

/** Return true if a completed vector contains an identity. */
static gboolean contains(const CharactersSearch *job, gunichar point) {
    for (gsize index = 0; index < characters_search_count(job); index++) {
        if (characters_search_id(job, index) == point) {
            return TRUE;
        }
    }
    return FALSE;
}

/** Prepared terms and explicit repertoires remain independent of caller-owned storage. */
static void test_copied_input(void) {
    gunichar points[] = { 0x20AC, 0x41, 0x20AC };
    CharactersSearch *job = fixture_points(points, 3, -1);
    assert(job);
    gchar term[] = "euro";
    gchar *terms[] = { term };
    gchar folded[] = "euro";
    gchar *han[] = { folded };
    guint32 operands[] = { CHARACTERS_HAN_ORDINARY, CHARACTERS_HAN_TEXT_FIELDS, 0 };
    assert(characters_search_prepare(job, "euro", NULL, 0, terms, 1, han, 1,
                                     operands, 3, FALSE, FALSE));
    points[0] = 0x42;
    memset(term, 'z', 4);
    memset(folded, 'z', 4);
    operands[0] = CHARACTERS_HAN_REJECT;
    assert(characters_search_count(job) == 0);
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0x20AC);
    assert(!characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Preserve exact scalars, including NUL, and independently folded Han metadata terms. */
static void test_exact_and_han(void) {
    CharactersSearch *job = fixture_job(CHARACTERS_SEARCH_ALL, -1, -1);
    gunichar point[] = { 0 };
    assert(job && characters_search_prepare(job, "U+0000", point, 1, NULL, 0, NULL, 0, NULL, 0, TRUE, FALSE));
    point[0] = 0x41;
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0);
    characters_search_free(job);
    job = fixture_job(CHARACTERS_SEARCH_ALL, -1, -1);
    gchar *terms[] = { "mandarin:ZHŌNG", "radical:2" };
    gchar folded[] = "zhong";
    gchar *han[] = { folded, "2" };
    guint32 operands[] = { CHARACTERS_HAN_FIELD, 0x401, 0, CHARACTERS_HAN_RADICAL, 0, 2 };
    assert(job && characters_search_prepare(job, "mandarin:ZHŌNG radical:2", NULL, 0,
                                           terms, 2, han, 2, operands, 6, FALSE, FALSE));
    memset(folded, 'z', 5);
    operands[2] = 99;
    operands[5] = 214;
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains(job, 0x4E2D) && !contains(job, 0x41));
    characters_search_free(job);
}

/** Native registry access and the copied language mask retain shared script membership. */
static void test_language_mask(void) {
    guint8 scripts[CHARACTERS_LANGUAGE_SCRIPT_LIMIT] = { 0 };
    assert(characters_language_tag(0) == NULL);
    assert(characters_language_layout_languages("unknown-layout") == NULL);
    assert(characters_language_layout_languages("us") != NULL);
    characters_language_add_locale("ja", scripts, sizeof(scripts));
    assert(scripts[characters_scalar_script_index("Hira")]);
    assert(scripts[characters_scalar_script_index("Kana")]);
    CharactersSearch *job = fixture_job(CHARACTERS_SEARCH_LANGUAGE, 0, -1);
    assert(job && characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, FALSE, FALSE));
    characters_search_set_scripts(job, scripts, sizeof(scripts));
    memset(scripts, 0, sizeof(scripts));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains(job, 0x3042) && contains(job, 0x30FC) && !contains(job, 0x41));
    characters_search_free(job);
}

/** Invalid input cannot publish results; cancellation works before preparation. */
static void test_invalid_and_canceled(void) {
    CharactersSearch *job = fixture_job(CHARACTERS_SEARCH_ALL, -1, -1);
    guint32 surrogate[] = { 0xD800 };
    assert(job && !characters_search_prepare(job, "U+D800", surrogate, 1, NULL, 0, NULL, 0, NULL, 0, TRUE, FALSE));
    assert(!characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, TRUE, FALSE));
    gchar too_long[514];
    memset(too_long, 'a', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';
    assert(!characters_search_prepare(job, too_long, NULL, 0, NULL, 0, NULL, 0, NULL, 0, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
    job = fixture_job(CHARACTERS_SEARCH_ALL, -1, -1);
    assert(job);
    characters_search_cancel(job);
    assert(characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_CANCELED);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Verify bounded clause validation, copying, union semantics and category intersection. */
static void test_semantic_rules(void) {
    CharactersSearch *job = fixture_job(CHARACTERS_SEARCH_SYMBOLS, -1, 4);
    guint32 rules[] = { 1u << 25, 0, 0, 0x10FFFF, 0x3FFFFFFF, 0, 0x2190, 0x2190 };
    assert(job);
    assert(!characters_search_set_rules(job, rules, 7));
    rules[3] = 0x110000;
    assert(!characters_search_set_rules(job, rules, 8));
    rules[3] = 0x10FFFF;
    assert(characters_search_set_rules(job, rules, 8));
    assert(!characters_search_set_rules(job, rules, 8));
    memset(rules, 0, sizeof(rules));
    assert(characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                     FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains(job, 0x20AC) && contains(job, 0x2190) && !contains(job, 'A'));
    characters_search_free(job);

    job = fixture_job(CHARACTERS_SEARCH_SYMBOLS, -1, -1);
    assert(job && characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                             FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Reject malformed descriptors without publishing a partial query. */
static void test_han_descriptor_validation(void) {
    CharactersSearch *job = fixture_job(CHARACTERS_SEARCH_ALL, -1, -1);
    gchar *terms[] = { "radical:215" };
    gchar *han[] = { "215" };
    guint32 rules[] = { CHARACTERS_HAN_RADICAL, 0, 215 };
    assert(job && !characters_search_prepare(job, terms[0], NULL, 0, terms, 1,
                                             han, 1, rules, 3, FALSE, FALSE));
    rules[0] = CHARACTERS_HAN_REJECT;
    rules[2] = 0;
    assert(!characters_search_prepare(job, terms[0], NULL, 0, terms, 1,
                                      han, 1, rules, 2, FALSE, FALSE));
    assert(characters_search_prepare(job, terms[0], NULL, 0, terms, 1,
                                     han, 1, rules, 3, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Validate copied vocabulary, category masks and missing metadata rejection. */
static void test_metadata(void)
{
    gunichar points[] = { 0x41, 0x0301, 0xE000 };
    CharactersSearch *job = characters_search_allocate_points(points, 3, -1);
    assert(job);
    guint32 masks[] = { 0x3E, 0x1C0, 0xE00, 0x30F80000,
                        0x0F000000, 0x7000, 0x18000, 0x20000 };
    gchar first[] = "letters-test";
    gchar private_use[] = "private-test";
    gchar *labels[] = { first, "Marks", "Numbers", "Punctuation", "Symbols",
                        "Separators", "Controls & Formatting", "Private Use" };
    gchar *fallbacks[] = { "Noncharacter", "Unassigned Character", private_use,
                           "Unnamed Character" };
    assert(!characters_search_set_metadata(job, masks, 7, labels, 8, fallbacks, 4));
    masks[1] = masks[0];
    assert(!characters_search_set_metadata(job, masks, 8, labels, 8, fallbacks, 4));
    masks[1] = 0x1C0;
    assert(!characters_search_set_metadata(job, masks, 8, labels, 8, fallbacks, 3));
    assert(characters_search_set_metadata(job, masks, 8, labels, 8, fallbacks, 4));
    assert(!characters_search_set_metadata(job, masks, 8, labels, 8, fallbacks, 4));
    memset(first, 'z', sizeof(first) - 1);
    memset(private_use, 'z', sizeof(private_use) - 1);
    masks[0] = 0;
    gchar *terms[] = { "letters-test" };
    gchar *han[] = { "" };
    guint32 operands[] = { CHARACTERS_HAN_ORDINARY, CHARACTERS_HAN_TEXT_FIELDS, 0 };
    assert(characters_search_prepare(job, terms[0], NULL, 0, terms, 1, han, 1,
        operands, 3, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && contains(job, 0x41));
    characters_search_free(job);

    job = characters_search_allocate_points(points, 3, 7);
    masks[0] = 0x3E;
    strcpy(first, "letters-test");
    strcpy(private_use, "private-test");
    assert(job && characters_search_set_metadata(job, masks, 8, labels, 8, fallbacks, 4));
    memset(private_use, 'z', sizeof(private_use) - 1);
    gchar *copied_terms[] = { "private-test" };
    assert(characters_search_prepare(job, copied_terms[0], NULL, 0, copied_terms, 1,
        han, 1, operands, 3, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && contains(job, 0xE000));
    characters_search_free(job);

    job = fixture_points(points, 3, 7);
    gchar *private_terms[] = { "Private Use Character" };
    assert(characters_search_prepare(job, private_terms[0], NULL, 0, private_terms, 1,
        han, 1, operands, 3, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && contains(job, 0xE000));
    characters_search_free(job);

    job = characters_search_allocate(CHARACTERS_SEARCH_ALL, -1, -1);
    assert(job && characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0,
        FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Exercise copied locale indexes, folded terms, selector keys and rejection paths. */
static void test_annotation_operands(void)
{
    guint32 locales[] = { characters_emoji_locale_index("FR"),
                          characters_emoji_locale_index("en") };
    assert(locales[0] != G_MAXUINT32 && locales[1] != G_MAXUINT32);
    assert(characters_emoji_locale_index("unknown") == G_MAXUINT32);
    gchar *folded = characters_emoji_fold_term("ÉMOTICÔNE");
    assert(folded && strcmp(folded, "émoticône") == 0);
    assert(!characters_emoji_fold_term("") && !characters_emoji_fold_term("\xFF"));
    gunichar smile[] = { 0x1F600 };
    assert(characters_emoji_keyword_matches(smile, 1, locales, 2, folded));
    assert(!characters_emoji_keyword_matches(smile, 1, locales + 1, 1, folded));
    gunichar dancer[] = { 0x1F9D1, 0x1F3FD, 0x200D, 0x1FA70 };
    assert(characters_emoji_keyword_matches(dancer, 4, locales, 2, "ballet"));
    assert(!characters_emoji_keyword_matches(dancer, 4, locales, 2, ""));
    CharactersSearch *job = fixture_job(CHARACTERS_SEARCH_EMOJI, -1, -1);
    gchar *terms[] = { "ÉMOTICÔNE" };
    gchar *han[] = { "" };
    guint32 operands[] = { CHARACTERS_HAN_ORDINARY, CHARACTERS_HAN_TEXT_FIELDS, 0 };
    guint32 invalid[] = { G_MAXUINT32 };
    guint32 duplicate[] = { locales[0], locales[0] };
    assert(!characters_search_set_annotation_locales(job, invalid, 1));
    assert(!characters_search_set_annotation_locales(job, duplicate, 2));
    assert(characters_search_set_annotation_locales(job, locales, 2));
    assert(!characters_search_set_annotation_locales(job, locales, 2));
    assert(!characters_search_set_annotation_terms(job, terms, 1));
    assert(characters_search_prepare(job, terms[0], NULL, 0, terms, 1, han, 1,
                                     operands, 3, FALSE, FALSE));
    gchar *prepared[] = { folded };
    assert(!characters_search_set_annotation_terms(job, prepared, 0));
    assert(characters_search_set_annotation_terms(job, prepared, 1));
    assert(!characters_search_set_annotation_terms(job, prepared, 1));
    memset(folded, 'z', strlen(folded));
    g_free(folded);
    locales[0] = G_MAXUINT32;
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    gboolean found = FALSE;
    for (gsize index = 0; index < characters_search_count(job); index++) {
        gchar *text = characters_search_text(characters_search_id(job, index));
        found |= text && strcmp(text, "😀") == 0;
        g_free(text);
    }
    assert(found);
    characters_search_free(job);
    job = fixture_job(CHARACTERS_SEARCH_EMOJI, -1, -1);
    assert(characters_search_prepare(job, terms[0], NULL, 0, terms, 1, han, 1,
                                     operands, 3, FALSE, FALSE));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Run the native prepared-request contracts without requiring a Vala compiler. */
int main(void) {
    test_copied_input();
    test_exact_and_han();
    test_language_mask();
    test_invalid_and_canceled();
    test_semantic_rules();
    test_han_descriptor_validation();
    test_metadata();
    test_annotation_operands();
    puts("8 prepared search contract groups passed");
    return 0;
}
