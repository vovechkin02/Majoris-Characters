#include "character-search.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "language-collections.h"
#include "scalar-properties.h"
#include "unihan.h"

/** Return true if a completed vector contains an identity. */
static bool contains(const CharactersSearch *job, uint32_t point) {
    for (size_t index = 0; index < characters_search_count(job); index++) {
        if (characters_search_id(job, index) == point) {
            return true;
        }
    }
    return false;
}

/** Prepared terms and explicit repertoires remain independent of caller-owned storage. */
static void test_copied_input(void) {
    uint32_t points[] = { 0x20AC, 0x41, 0x20AC };
    CharactersSearch *job = characters_search_allocate_points(points, 3, -1);
    assert(job);
    char term[] = "euro";
    char *terms[] = { term };
    char folded[] = "euro";
    char *han[] = { folded };
    uint32_t operands[] = { CHARACTERS_HAN_ORDINARY, CHARACTERS_HAN_TEXT_FIELDS, 0 };
    assert(characters_search_prepare(job, "euro", NULL, 0, terms, 1, han, 1,
                                     operands, 3, false, false));
    points[0] = 0x42;
    memset(term, 'z', 4);
    memset(folded, 'z', 4);
    operands[0] = CHARACTERS_HAN_REJECT;
    assert(characters_search_count(job) == 0);
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0x20AC);
    assert(!characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, false, false));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Preserve exact scalars, including NUL, and independently folded Han metadata terms. */
static void test_exact_and_han(void) {
    CharactersSearch *job = characters_search_allocate(CHARACTERS_SEARCH_ALL, -1, -1);
    uint32_t point[] = { 0 };
    assert(job && characters_search_prepare(job, "U+0000", point, 1, NULL, 0, NULL, 0, NULL, 0, true, false));
    point[0] = 0x41;
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0);
    characters_search_free(job);
    job = characters_search_allocate(CHARACTERS_SEARCH_ALL, -1, -1);
    char *terms[] = { "mandarin:ZHŌNG", "radical:2" };
    char folded[] = "zhong";
    char *han[] = { folded, "2" };
    uint32_t operands[] = { CHARACTERS_HAN_FIELD, 0x401, 0, CHARACTERS_HAN_RADICAL, 0, 2 };
    assert(job && characters_search_prepare(job, "mandarin:ZHŌNG radical:2", NULL, 0,
                                           terms, 2, han, 2, operands, 6, false, false));
    memset(folded, 'z', 5);
    operands[2] = 99;
    operands[5] = 214;
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains(job, 0x4E2D) && !contains(job, 0x41));
    characters_search_free(job);
}

/** Native registry access and the copied language mask retain shared script membership. */
static void test_language_mask(void) {
    uint8_t scripts[CHARACTERS_LANGUAGE_SCRIPT_LIMIT] = { 0 };
    assert(characters_language_tag(0) == NULL);
    assert(characters_language_layout_languages("unknown-layout") == NULL);
    assert(characters_language_layout_languages("us") != NULL);
    characters_language_add_locale("ja", scripts, sizeof(scripts));
    assert(scripts[characters_scalar_script_index("Hira")]);
    assert(scripts[characters_scalar_script_index("Kana")]);
    CharactersSearch *job = characters_search_allocate(CHARACTERS_SEARCH_LANGUAGE, 0, -1);
    assert(job && characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, false, false));
    characters_search_set_scripts(job, scripts, sizeof(scripts));
    memset(scripts, 0, sizeof(scripts));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains(job, 0x3042) && contains(job, 0x30FC) && !contains(job, 0x41));
    characters_search_free(job);
}

/** Invalid input cannot publish results; cancellation works before preparation. */
static void test_invalid_and_canceled(void) {
    CharactersSearch *job = characters_search_allocate(CHARACTERS_SEARCH_ALL, -1, -1);
    uint32_t surrogate[] = { 0xD800 };
    assert(job && !characters_search_prepare(job, "U+D800", surrogate, 1, NULL, 0, NULL, 0, NULL, 0, true, false));
    assert(!characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, true, false));
    char too_long[514];
    memset(too_long, 'a', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';
    assert(!characters_search_prepare(job, too_long, NULL, 0, NULL, 0, NULL, 0, NULL, 0, false, false));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
    job = characters_search_allocate(CHARACTERS_SEARCH_ALL, -1, -1);
    assert(job);
    characters_search_cancel(job);
    assert(characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0, false, false));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_CANCELED);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Verify bounded clause validation, copying, union semantics and category intersection. */
static void test_semantic_rules(void) {
    CharactersSearch *job = characters_search_allocate(CHARACTERS_SEARCH_SYMBOLS, -1, 4);
    uint32_t rules[] = { 1u << 25, 0, 0, 0x10FFFF, 0x3FFFFFFF, 0, 0x2190, 0x2190 };
    assert(job);
    assert(!characters_search_set_rules(job, rules, 7));
    rules[3] = 0x110000;
    assert(!characters_search_set_rules(job, rules, 8));
    rules[3] = 0x10FFFF;
    assert(characters_search_set_rules(job, rules, 8));
    assert(!characters_search_set_rules(job, rules, 8));
    memset(rules, 0, sizeof(rules));
    assert(characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                     false, false));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains(job, 0x20AC) && contains(job, 0x2190) && !contains(job, 'A'));
    characters_search_free(job);

    job = characters_search_allocate(CHARACTERS_SEARCH_SYMBOLS, -1, -1);
    assert(job && characters_search_prepare(job, "", NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                                             false, false));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Reject malformed descriptors without publishing a partial query. */
static void test_han_descriptor_validation(void) {
    CharactersSearch *job = characters_search_allocate(CHARACTERS_SEARCH_ALL, -1, -1);
    char *terms[] = { "radical:215" };
    char *han[] = { "215" };
    uint32_t rules[] = { CHARACTERS_HAN_RADICAL, 0, 215 };
    assert(job && !characters_search_prepare(job, terms[0], NULL, 0, terms, 1,
                                             han, 1, rules, 3, false, false));
    rules[0] = CHARACTERS_HAN_REJECT;
    rules[2] = 0;
    assert(!characters_search_prepare(job, terms[0], NULL, 0, terms, 1,
                                      han, 1, rules, 2, false, false));
    assert(characters_search_prepare(job, terms[0], NULL, 0, terms, 1,
                                     han, 1, rules, 3, false, false));
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(characters_search_count(job) == 0);
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
    puts("6 prepared search contract groups passed");
    return 0;
}
