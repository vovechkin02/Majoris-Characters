#include "font-catalog.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <pango/pangoft2.h>

#include <glib.h>

/** Validate raw installed face access and exact mapped-scalar enumeration. */
static gsize test_coverage(CharactersFontCatalog *catalog) {
    gsize matching = G_MAXSIZE;
    gsize count = characters_fonts_count(catalog);
    assert(count > 0);
    for (gsize index = 0; index < count; index++) {
        const gchar *family = characters_fonts_family(catalog, index);
        if (!family || !g_utf8_validate(family, -1, NULL) ||
            !characters_fonts_file(catalog, index) ||
            characters_fonts_face_index(catalog, index) < 0) {
            continue;
        }
        if (matching == G_MAXSIZE && strcmp(family, "DejaVu Sans") == 0 &&
            characters_fonts_contains(catalog, index, 'A')) {
            matching = index;
        }
    }
    /* The Debian build test dependency supplies DejaVu Sans. */
    assert(matching != G_MAXSIZE);
    gsize length = 0;
    gunichar *points = characters_fonts_points(catalog, matching, &length);
    assert(points && length > 0);
    gboolean contains_a = FALSE;
    for (gsize index = 0; index < length; index++) {
        assert(points[index] <= 0x10FFFF);
        assert(points[index] < 0xD800 || points[index] > 0xDFFF);
        assert(characters_fonts_contains(catalog, matching, points[index]));
        if (index) {
            assert(points[index] > points[index - 1]);
        }
        if (points[index] == 'A') {
            contains_a = TRUE;
        }
    }
    assert(contains_a);
    assert(!characters_fonts_contains(catalog, matching, 0xD800));
    assert(!characters_fonts_contains(catalog, matching, G_MAXUINT32));
    printf("%zu installed faces; selected DejaVu face has %zu mapped scalars\n", count, length);
    g_free(points);
    return matching;
}

/** Verify the Pango font map accepts a snapshot and rejects unavailable identities. */
static void test_renderer(CharactersFontCatalog *catalog, gsize face) {
    PangoFontMap *map = pango_ft2_font_map_new();
    assert(map && characters_fonts_apply_config(catalog, map));
    PangoContext *context = pango_font_map_create_context(map);
    gchar *description = characters_fonts_description(catalog, face);
    assert(description && *description);
    PangoFontDescription *requested = pango_font_description_from_string(description);
    pango_font_description_set_size(requested, 24 * PANGO_SCALE);
    PangoFont *font = pango_context_load_font(context, requested);
    assert(font);
    /* The nominal renderer may substitute an equivalent style; do not claim exact file identity. */
    printf("Nominal face matches installed file: %s\n",
           characters_fonts_matches(catalog, face, font) ? "yes" : "substituted");
    assert(!characters_fonts_matches(catalog, G_MAXSIZE, font));
    assert(!characters_fonts_matches(catalog, face, NULL));
    pango_font_description_free(requested);
    g_free(description);
    g_object_unref(font);
    g_object_unref(context);
    g_object_unref(map);
}

/** Checks invalid indexes and null input independently of the available font repertoire. */
static void test_invalid(CharactersFontCatalog *catalog) {
    gsize length = 99;
    assert(!characters_fonts_family(catalog, G_MAXSIZE));
    assert(!characters_fonts_description(catalog, G_MAXSIZE));
    assert(characters_fonts_face_index(catalog, G_MAXSIZE) == -1);
    assert(!characters_fonts_points(catalog, G_MAXSIZE, &length) && length == 0);
    assert(!characters_fonts_points(catalog, 0, NULL));
    assert(!characters_fonts_contains(catalog, G_MAXSIZE, 'A'));
    assert(!characters_fonts_is_variable(catalog, G_MAXSIZE));
    assert(!characters_fonts_is_color(catalog, G_MAXSIZE));
    assert(characters_fonts_count(NULL) == 0);
    assert(!characters_fonts_is_current(NULL));
    assert(!characters_fonts_apply_config(catalog, NULL));
    characters_fonts_free(NULL);
}

/** Exercises the installed inventory through its actual Fontconfig and PangoFc dependencies. */
int main(void) {
    CharactersFontCatalog *catalog = characters_fonts_load();
    assert(catalog);
    gsize face = test_coverage(catalog);
    test_renderer(catalog, face);
    test_invalid(catalog);
    characters_fonts_free(catalog);
    puts("3 font catalog contract groups passed");
    return 0;
}
