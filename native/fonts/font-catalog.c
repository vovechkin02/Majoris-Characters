#include "font-catalog.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <fontconfig/fontconfig.h>
#include <pango/pangofc-font.h>
#include <pango/pangofc-fontmap.h>

#include "../scalar-validity.h"

/** Owns the isolated Fontconfig configuration and immutable result patterns. */
struct CharactersFontCatalog {
    /** Independent configuration loaded from the installed system and user font locations. */
    FcConfig *config;
    /** Raw discovery patterns, including exact file and face identity and cmap coverage. */
    FcFontSet *faces;
};

/** Return a borrowed primary string, with no substitution for absent properties. */
static const gchar *get_string(FcPattern *pattern, const gchar *property)
{
    FcChar8 *value = NULL;
    return pattern && FcPatternGetString(pattern, property, 0, &value) == FcResultMatch ?
        (const gchar *)value : NULL;
}

/** Return the exact Fontconfig index including any named-instance bits. */
static gint get_face_index(FcPattern *pattern)
{
    gint index;
    return pattern && FcPatternGetInteger(pattern, FC_INDEX, 0, &index) == FcResultMatch ?
        index : -1;
}

/** Return a borrowed coverage set belonging to its installed pattern. */
static FcCharSet *get_charset(FcPattern *pattern)
{
    FcCharSet *charset = NULL;
    return pattern && FcPatternGetCharSet(pattern, FC_CHARSET, 0, &charset) == FcResultMatch ?
        charset : NULL;
}

/** Retrieve one pattern only within the bounds of the immutable snapshot. */
static FcPattern *get_face(const CharactersFontCatalog *catalog, gsize index)
{
    return catalog && index < (gsize)catalog->faces->nfont ? catalog->faces->fonts[index] : NULL;
}

/** Read a Fontconfig boolean property without inventing support when absent. */
static gboolean get_bool(FcPattern *pattern, const gchar *property)
{
    FcBool value = FcFalse;
    return pattern && FcPatternGetBool(pattern, property, 0, &value) == FcResultMatch && value;
}

CharactersFontCatalog *characters_fonts_load(void)
{
    CharactersFontCatalog *catalog = g_try_new0(CharactersFontCatalog, 1);
    if (!catalog) {
        return NULL;
    }
    catalog->config = FcInitLoadConfigAndFonts();
    if (!catalog->config) {
        characters_fonts_free(catalog);
        return NULL;
    }
    FcPattern *query = FcPatternCreate();
    FcObjectSet *objects = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_INDEX,
        FC_CHARSET, FC_WEIGHT, FC_WIDTH, FC_SLANT, FC_SIZE, FC_VARIABLE, FC_COLOR,
        FC_FONT_VARIATIONS, FC_SCALABLE, FC_SYMBOL, (gchar *)NULL);
    FcFontSet *listed = query && objects ? FcFontList(catalog->config, query, objects) : NULL;
    if (query) {
        FcPatternDestroy(query);
    }
    if (objects) {
        FcObjectSetDestroy(objects);
    }
    if (!listed) {
        characters_fonts_free(catalog);
        return NULL;
    }
    catalog->faces = listed;
    return catalog;
}

void characters_fonts_free(CharactersFontCatalog *catalog)
{
    if (!catalog) {
        return;
    }
    if (catalog->faces) {
        FcFontSetDestroy(catalog->faces);
    }
    if (catalog->config) {
        FcConfigDestroy(catalog->config);
    }
    g_free(catalog);
}

gsize characters_fonts_count(const CharactersFontCatalog *catalog)
{
    return catalog ? (gsize)catalog->faces->nfont : 0;
}

const gchar *characters_fonts_family(const CharactersFontCatalog *catalog, gsize index)
{
    return get_string(get_face(catalog, index), FC_FAMILY);
}

const gchar *characters_fonts_style(const CharactersFontCatalog *catalog, gsize index)
{
    return get_string(get_face(catalog, index), FC_STYLE);
}

const gchar *characters_fonts_file(const CharactersFontCatalog *catalog, gsize index)
{
    return get_string(get_face(catalog, index), FC_FILE);
}

gint characters_fonts_face_index(const CharactersFontCatalog *catalog, gsize index)
{
    return get_face_index(get_face(catalog, index));
}

gchar *characters_fonts_description(const CharactersFontCatalog *catalog, gsize index)
{
    FcPattern *face = get_face(catalog, index);
    if (!face) {
        return NULL;
    }
    PangoFontDescription *description = pango_fc_font_description_from_pattern(face, FALSE);
    if (!description) {
        return NULL;
    }
    gchar *text = pango_font_description_to_string(description);
    pango_font_description_free(description);
    if (text && !g_utf8_validate(text, -1, NULL)) {
        g_free(text);
        return NULL;
    }
    return text;
}

gboolean characters_fonts_is_variable(const CharactersFontCatalog *catalog, gsize index)
{
    return get_bool(get_face(catalog, index), FC_VARIABLE);
}

gboolean characters_fonts_is_color(const CharactersFontCatalog *catalog, gsize index)
{
    return get_bool(get_face(catalog, index), FC_COLOR);
}

gboolean characters_fonts_contains(const CharactersFontCatalog *catalog, gsize index, gunichar point)
{
    FcCharSet *charset = get_charset(get_face(catalog, index));
    if (!charset || !characters_scalar_is_valid(point) || !FcCharSetHasChar(charset, point)) {
        return FALSE;
    }
    if (get_bool(get_face(catalog, index), FC_SYMBOL)) {
        CharactersFontFace *face = characters_fonts_open_face(catalog, index);
        gboolean mapped = characters_font_face_glyph(face, point, FALSE) != 0;
        characters_font_face_free(face);
        return mapped;
    }
    return TRUE;
}

gunichar *characters_fonts_points(const CharactersFontCatalog *catalog, gsize index,
                                 gsize *length)
{
    if (!length) {
        return NULL;
    }
    *length = 0;
    CharactersFontFace *face = characters_fonts_open_face(catalog, index);
    gunichar *points = characters_font_face_unicode_points(face, length);
    characters_font_face_free(face);
    return points;
}

gboolean characters_fonts_is_current(const CharactersFontCatalog *catalog)
{
    return catalog && FcConfigUptoDate(catalog->config);
}

gboolean characters_fonts_apply_config(const CharactersFontCatalog *catalog, PangoFontMap *map)
{
    if (!catalog || !map || !PANGO_IS_FC_FONT_MAP(map)) {
        return FALSE;
    }
    pango_fc_font_map_set_config(PANGO_FC_FONT_MAP(map), catalog->config);
    return TRUE;
}

gboolean characters_fonts_matches(const CharactersFontCatalog *catalog, gsize index, PangoFont *font)
{
    FcPattern *expected = get_face(catalog, index);
    if (!expected || !font || !PANGO_IS_FC_FONT(font)) {
        return FALSE;
    }
    FcPattern *actual = pango_fc_font_get_pattern(PANGO_FC_FONT(font));
    const gchar *file = get_string(actual, FC_FILE);
    const gchar *expected_file = get_string(expected, FC_FILE);
    gint expected_index = get_face_index(expected);
    return file && expected_file && expected_index >= 0 &&
        strcmp(file, expected_file) == 0 && get_face_index(actual) == expected_index;
}

CharactersFontFace *characters_fonts_open_face(const CharactersFontCatalog *catalog, gsize index)
{
    return characters_font_face_open(characters_fonts_file(catalog, index),
                                      characters_fonts_face_index(catalog, index));
}
