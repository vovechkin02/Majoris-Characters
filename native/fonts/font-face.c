#include "font-face.h"

#include <math.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

/** Maximum accepted design axes in a single installed face. */
#define AXIS_LIMIT 256u
/** Upper bound on raw symbol entries, independent of malformed cmap sizes. */
#define SYMBOL_LIMIT 65536u

/** Owns FreeType resources and the exact selected instance's coordinates. */
struct CharactersFontFace {
    /** Independent library avoids cross-thread mutable FreeType state. */
    FT_Library library;
    /** Exact file and face index, including named-instance bits. */
    FT_Face face;
    /** Unicode cmap selected by FreeType, if available. */
    FT_CharMap unicode_map;
    /** First Microsoft Symbol cmap, if available. */
    FT_CharMap symbol_map;
    /** Design axes, owned by the FreeType library. */
    FT_MM_Var *axes;
    /** Current fixed-point design coordinates. */
    FT_Fixed *coordinates;
    /** Initial coordinates of the selected face/instance. */
    FT_Fixed *initial;
    /** Exact raw symbol codes, sorted and immutable after open. */
    GArray *symbols;
};

/** Check a Unicode scalar without admitting UTF-16 surrogate values. */
static bool is_scalar(uint32_t code) {
    return code <= 0x10FFFFu && !(code >= 0xD800u && code <= 0xDFFFu);
}

/** Collect only codes actually enumerated by the selected cmap. */
static GArray *collect_points(FT_Face face, bool symbol) {
    GArray *points = g_array_new(FALSE, FALSE, sizeof(uint32_t));
    FT_UInt glyph = 0;
    FT_ULong code = FT_Get_First_Char(face, &glyph);
    while (glyph) {
        if (code <= UINT32_MAX && (symbol || is_scalar((uint32_t)code))) {
            uint32_t point = (uint32_t)code;
            if (symbol && points->len == SYMBOL_LIMIT) {
                g_array_unref(points);
                return NULL;
            }
            g_array_append_val(points, point);
        }
        FT_ULong next = FT_Get_Next_Char(face, code, &glyph);
        if (glyph && next <= code) {
            g_array_unref(points);
            return NULL;
        }
        code = next;
    }
    return points;
}

/** Discover axes without discarding the selected named instance's initial coordinates. */
static void load_axes(CharactersFontFace *face) {
    if (!FT_HAS_MULTIPLE_MASTERS(face->face) ||
        FT_Get_MM_Var(face->face, &face->axes) != 0) {
        return;
    }
    FT_UInt count = face->axes->num_axis;
    if (!count || count > AXIS_LIMIT) {
        FT_Done_MM_Var(face->library, face->axes);
        face->axes = NULL;
        return;
    }
    face->coordinates = g_new(FT_Fixed, count);
    face->initial = g_new(FT_Fixed, count);
    if (FT_Get_Var_Design_Coordinates(face->face, count, face->coordinates) != 0) {
        FT_Done_MM_Var(face->library, face->axes);
        face->axes = NULL;
        g_clear_pointer(&face->coordinates, g_free);
        g_clear_pointer(&face->initial, g_free);
        return;
    }
    memcpy(face->initial, face->coordinates, count * sizeof(FT_Fixed));
}

CharactersFontFace *characters_font_face_open(const char *file, int index) {
    if (!file || index < 0) {
        return NULL;
    }
    CharactersFontFace *face = g_new0(CharactersFontFace, 1);
    if (FT_Init_FreeType(&face->library) != 0 ||
        FT_New_Face(face->library, file, index, &face->face) != 0) {
        characters_font_face_free(face);
        return NULL;
    }
    if (FT_Select_Charmap(face->face, FT_ENCODING_UNICODE) == 0) {
        face->unicode_map = face->face->charmap;
    }
    if (FT_Select_Charmap(face->face, FT_ENCODING_MS_SYMBOL) == 0) {
        face->symbol_map = face->face->charmap;
        face->symbols = collect_points(face->face, true);
        if (!face->symbols) {
            characters_font_face_free(face);
            return NULL;
        }
    }
    load_axes(face);
    return face;
}

void characters_font_face_free(CharactersFontFace *face) {
    if (!face) {
        return;
    }
    if (face->axes) {
        FT_Done_MM_Var(face->library, face->axes);
    }
    if (face->face) {
        FT_Done_Face(face->face);
    }
    if (face->library) {
        FT_Done_FreeType(face->library);
    }
    g_clear_pointer(&face->symbols, g_array_unref);
    g_free(face->coordinates);
    g_free(face->initial);
    g_free(face);
}

size_t characters_font_face_axis_count(const CharactersFontFace *face) {
    return face && face->axes ? face->axes->num_axis : 0;
}

char *characters_font_face_axis_tag(const CharactersFontFace *face, size_t index) {
    if (index >= characters_font_face_axis_count(face)) {
        return NULL;
    }
    FT_ULong tag = face->axes->axis[index].tag;
    return g_strdup_printf("%c%c%c%c", (int)((tag >> 24) & 255),
        (int)((tag >> 16) & 255), (int)((tag >> 8) & 255), (int)(tag & 255));
}

