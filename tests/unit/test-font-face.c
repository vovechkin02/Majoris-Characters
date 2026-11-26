#include "font-face.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

/** Count nontransparent pixels and verify the exact tightly packed image contract. */
static gsize ink(GBytes *bytes) {
    assert(bytes);
    gsize size;
    const guint8 *pixels = g_bytes_get_data(bytes, &size);
    assert(size == 96u * 96u * 4u);
    gsize count = 0;
    for (gsize index = 3; index < size; index += 4) {
        count += pixels[index] != 0;
    }
    return count;
}

/** Two faces in the same file must produce visibly different exact glyphs. */
static void test_collection(const gchar *path) {
    CharactersFontFace *regular = characters_font_face_open(path, 0);
    CharactersFontFace *bold = characters_font_face_open(path, 1);
    assert(regular && bold);
    assert(!characters_font_face_open(path, 2));
    assert(characters_font_face_axis_count(regular) == 0);
    gsize length;
    gunichar *points = characters_font_face_unicode_points(regular, &length);
    assert(points && length == 2 && points[0] == 0x20 && points[1] == 0x41);
    g_free(points);
    GBytes *thin = characters_font_face_render(regular, 'A', FALSE, 48, 10, 20, 30);
    GBytes *thick = characters_font_face_render(bold, 'A', FALSE, 48, 10, 20, 30);
    assert(ink(thin) > 0 && ink(thick) > ink(thin));
    GBytes *space = characters_font_face_render(regular, ' ', FALSE, 48, 10, 20, 30);
    assert(ink(space) == 0);
    assert(!characters_font_face_render(regular, 'Z', FALSE, 48, 0, 0, 0));
    assert(!characters_font_face_render(regular, 'A', FALSE, 0, 0, 0, 0));
    assert(!characters_font_face_render(regular, 'A', FALSE, 193, 0, 0, 0));
    g_bytes_unref(space);
    g_bytes_unref(thick);
    g_bytes_unref(thin);
    characters_font_face_free(bold);
    characters_font_face_free(regular);
}

/** Axes must adjust real pixels and restore the original named-instance coordinates. */
static void test_variable(const gchar *path) {
    CharactersFontFace *face = characters_font_face_open(path, 0);
    assert(face && characters_font_face_axis_count(face) == 1);
    gchar *tag = characters_font_face_axis_tag(face, 0);
    assert(tag && strcmp(tag, "wght") == 0);
    g_free(tag);
    gdouble minimum, maximum, default_value, current;
    assert(characters_font_face_axis_range(face, 0, &minimum, &maximum, &default_value, &current));
    assert(minimum == 100 && maximum == 900 && default_value == 100 && current == 100);
    GBytes *thin = characters_font_face_render(face, 'A', FALSE, 48, 0, 0, 0);
    assert(characters_font_face_set_axis(face, 0, 900));
    GBytes *thick = characters_font_face_render(face, 'A', FALSE, 48, 0, 0, 0);
    assert(ink(thick) > ink(thin));
    assert(!characters_font_face_set_axis(face, 0, NAN));
    assert(!characters_font_face_set_axis(face, 0, 901));
    assert(!characters_font_face_set_axis(face, 1, 100));
    assert(characters_font_face_reset(face));
    GBytes *reset = characters_font_face_render(face, 'A', FALSE, 48, 0, 0, 0);
    assert(g_bytes_equal(thin, reset));
    g_bytes_unref(reset);
    g_bytes_unref(thick);
    g_bytes_unref(thin);
    characters_font_face_free(face);
    face = characters_font_face_open(path, 0x10000);
    assert(face && characters_font_face_axis_range(face, 0, &minimum, &maximum,
                                                  &default_value, &current));
    assert(current == 900);
    assert(characters_font_face_set_axis(face, 0, 100));
    assert(characters_font_face_reset(face));
    assert(characters_font_face_axis_range(face, 0, &minimum, &maximum, &default_value, &current));
    assert(current == 900);
    characters_font_face_free(face);
}

/** Raw Microsoft Symbol mappings must not become fabricated Unicode identities. */
static void test_symbol(const gchar *path) {
    CharactersFontFace *face = characters_font_face_open(path, 0);
    assert(face && characters_font_face_symbol_count(face) == 2);
    assert(characters_font_face_symbol_code(face, 0) == 0xF041);
    assert(characters_font_face_symbol_code(face, 1) == 0xF042);
    assert(characters_font_face_symbol_code(face, 2) == G_MAXUINT32);
    gsize length;
    gunichar *points = characters_font_face_unicode_points(face, &length);
    assert(points && length == 0);
    g_free(points);
    assert(!characters_font_face_glyph(face, 0xF041, FALSE));
    assert(!characters_font_face_glyph(face, 0x41, TRUE));
    assert(characters_font_face_glyph(face, 0xF041, TRUE) == 1);
    GBytes *glyph = characters_font_face_render(face, 0xF041, TRUE, 48, 0, 0, 0);
    assert(ink(glyph) > 0);
    g_bytes_unref(glyph);
    characters_font_face_free(face);
}

/** Validate failure boundaries without requiring any system-installed font. */
static void test_invalid(void) {
    assert(!characters_font_face_open(NULL, 0));
    assert(!characters_font_face_open("/nonexistent/font.ttf", 0));
    assert(characters_font_face_axis_count(NULL) == 0);
    assert(!characters_font_face_axis_tag(NULL, 0));
    assert(!characters_font_face_reset(NULL));
    assert(!characters_font_face_glyph(NULL, 'A', FALSE));
    assert(!characters_font_face_render(NULL, 'A', FALSE, 48, 0, 0, 0));
    characters_font_face_free(NULL);
}

/** Execute deterministic exact-face, variable-font and symbol-map contract tests. */
int main(int argc, char **argv) {
    assert(argc == 4);
    test_collection(argv[1]);
    test_variable(argv[2]);
    test_symbol(argv[3]);
    test_invalid();
    puts("4 exact font face contract groups passed");
    return 0;
}
