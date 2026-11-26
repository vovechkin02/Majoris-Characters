#include "search-contract-tests.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unicode-engine.h"

/** Start a valid request and require successful publication. Caller owns it. */
static CharactersSearch *search(CharactersSearchScope scope, int filter, int category,
                                const char *query) {
    CharactersSearch *job = characters_query_parser_create(scope, filter, category, query);
    assert(job);
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    return job;
}

/** Find the collection slot by its immutable native label. */
static int find_filter(CharactersSearchScope scope, const char *name) {
    for (size_t index = 0; index < characters_search_filter_count(scope); index++) {
        const char *label = characters_search_filter_name(scope, index);
        if (label && strcmp(label, name) == 0) {
            return (int)index;
        }
    }
    assert(!"Expected filter is absent");
    return -1;
}

/** Determine whether the completed result contains the specified identity. */
static bool contains_id(CharactersSearch *job, uint32_t id) {
    for (size_t index = 0; index < characters_search_count(job); index++) {
        if (characters_search_id(job, index) == id) {
            return true;
        }
    }
    return false;
}

/** Assert a result contains the exact UTF-8 sequence rather than a presentation substitute. */
static bool contains_text(CharactersSearch *job, const char *expected) {
    for (size_t index = 0; index < characters_search_count(job); index++) {
        char *text = characters_search_text(characters_search_id(job, index));
        bool match = text && strcmp(text, expected) == 0;
        free(text);
        if (match) {
            return true;
        }
    }
    return false;
}

/** All assigned scalars occur in ascending order exactly once, followed by catalog sequences. */
static void test_complete_repertoire(void) {
    CharactersSearch *job = search(CHARACTERS_SEARCH_ALL, -1, -1, "");
    size_t index = 0;
    for (uint32_t point = 0; point <= 0x10FFFF; point++) {
        if (characters_unicode_is_assigned(point)) {
            assert(characters_search_id(job, index++) == point);
        }
    }
    size_t scalar_count = index;
    assert(scalar_count == 297334);
    assert(characters_search_count(job) > scalar_count);
    for (; index < characters_search_count(job); index++) {
        assert(characters_search_id(job, index) > 0x10FFFF);
    }
    assert(characters_search_id(job, index) == UINT32_MAX);
    printf("Repertoire: %zu assigned scalars; %zu total identities\n",
           scalar_count, characters_search_count(job));
    characters_search_free(job);
}