bool characters_font_face_axis_range(const CharactersFontFace *face, size_t index,
                                     double *minimum, double *maximum,
                                     double *default_value, double *current) {
    if (index >= characters_font_face_axis_count(face) ||
        !minimum || !maximum || !default_value || !current) {
        return false;
    }
    const FT_Var_Axis *axis = &face->axes->axis[index];
    *minimum = axis->minimum / 65536.0;
    *maximum = axis->maximum / 65536.0;
    *default_value = axis->def / 65536.0;
    *current = face->coordinates[index] / 65536.0;
    return true;
}

bool characters_font_face_set_axis(CharactersFontFace *face, size_t index, double value) {
    double minimum, maximum, default_value, current;
    if (!characters_font_face_axis_range(face, index, &minimum, &maximum,
                                        &default_value, &current) ||
        !isfinite(value) || value < minimum || value > maximum) {
        return false;
    }
    FT_Fixed previous = face->coordinates[index];
    face->coordinates[index] = (FT_Fixed)(value * 65536.0 + (value < 0 ? -0.5 : 0.5));
    if (FT_Set_Var_Design_Coordinates(face->face, face->axes->num_axis,
                                      face->coordinates) != 0) {
        face->coordinates[index] = previous;
        FT_Set_Var_Design_Coordinates(face->face, face->axes->num_axis, face->coordinates);
        return false;
    }
    return true;
}

bool characters_font_face_reset(CharactersFontFace *face) {
    if (!face || !face->axes || FT_Set_Var_Design_Coordinates(face->face,
            face->axes->num_axis, face->initial) != 0) {
        return false;
    }
    memcpy(face->coordinates, face->initial, face->axes->num_axis * sizeof(FT_Fixed));
    return true;
}

uint32_t *characters_font_face_unicode_points(CharactersFontFace *face, size_t *length) {
    if (!length) {
        return NULL;
    }
    *length = 0;
    if (!face) {
        return NULL;
    }
    if (!face->unicode_map) {
        return g_new0(uint32_t, 1);
    }
    if (FT_Set_Charmap(face->face, face->unicode_map) != 0) {
        return NULL;
    }
    GArray *points = collect_points(face->face, false);
    if (!points) {
        return NULL;
    }
    *length = points->len;
    if (!points->len) {
        g_array_unref(points);
        return g_new0(uint32_t, 1);
    }
    return (uint32_t *)g_array_free(points, FALSE);
}

const uint32_t *characters_font_face_symbol_points(const CharactersFontFace *face, size_t *length) {
    if (!length) {
        return NULL;
    }
    *length = face && face->symbols ? face->symbols->len : 0;
    return *length ? (const uint32_t *)face->symbols->data : NULL;
}

/** Returns the raw mapping count without transferring the underlying array. */
size_t characters_font_face_symbol_count(const CharactersFontFace *face) {
    return face && face->symbols ? face->symbols->len : 0;
}

/** Returns one raw mapping, without converting it to Unicode. */
uint32_t characters_font_face_symbol_code(const CharactersFontFace *face, size_t index) {
    if (index >= characters_font_face_symbol_count(face)) {
        return UINT32_MAX;
    }
    return g_array_index(face->symbols, uint32_t, index);
}

/** Test raw membership to avoid FreeType's symbol alias fallback for unmapped low codes. */
static bool has_symbol(const CharactersFontFace *face, uint32_t code) {
    size_t length;
    const uint32_t *points = characters_font_face_symbol_points(face, &length);
    size_t low = 0;
    size_t high = length;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (points[middle] == code) {
            return true;
        }
        if (points[middle] < code) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return false;
}

uint32_t characters_font_face_glyph(CharactersFontFace *face, uint32_t code, bool symbol) {
    if (!face || (symbol ? !has_symbol(face, code) : !is_scalar(code))) {
        return 0;
    }
    FT_CharMap map = symbol ? face->symbol_map : face->unicode_map;
    if (!map || FT_Set_Charmap(face->face, map) != 0) {
        return 0;
    }
    return FT_Get_Char_Index(face->face, code);
}

/** Select a scalable size or the closest actual bitmap strike. */
static bool set_size(FT_Face face, unsigned size_px) {
    if (FT_Set_Pixel_Sizes(face, 0, size_px) == 0) {
        return true;
    }
    int best = -1;
    long distance = G_MAXLONG;
    for (int index = 0; index < face->num_fixed_sizes; index++) {
        long delta = (long)face->available_sizes[index].height - (long)size_px;
        if (delta < 0) {
            delta = -delta;
        }
        if (delta < distance) {
            best = index;
            distance = delta;
        }
    }
    return best >= 0 && FT_Select_Size(face, best) == 0;
}

/** Convert a premultiplied FreeType color channel into straight RGBA. */
static uint8_t unpremultiply(uint8_t channel, uint8_t alpha) {
    return alpha ? (uint8_t)MIN(255u, ((unsigned)channel * 255u + alpha / 2u) / alpha) : 0;
}

