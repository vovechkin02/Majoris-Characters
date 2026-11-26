#include "font-catalog.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <pango/pangoft2.h>

/** Validate raw installed face access and exact mapped-scalar enumeration. */
static size_t test_coverage(CharactersFontCatalog *catalog) {
    size_t matching = SIZE_MAX;
    size_t count = characters_fonts_count(catalog);
    assert(count > 0);
    for (size_t index = 0; index < count; index++) {
        const char *family = characters_fonts_family(catalog, index);
        if (!family || !g_utf8_validate(family, -1, NULL) ||
            !characters_fonts_file(catalog, index) ||
            characters_fonts_face_index(catalog, index) < 0) {
            continue;
        }
        if (matching == SIZE_MAX && strcmp(family, "DejaVu Sans") == 0 &&
            characters_fonts_contains(catalog, index, 'A')) {
            matching = index;
        }
    }
    /* The Debian build test dependency supplies DejaVu Sans. */
    assert(matching != SIZE_MAX);
    size_t length = 0;
    uint32_t *points = characters_fonts_points(catalog, matching, &length);
    assert(points && length > 0);
    bool contains_a = false;
    for (size_t index = 0; index < length; index++) {
        assert(points[index] <= 0x10FFFF);
        assert(points[index] < 0xD800 || points[index] > 0xDFFF);
        assert(characters_fonts_contains(catalog, matching, points[index]));
        if (index) {
            assert(points[index] > points[index - 1]);
        }
        if (points[index] == 'A') {
            contains_a = true;
        }
    }
    assert(contains_a);
    assert(!characters_fonts_contains(catalog, matching, 0xD800));
    assert(!characters_fonts_contains(catalog, matching, UINT32_MAX));
    printf("%zu installed faces; selected DejaVu face has %zu mapped scalars\n", count, length);
    g_free(points);
    return matching;
}

/** Verify the Pango font map accepts a snapshot and rejects unavailable identities. */
static void test_renderer(CharactersFontCatalog *catalog, size_t face) {
    PangoFontMap *map = pango_ft2_font_map_new();
    assert(map && characters_fonts_apply_config(catalog, map));
    PangoContext *context = pango_font_map_create_context(map);
    char *description = characters_fonts_description(catalog, face);
    assert(description && *description);
    PangoFontDescription *requested = pango_font_description_from_string(description);
    pango_font_description_set_size(requested, 24 * PANGO_SCALE);
    PangoFont *font = pango_context_load_font(context, requested);
    assert(font);
    /* The nominal renderer may substitute an equivalent style; do not claim exact file identity. */
    printf("Nominal face matches installed file: %s\n",
           characters_fonts_matches(catalog, face, font) ? "yes" : "substituted");
    assert(!characters_fonts_matches(catalog, SIZE_MAX, font));
    assert(!characters_fonts_matches(catalog, face, NULL));
    pango_font_description_free(requested);
    g_free(description);
    g_object_unref(font);
    g_object_unref(context);
    g_object_unref(map);
}

/** Checks invalid indexes and null input independently of the available font repertoire. */
static void test_invalid(CharactersFontCatalog *catalog) {
    size_t length = 99;
    assert(!characters_fonts_family(catalog, SIZE_MAX));
    assert(!characters_fonts_description(catalog, SIZE_MAX));
    assert(characters_fonts_face_index(catalog, SIZE_MAX) == -1);
    assert(!characters_fonts_points(catalog, SIZE_MAX, &length) && length == 0);
    assert(!characters_fonts_points(catalog, 0, NULL));
    assert(!characters_fonts_contains(catalog, SIZE_MAX, 'A'));
    assert(!characters_fonts_is_variable(catalog, SIZE_MAX));
    assert(!characters_fonts_is_color(catalog, SIZE_MAX));
    assert(characters_fonts_count(NULL) == 0);
    assert(!characters_fonts_is_current(NULL));
    assert(!characters_fonts_apply_config(catalog, NULL));
    characters_fonts_free(NULL);
}

/** Exercises the installed inventory through its actual Fontconfig and PangoFc dependencies. */
int main(void) {
    CharactersFontCatalog *catalog = characters_fonts_load();
    assert(catalog);
    size_t face = test_coverage(catalog);
    test_renderer(catalog, face);
    test_invalid(catalog);
    characters_fonts_free(catalog);
    puts("3 font catalog contract groups passed");
    return 0;
}