/** Blocks, scripts, semantic categories and general categories intersect correctly. */
static void test_filters(void) {
    int latin = find_filter(CHARACTERS_SEARCH_BLOCK, "Basic Latin");
    CharactersSearch *job = search(CHARACTERS_SEARCH_BLOCK, latin, -1, "");
    assert(characters_search_count(job) == 128);
    assert(characters_search_id(job, 0) == 0);
    assert(characters_search_id(job, 127) == 127);
    characters_search_free(job);

    int greek = find_filter(CHARACTERS_SEARCH_SCRIPT, "Greek");
    job = search(CHARACTERS_SEARCH_SCRIPT, greek, 0, "");
    assert(contains_id(job, 0x03B1));
    assert(!contains_id(job, 'A'));
    assert(!contains_id(job, 0x037E));
    characters_search_free(job);

    int beria = find_filter(CHARACTERS_SEARCH_SCRIPT, "Beria_Erfe");
    job = search(CHARACTERS_SEARCH_SCRIPT, beria, 0, "");
    assert(contains_id(job, 0x16EA0));
    assert(contains_id(job, 0x16ED3));
    assert(!contains_id(job, 'A'));
    characters_search_free(job);

    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "U+323B0");
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0x323B0);
    characters_search_free(job);

    int currencies = find_filter(CHARACTERS_SEARCH_SYMBOLS, "Currencies");
    job = search(CHARACTERS_SEARCH_SYMBOLS, currencies, 4, "euro");
    assert(contains_id(job, 0x20AC));
    assert(!contains_id(job, 'E'));
    characters_search_free(job);

    job = search(CHARACTERS_SEARCH_BLOCK, latin, 2, "");
    assert(characters_search_count(job) == 10);
    assert(characters_search_id(job, 0) == '0');
    characters_search_free(job);

    job = search(CHARACTERS_SEARCH_BLOCK, latin, -1, "U+20AC");
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Numeric syntax, aliases, literal characters, AND terms and empty matches are deterministic. */
static void test_query_forms(void) {
    const char *queries[] = { "U+20AC", "0x20ac", "20AC", "dec:8364", "€" };
    for (size_t index = 0; index < sizeof(queries) / sizeof(queries[0]); index++) {
        CharactersSearch *job = search(CHARACTERS_SEARCH_ALL, -1, -1, queries[index]);
        assert(characters_search_count(job) == 1);
        assert(characters_search_id(job, 0) == 0x20AC);
        characters_search_free(job);
    }
    CharactersSearch *job = search(CHARACTERS_SEARCH_ALL, -1, -1, "41");
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 'A');
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "small greek letter alpha");
    assert(contains_id(job, 0x03B1));
    assert(!contains_id(job, 0x0391));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "line feed");
    assert(contains_id(job, 10));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "face");
    assert(contains_id(job, 0xFACE));
    assert(contains_text(job, "😀"));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "zzzz-no-such-character-zzzz");
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Emoji groups retain exact sequences; named sequences are individually searchable. */
static void test_sequences(void) {
    for (size_t group = 0; group < characters_unicode_emoji_group_count(); group++) {
        CharactersSearch *job = search(CHARACTERS_SEARCH_EMOJI, (int)group, -1, "");
        assert(characters_search_count(job) == characters_unicode_emoji_group_size(group));
        for (size_t index = 0; index < characters_search_count(job); index++) {
            uint32_t id = characters_search_id(job, index);
            char *text = characters_search_text(id);
            char *expected = characters_unicode_emoji_text(
                characters_unicode_emoji_group_index(group, index));
            assert(text && expected && strcmp(text, expected) == 0);
            free(text);
            free(expected);
        }
        characters_search_free(job);
    }
    const char *queries[] = { "👩‍💻", "U+1F469 U+200D U+1F4BB", "woman technologist" };
    for (size_t index = 0; index < sizeof(queries) / sizeof(queries[0]); index++) {
        CharactersSearch *job = search(CHARACTERS_SEARCH_EMOJI, -1, -1, queries[index]);
        assert(contains_text(job, "👩‍💻"));
        characters_search_free(job);
    }
    CharactersSearch *job = search(CHARACTERS_SEARCH_ALL, -1, -1, "U+0031 U+FE0F U+20E3");
    assert(characters_search_count(job) == 1);
    assert(contains_text(job, "1️⃣"));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_NAMED, -1, -1, "");
    assert(characters_search_count(job) == characters_unicode_named_count());
    uint32_t id = characters_search_id(job, 0);
    char *name = characters_search_name(id);
    char *text = characters_search_text(id);
    assert(name && text);
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_NAMED, -1, -1, name);
    assert(contains_id(job, id));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_NAMED, -1, -1, text);
    assert(contains_id(job, id));
    characters_search_free(job);
    free(name);
    free(text);
}

/** Render invisible scalars and combining marks without replacing their underlying identities. */
static void test_display(void) {
    char *display = characters_search_display(0);
    char *code = characters_search_code(0);
    assert(display && *display && code && strcmp(code, "U+0000") == 0);
    assert(characters_search_text(0) == NULL);
    free(display);
    free(code);
    display = characters_search_display(0x0301);
    char *text = characters_search_text(0x0301);
    assert(display && text && strcmp(display, "◌́") == 0 && strcmp(text, "́") == 0);
    free(display);
    free(text);
    assert(characters_search_name(0xD800) == NULL);
    assert(characters_search_code(UINT32_MAX) == NULL);
    assert(characters_search_text(UINT32_MAX) == NULL);
    CharactersSearch *job = search(CHARACTERS_SEARCH_EMOJI, -1, -1, "👩‍💻");
    code = characters_search_code(characters_search_id(job, 0));
    assert(code && strcmp(code, "U+1F469 U+200D U+1F4BB") == 0);
    free(code);
    characters_search_free(job);
}

