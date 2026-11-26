#include "font-catalog.h"

#include <stdlib.h>
#include <string.h>

#include <fontconfig/fontconfig.h>
#include <pango/pangofc-font.h>
#include <pango/pangofc-fontmap.h>

/** Owns the isolated Fontconfig configuration and immutable result patterns. */
struct CharactersFontCatalog {
    /** Independent configuration loaded from the installed system and user font locations. */
    FcConfig *config;
    /** Raw discovery patterns, including exact file and face identity and cmap coverage. */
    FcFontSet *faces;
};

/** Return a borrowed primary string, with no substitution for absent properties. */
static const char *get_string(FcPattern *pattern, const char *property) {
    FcChar8 *value = NULL;
    return pattern && FcPatternGetString(pattern, property, 0, &value) == FcResultMatch ?
        (const char *)value : NULL;
}

/** Return the exact Fontconfig index including any named-instance bits. */
static int get_face_index(FcPattern *pattern) {
    int index;
    return pattern && FcPatternGetInteger(pattern, FC_INDEX, 0, &index) == FcResultMatch ?
        index : -1;
}

/** Return a borrowed coverage set belonging to its installed pattern. */
static FcCharSet *get_charset(FcPattern *pattern) {
    FcCharSet *charset = NULL;
    return pattern && FcPatternGetCharSet(pattern, FC_CHARSET, 0, &charset) == FcResultMatch ?
        charset : NULL;
}

/** Retrieve one pattern only within the bounds of the immutable snapshot. */
static FcPattern *get_face(const CharactersFontCatalog *catalog, size_t index) {
    return catalog && index < (size_t)catalog->faces->nfont ? catalog->faces->fonts[index] : NULL;
}

/** Read a Fontconfig boolean property without inventing support when absent. */
static bool get_bool(FcPattern *pattern, const char *property) {
    FcBool value = FcFalse;
    return pattern && FcPatternGetBool(pattern, property, 0, &value) == FcResultMatch && value;
}

/** Restrict coverage to valid scalar values, excluding UTF-16 surrogate code units. */
static bool is_scalar(uint32_t point) {
    return point <= 0x10FFFFu && !(point >= 0xD800u && point <= 0xDFFFu);
}

CharactersFontCatalog *characters_fonts_load(void) {
    CharactersFontCatalog *catalog = calloc(1, sizeof(*catalog));
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
        FC_FONT_VARIATIONS, FC_SCALABLE, FC_SYMBOL, (char *)NULL);
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

void characters_fonts_free(CharactersFontCatalog *catalog) {
    if (!catalog) {
        return;
    }
    if (catalog->faces) {
        FcFontSetDestroy(catalog->faces);
    }
    if (catalog->config) {
        FcConfigDestroy(catalog->config);
    }
    free(catalog);
}

size_t characters_fonts_count(const CharactersFontCatalog *catalog) {
    return catalog ? (size_t)catalog->faces->nfont : 0;
}

const char *characters_fonts_family(const CharactersFontCatalog *catalog, size_t index) {
    return get_string(get_face(catalog, index), FC_FAMILY);
}

const char *characters_fonts_style(const CharactersFontCatalog *catalog, size_t index) {
    return get_string(get_face(catalog, index), FC_STYLE);
}

const char *characters_fonts_file(const CharactersFontCatalog *catalog, size_t index) {
    return get_string(get_face(catalog, index), FC_FILE);
}

int characters_fonts_face_index(const CharactersFontCatalog *catalog, size_t index) {
    return get_face_index(get_face(catalog, index));
}

char *characters_fonts_description(const CharactersFontCatalog *catalog, size_t index) {
    FcPattern *face = get_face(catalog, index);
    if (!face) {
        return NULL;
    }
    PangoFontDescription *description = pango_fc_font_description_from_pattern(face, FALSE);
    if (!description) {
        return NULL;
    }
    char *text = pango_font_description_to_string(description);
    pango_font_description_free(description);
    if (text && !g_utf8_validate(text, -1, NULL)) {
        g_free(text);
        return NULL;
    }
    return text;
}

bool characters_fonts_is_variable(const CharactersFontCatalog *catalog, size_t index) {
    return get_bool(get_face(catalog, index), FC_VARIABLE);
}

bool characters_fonts_is_color(const CharactersFontCatalog *catalog, size_t index) {
    return get_bool(get_face(catalog, index), FC_COLOR);
}

bool characters_fonts_contains(const CharactersFontCatalog *catalog, size_t index, uint32_t point) {
    FcCharSet *charset = get_charset(get_face(catalog, index));
    if (!charset || !is_scalar(point) || !FcCharSetHasChar(charset, point)) {
        return false;
    }
    if (get_bool(get_face(catalog, index), FC_SYMBOL)) {
        CharactersFontFace *face = characters_fonts_open_face(catalog, index);
        bool mapped = characters_font_face_glyph(face, point, false) != 0;
        characters_font_face_free(face);
        return mapped;
    }
    return true;
}

uint32_t *characters_fonts_points(const CharactersFontCatalog *catalog, size_t index,
                                 size_t *length) {
    if (!length) {
        return NULL;
    }
    *length = 0;
    CharactersFontFace *face = characters_fonts_open_face(catalog, index);
    uint32_t *points = characters_font_face_unicode_points(face, length);
    characters_font_face_free(face);
    return points;
}

bool characters_fonts_is_current(const CharactersFontCatalog *catalog) {
    return catalog && FcConfigUptoDate(catalog->config);
}

bool characters_fonts_apply_config(const CharactersFontCatalog *catalog, PangoFontMap *map) {
    if (!catalog || !map || !PANGO_IS_FC_FONT_MAP(map)) {
        return false;
    }
    pango_fc_font_map_set_config(PANGO_FC_FONT_MAP(map), catalog->config);
    return true;
}

bool characters_fonts_matches(const CharactersFontCatalog *catalog, size_t index, PangoFont *font) {
    FcPattern *expected = get_face(catalog, index);
    if (!expected || !font || !PANGO_IS_FC_FONT(font)) {
        return false;
    }
    FcPattern *actual = pango_fc_font_get_pattern(PANGO_FC_FONT(font));
    const char *file = get_string(actual, FC_FILE);
    const char *expected_file = get_string(expected, FC_FILE);
    int expected_index = get_face_index(expected);
    return file && expected_file && expected_index >= 0 &&
        strcmp(file, expected_file) == 0 && get_face_index(actual) == expected_index;
}

CharactersFontFace *characters_fonts_open_face(const CharactersFontCatalog *catalog, size_t index) {
    return characters_font_face_open(characters_fonts_file(catalog, index),
                                      characters_fonts_face_index(catalog, index));
}