/** Convert one supported FreeType bitmap pixel, preserving native color when present. */
static void read_pixel(const FT_Bitmap *bitmap, const uint8_t *row, unsigned x,
                       uint8_t *pixel, uint8_t red, uint8_t green, uint8_t blue) {
    unsigned alpha = 0;
    switch (bitmap->pixel_mode) {
        case FT_PIXEL_MODE_BGRA:
            pixel[3] = row[x * 4u + 3u];
            pixel[0] = unpremultiply(row[x * 4u + 2u], pixel[3]);
            pixel[1] = unpremultiply(row[x * 4u + 1u], pixel[3]);
            pixel[2] = unpremultiply(row[x * 4u], pixel[3]);
            return;
        case FT_PIXEL_MODE_MONO:
            alpha = (row[x / 8u] & (0x80u >> (x % 8u))) ? 255u : 0u;
            break;
        case FT_PIXEL_MODE_GRAY:
            alpha = bitmap->num_grays > 1 ?
                MIN(255u, (unsigned)row[x] * 255u / (bitmap->num_grays - 1u)) : 0;
            break;
        case FT_PIXEL_MODE_GRAY2:
            alpha = ((row[x / 4u] >> (6u - 2u * (x % 4u))) & 3u) * 85u;
            break;
        case FT_PIXEL_MODE_GRAY4:
            alpha = ((row[x / 2u] >> (4u - 4u * (x % 2u))) & 15u) * 17u;
            break;
        default:
            break;
    }
    pixel[0] = red;
    pixel[1] = green;
    pixel[2] = blue;
    pixel[3] = (uint8_t)alpha;
}

GBytes *characters_font_face_render(CharactersFontFace *face, uint32_t code, bool symbol,
                                    unsigned size_px, uint8_t red, uint8_t green, uint8_t blue) {
    if (size_px < 8u || size_px > 192u) {
        return NULL;
    }
    uint32_t glyph = characters_font_face_glyph(face, code, symbol);
    if (!glyph || !set_size(face->face, size_px) ||
        FT_Load_Glyph(face->face, glyph, FT_LOAD_COLOR) != 0) {
        return NULL;
    }
    FT_GlyphSlot slot = face->face->glyph;
    if (slot->format != FT_GLYPH_FORMAT_BITMAP &&
        FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL) != 0) {
        return NULL;
    }
    const FT_Bitmap *bitmap = &slot->bitmap;
    unsigned side = size_px * 2u;
    size_t length = (size_t)side * side * 4u;
    uint8_t *pixels = g_malloc0(length);
    if (!bitmap->width || !bitmap->rows) {
        return g_bytes_new_take(pixels, length);
    }
    if (!bitmap->buffer || bitmap->width > 16384u || bitmap->rows > 16384u ||
        (bitmap->pixel_mode != FT_PIXEL_MODE_BGRA && bitmap->pixel_mode != FT_PIXEL_MODE_GRAY &&
         bitmap->pixel_mode != FT_PIXEL_MODE_MONO && bitmap->pixel_mode != FT_PIXEL_MODE_GRAY2 &&
         bitmap->pixel_mode != FT_PIXEL_MODE_GRAY4)) {
        g_free(pixels);
        return NULL;
    }
    size_t row_bytes = bitmap->width;
    switch (bitmap->pixel_mode) {
        case FT_PIXEL_MODE_BGRA:
            row_bytes *= 4u;
            break;
        case FT_PIXEL_MODE_MONO:
            row_bytes = (row_bytes + 7u) / 8u;
            break;
        case FT_PIXEL_MODE_GRAY2:
            row_bytes = (row_bytes + 3u) / 4u;
            break;
        case FT_PIXEL_MODE_GRAY4:
            row_bytes = (row_bytes + 1u) / 2u;
            break;
        default:
            break;
    }
    int64_t pitch = bitmap->pitch;
    if ((uint64_t)(pitch < 0 ? -pitch : pitch) < row_bytes ||
        (uint64_t)(pitch < 0 ? -pitch : pitch) * bitmap->rows > PTRDIFF_MAX) {
        g_free(pixels);
        return NULL;
    }
    unsigned width = bitmap->width;
    unsigned height = bitmap->rows;
    unsigned largest = MAX(width, height);
    if (largest > side) {
        width = MAX(1u, width * side / largest);
        height = MAX(1u, height * side / largest);
    }
    unsigned left = (side - width) / 2u;
    unsigned top = (side - height) / 2u;
    for (unsigned y = 0; y < height; y++) {
        unsigned source_y = y * bitmap->rows / height;
        /* FreeType pitch is the signed offset to the next visual row. */
        const uint8_t *row = bitmap->buffer + (ptrdiff_t)source_y * bitmap->pitch;
        for (unsigned x = 0; x < width; x++) {
            unsigned source_x = x * bitmap->width / width;
            uint8_t *pixel = pixels + ((size_t)(top + y) * side + left + x) * 4u;
            read_pixel(bitmap, row, source_x, pixel, red, green, blue);
        }
    }
    return g_bytes_new_take(pixels, length);
}
