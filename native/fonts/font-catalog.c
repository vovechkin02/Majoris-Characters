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
    /** Sorted patterns, including exact file and face identity and cmap coverage. */
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

/** Compare nullable labels without treating missing properties as valid face identities. */
static int compare_strings(const char *left, const char *right) {
    return strcmp(left ? left : "", right ? right : "");
}

/** Sort by primary family, style, file and exact face index for deterministic grouping. */
static int compare_faces(const void *left, const void *right) {
    FcPattern *a = *(FcPattern *const *)left;
    FcPattern *b = *(FcPattern *const *)right;
    int result = compare_strings(get_string(a, FC_FAMILY), get_string(b, FC_FAMILY));
    if (!result) {
        result = compare_strings(get_string(a, FC_STYLE), get_string(b, FC_STYLE));
    }
    if (!result) {
        result = compare_strings(get_string(a, FC_FILE), get_string(b, FC_FILE));
    }
    if (!result) {
        int a_index = get_face_index(a);
        int b_index = get_face_index(b);
        result = (a_index > b_index) - (a_index < b_index);
    }
    return result;
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
        FC_FONT_VARIATIONS, FC_SCALABLE, (char *)NULL);
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
    catalog->faces = FcFontSetCreate();
    if (!catalog->faces) {
        FcFontSetDestroy(listed);
        characters_fonts_free(catalog);
        return NULL;
    }
    for (int index = 0; index < listed->nfont; index++) {
        FcPattern *pattern = listed->fonts[index];
        const char *family = get_string(pattern, FC_FAMILY);
        if (!family || !g_utf8_validate(family, -1, NULL) || !get_string(pattern, FC_FILE) ||
            get_face_index(pattern) < 0 || !get_charset(pattern)) {
            continue;
        }
        FcPatternReference(pattern);
        if (!FcFontSetAdd(catalog->faces, pattern)) {
            FcPatternDestroy(pattern);
            FcFontSetDestroy(listed);
            characters_fonts_free(catalog);
            return NULL;
        }
    }
    FcFontSetDestroy(listed);
    if (catalog->faces->nfont > 1) {
        qsort(catalog->faces->fonts, (size_t)catalog->faces->nfont,
              sizeof(*catalog->faces->fonts), compare_faces);
    }
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
    const char *style = get_string(get_face(catalog, index), FC_STYLE);
    return style && g_utf8_validate(style, -1, NULL) ? style : NULL;
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
    return charset && is_scalar(point) && FcCharSetHasChar(charset, point);
}

uint32_t *characters_fonts_points(const CharactersFontCatalog *catalog, size_t index,
                                 size_t *length) {
    if (!length) {
        return NULL;
    }
    *length = 0;
    FcCharSet *charset = get_charset(get_face(catalog, index));
    if (!charset) {
        return NULL;
    }
    /* Allocate only the maximum valid Unicode space; malformed high pages are ignored. */
    size_t capacity = FcCharSetCount(charset);
    if (capacity > 0x110000u) {
        capacity = 0x110000u;
    }
    uint32_t *points = g_try_new(uint32_t, capacity ? capacity : 1);
    if (!points) {
        return NULL;
    }
    FcChar32 map[FC_CHARSET_MAP_SIZE];
    FcChar32 next;
    for (FcChar32 page = FcCharSetFirstPage(charset, map, &next);
         page != FC_CHARSET_DONE && page <= 0x10FFFFu;
         page = FcCharSetNextPage(charset, map, &next)) {
        for (unsigned word = 0; word < FC_CHARSET_MAP_SIZE; word++) {
            uint32_t bits = map[word];
            while (bits) {
                unsigned bit = (unsigned)__builtin_ctz(bits);
                uint32_t point = page + word * 32u + bit;
                if (is_scalar(point)) {
                    if (*length == capacity) {
                        g_free(points);
                        *length = 0;
                        return NULL;
                    }
                    points[(*length)++] = point;
                }
                bits &= bits - 1u;
            }
        }
    }
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
    return file && strcmp(file, get_string(expected, FC_FILE)) == 0 &&
        get_face_index(actual) == get_face_index(expected);
}