/** Malformed explicit values, invalid UTF-8, oversize input and invalid filters fail closed. */
static void test_invalid(void) {
    const char *queries[] = { "U+D800", "U+110000", "U+", "dec:-1", "0xFFZZ", "\xFF" };
    for (size_t index = 0; index < sizeof(queries) / sizeof(queries[0]); index++) {
        CharactersSearch *job = characters_query_parser_create(CHARACTERS_SEARCH_ALL, -1, -1,
                                                     queries[index]);
        assert(job && characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
        assert(characters_search_count(job) == 0);
        characters_search_free(job);
    }
    char oversized[514];
    memset(oversized, 'a', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    CharactersSearch *job = characters_query_parser_create(CHARACTERS_SEARCH_ALL, -1, -1, oversized);
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
    job = characters_query_parser_create(CHARACTERS_SEARCH_BLOCK, 99999, -1, "");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Cancel from a second thread; it accesses only the request's atomic flag. */
static void *cancel_worker(void *argument) {
    characters_search_cancel(argument);
    return NULL;
}

/** Pre-canceled requests publish nothing; concurrent cancellation permits only complete results. */
static void test_cancellation(void) {
    CharactersSearch *job = characters_query_parser_create(CHARACTERS_SEARCH_ALL, -1, -1, "letter");
    assert(job);
    characters_search_cancel(job);
    assert(characters_search_run(job) == CHARACTERS_SEARCH_CANCELED);
    assert(characters_search_count(job) == 0);
    characters_search_free(job);

    job = characters_query_parser_create(CHARACTERS_SEARCH_ALL, -1, -1, "letter");
    assert(job);
    pthread_t thread;
    assert(pthread_create(&thread, NULL, cancel_worker, job) == 0);
    CharactersSearchStatus status = characters_search_run(job);
    assert(pthread_join(thread, NULL) == 0);
    assert(status == CHARACTERS_SEARCH_CANCELED || status == CHARACTERS_SEARCH_OK);
    if (status == CHARACTERS_SEARCH_CANCELED) {
        assert(characters_search_count(job) == 0);
    } else {
        assert(contains_id(job, 'A'));
    }
    assert(characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Shared Japanese marks appear in both Hiragana and Katakana script collections. */
static void test_script_extensions(void) {
    int hiragana = find_filter(CHARACTERS_SEARCH_SCRIPT, "Hiragana");
    int katakana = find_filter(CHARACTERS_SEARCH_SCRIPT, "Katakana");
    CharactersSearch *job = search(CHARACTERS_SEARCH_SCRIPT, hiragana, -1, "U+30FC");
    assert(contains_id(job, 0x30FC));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_SCRIPT, katakana, -1, "U+30FC");
    assert(contains_id(job, 0x30FC));
    characters_search_free(job);
}

/** Language collections combine associated scripts and configured keyboard layouts. */
static void test_language_collections(void) {
    int japanese = find_filter(CHARACTERS_SEARCH_LANGUAGE, "Japanese");
    CharactersSearch *job = search(CHARACTERS_SEARCH_LANGUAGE, japanese, -1, "");
    assert(contains_id(job, 0x3042) && contains_id(job, 0x30A2));
    assert(contains_id(job, 0x4E2D) && !contains_id(job, 0x0410));
    characters_search_free(job);

    job = characters_query_parser_create(CHARACTERS_SEARCH_LANGUAGE, 0, -1, "");
    assert(job);
    characters_language_selection_configure(job, 0, "ko_KR.UTF-8", "");
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains_id(job, 0xAC00) && contains_id(job, 0x4E2D));
    characters_search_free(job);

    job = characters_query_parser_create(CHARACTERS_SEARCH_LANGUAGE, 1, -1, "");
    assert(job);
    characters_language_selection_configure(job, 1, "en", "us,ru,unknown-layout");
    assert(characters_search_run(job) == CHARACTERS_SEARCH_OK);
    assert(contains_id(job, 'A') && contains_id(job, 0x0410));
    assert(!contains_id(job, 0x3042));
    characters_search_free(job);

    job = search(CHARACTERS_SEARCH_LANGUAGE, 1, -1, "");
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
}

/** Reading searches, collation groups, combined filters and missing data stay distinct. */
static void test_han_search(void) {
    CharactersSearch *job = search(CHARACTERS_SEARCH_ALL, -1, -1, "mandarin:ZHŌNG");
    assert(contains_id(job, 0x4E2D) && !contains_id(job, 'A'));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "mandarin:hang");
    assert(contains_id(job, 0x884C));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "korean:중");
    assert(contains_id(job, 0x4E2D));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "radical:2 strokes:4 mandarin:zhong");
    assert(contains_id(job, 0x4E2D));
    characters_search_free(job);
    int radical = find_filter(CHARACTERS_SEARCH_RADICAL, "085 水");
    job = search(CHARACTERS_SEARCH_RADICAL, radical, -1, "");
    assert(contains_id(job, 0x6C34) && contains_id(job, 0x6F22));
    assert(!contains_id(job, 'A'));
    characters_search_free(job);
    int strokes = find_filter(CHARACTERS_SEARCH_STROKES, "4");
    job = search(CHARACTERS_SEARCH_STROKES, strokes, -1, "U+4E2D");
    assert(contains_id(job, 0x4E2D));
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_ALL, -1, -1, "radical:215");
    assert(characters_search_count(job) == 0);
    characters_search_free(job);

    char *value = characters_unicode_property(0x4E2D, "Mandarin_Fallback");
    assert(value && strstr(value, "zhōng"));
    free(value);
    value = characters_unicode_property(0x4E2D, "Hangul_Fallback");
    assert(value && strcmp(value, "중") == 0);
    free(value);
    assert(characters_unicode_property('A', "Mandarin_Fallback") == NULL);
}

/** Plane filters partition the assigned and reserved scalar spaces without surrogates. */
static void test_planes_and_unassigned(void) {
    assert(characters_search_filter_count(CHARACTERS_SEARCH_PLANE) == 17);
    assert(characters_search_filter_count(CHARACTERS_SEARCH_UNASSIGNED) == 17);
    size_t assigned_count = 0;
    size_t reserved_count = 0;
    for (int plane = 0; plane <= 16; plane++) {
        CharactersSearch *assigned = search(CHARACTERS_SEARCH_PLANE, plane, -1, "");
        CharactersSearch *reserved = search(CHARACTERS_SEARCH_UNASSIGNED, plane, -1, "");
        size_t assigned_index = 0;
        size_t reserved_index = 0;
        for (uint32_t point = (uint32_t)plane << 16;
             point < (((uint32_t)plane + 1) << 16); point++) {
            if (!characters_unicode_is_scalar(point)) {
                continue;
            }
            if (characters_unicode_is_assigned(point)) {
                assert(characters_search_id(assigned, assigned_index++) == point);
            } else {
                assert(characters_search_id(reserved, reserved_index++) == point);
            }
        }
        assert(assigned_index == characters_search_count(assigned));
        assert(reserved_index == characters_search_count(reserved));
        assigned_count += assigned_index;
        reserved_count += reserved_index;
        characters_search_free(assigned);
        characters_search_free(reserved);
    }
    assert(assigned_count == 297334 && assigned_count + reserved_count == 0x110000 - 0x800);
    CharactersSearch *job = search(CHARACTERS_SEARCH_UNASSIGNED, 0, -1, "U+0378");
    assert(characters_search_count(job) == 1 && characters_search_id(job, 0) == 0x0378);
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_UNASSIGNED, 1, -1, "U+0378");
    assert(characters_search_count(job) == 0);
    characters_search_free(job);
    job = search(CHARACTERS_SEARCH_UNASSIGNED, 16, -1, "U+10FFFF");
    assert(characters_search_count(job) == 1);
    characters_search_free(job);
    char *name = characters_search_name(0x10FFFF);
    assert(name && strcmp(name, "Noncharacter") == 0);
    free(name);
    char *display = characters_search_display(0x0378);
    assert(display && strcmp(display, "·") == 0);
    free(display);
    job = characters_query_parser_create(CHARACTERS_SEARCH_PLANE, 17, -1, "");
    assert(job && characters_search_run(job) == CHARACTERS_SEARCH_INVALID);
    characters_search_free(job);
}

/** Expanded semantic collections match Unicode properties and curated range entries. */
static void test_semantic_collections(void) {
    const struct {
        /** Semantic collection label. */
        const char *label;
        /** Representative matching scalar. */
        uint32_t point;
    } cases[] = {
        { "Superscripts", 0xA7F1 }, { "Subscripts", 0x2082 },
        { "Fractions", 0x00BD }, { "Whitespace", 0x0009 },
        { "Quotation Marks", 0x201C }, { "Accessibility Symbols", 0x267F },
        { "Letterlike Symbols", 0x2122 }, { "Religious Symbols", 0x262F },
        { "Variation Selectors", 0xE0100 }, { "Block Elements", 0x2588 },
        { "Technical", 0x26A1 }
    };
    assert(characters_search_filter_count(CHARACTERS_SEARCH_SYMBOLS) == 27);
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
        int filter = find_filter(CHARACTERS_SEARCH_SYMBOLS, cases[index].label);
        CharactersSearch *job = search(CHARACTERS_SEARCH_SYMBOLS, filter, -1, "");
        assert(contains_id(job, cases[index].point));
        assert(!contains_id(job, 'A'));
        characters_search_free(job);
    }
}

/** Execute focused native search contracts, with assertions enabled by the test target. */
int characters_run_contract_tests(void) {
    test_complete_repertoire();
    test_filters();
    test_query_forms();
    test_sequences();
    test_display();
    test_invalid();
    test_cancellation();
    test_script_extensions();
    test_language_collections();
    test_han_search();
    test_planes_and_unassigned();
    test_semantic_collections();
    puts("12 character search contract groups passed");
    return 0;
}
