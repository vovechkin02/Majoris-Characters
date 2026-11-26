#include "font-face.h"

#include <math.h>
#include <string.h>

#include <glib.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

#include "../scalar-validity.h"

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

/** Collect only codes actually enumerated by the selected cmap. */
static GArray *collect_points(FT_Face face, gboolean symbol)
{
    GArray *points = g_array_new(FALSE, FALSE, sizeof(guint32));
    FT_UInt glyph = 0;
    FT_ULong code = FT_Get_First_Char(face, &glyph);
    while (glyph) {
        if (code <= G_MAXUINT32 && (symbol || characters_scalar_is_valid((guint32)code))) {
            guint32 mapped_code = (guint32)code;
            if (symbol && points->len == SYMBOL_LIMIT) {
                g_array_unref(points);
                return NULL;
            }
            g_array_append_val(points, mapped_code);
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
static void load_axes(CharactersFontFace *face)
{
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

/** Check one exact Unicode cmap without allocating axis or symbol metadata. */
gboolean characters_font_file_contains(const gchar *file, gint index, gunichar point)
{
    if (!file || index < 0 || !characters_scalar_is_valid(point)) {
        return FALSE;
    }
    FT_Library library;
    if (FT_Init_FreeType(&library) != 0) {
        return FALSE;
    }
    FT_Face face;
    gboolean mapped = FALSE;
    if (FT_New_Face(library, file, index, &face) == 0) {
        mapped = FT_Select_Charmap(face, FT_ENCODING_UNICODE) == 0 &&
            FT_Get_Char_Index(face, point) != 0;
        FT_Done_Face(face);
    }
    FT_Done_FreeType(library);
    return mapped;
}

CharactersFontFace *characters_font_face_open(const gchar *file, gint index)
{
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
        face->symbols = collect_points(face->face, TRUE);
        if (!face->symbols) {
            characters_font_face_free(face);
            return NULL;
        }
    }
    load_axes(face);
    return face;
}

void characters_font_face_free(CharactersFontFace *face)
{
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

gsize characters_font_face_axis_count(const CharactersFontFace *face)
{
    return face && face->axes ? face->axes->num_axis : 0;
}

gchar *characters_font_face_axis_tag(const CharactersFontFace *face, gsize index)
{
    if (index >= characters_font_face_axis_count(face)) {
        return NULL;
    }
    FT_ULong tag = face->axes->axis[index].tag;
    return g_strdup_printf("%c%c%c%c", (gint)((tag >> 24) & 255),
        (gint)((tag >> 16) & 255), (gint)((tag >> 8) & 255), (gint)(tag & 255));
}

gboolean characters_font_face_axis_range(const CharactersFontFace *face, gsize index,
                                     gdouble *minimum, gdouble *maximum,
                                     gdouble *default_value, gdouble *current)
{
    if (index >= characters_font_face_axis_count(face) ||
        !minimum || !maximum || !default_value || !current) {
        return FALSE;
    }
    const FT_Var_Axis *axis = &face->axes->axis[index];
    *minimum = axis->minimum / 65536.0;
    *maximum = axis->maximum / 65536.0;
    *default_value = axis->def / 65536.0;
    *current = face->coordinates[index] / 65536.0;
    return TRUE;
}

gboolean characters_font_face_set_axis(CharactersFontFace *face, gsize index, gdouble value)
{
    gdouble minimum, maximum, default_value, current;
    if (!characters_font_face_axis_range(face, index, &minimum, &maximum,
                                        &default_value, &current) ||
        !isfinite(value) || value < minimum || value > maximum) {
        return FALSE;
    }
    FT_Fixed previous = face->coordinates[index];
    face->coordinates[index] = (FT_Fixed)(value * 65536.0 + (value < 0 ? -0.5 : 0.5));
    if (FT_Set_Var_Design_Coordinates(face->face, face->axes->num_axis,
                                      face->coordinates) != 0) {
        face->coordinates[index] = previous;
        FT_Set_Var_Design_Coordinates(face->face, face->axes->num_axis, face->coordinates);
        return FALSE;
    }
    return TRUE;
}

gboolean characters_font_face_reset(CharactersFontFace *face)
{
    if (!face || !face->axes || FT_Set_Var_Design_Coordinates(face->face,
            face->axes->num_axis, face->initial) != 0) {
        return FALSE;
    }
    memcpy(face->coordinates, face->initial, face->axes->num_axis * sizeof(FT_Fixed));
    return TRUE;
}

gunichar *characters_font_face_unicode_points(CharactersFontFace *face, gsize *length)
{
    if (!length) {
        return NULL;
    }
    *length = 0;
    if (!face) {
        return NULL;
    }
    if (!face->unicode_map) {
        return g_new0(guint32, 1);
    }
    if (FT_Set_Charmap(face->face, face->unicode_map) != 0) {
        return NULL;
    }
    GArray *points = collect_points(face->face, FALSE);
    if (!points) {
        return NULL;
    }
    *length = points->len;
    if (!points->len) {
        g_array_unref(points);
        return g_new0(guint32, 1);
    }
    return (guint32 *)g_array_free(points, FALSE);
}

/** Borrow sorted raw symbol codes for internal membership lookup. */
static const guint32 *symbol_points(const CharactersFontFace *face, gsize *length)
{
    if (!length) {
        return NULL;
    }
    *length = face && face->symbols ? face->symbols->len : 0;
    return *length ? (const guint32 *)face->symbols->data : NULL;
}

/** Returns the raw mapping count without transferring the underlying array. */
gsize characters_font_face_symbol_count(const CharactersFontFace *face)
{
    return face && face->symbols ? face->symbols->len : 0;
}

/** Returns one raw mapping, without converting it to Unicode. */
guint32 characters_font_face_symbol_code(const CharactersFontFace *face, gsize index)
{
    if (index >= characters_font_face_symbol_count(face)) {
        return G_MAXUINT32;
    }
    return g_array_index(face->symbols, guint32, index);
}

/** Test raw membership to avoid FreeType's symbol alias fallback for unmapped low codes. */
static gboolean has_symbol(const CharactersFontFace *face, guint32 code)
{
    gsize length;
    const gunichar *points = symbol_points(face, &length);
    gsize low = 0;
    gsize high = length;
    while (low < high) {
        gsize middle = low + (high - low) / 2;
        if (points[middle] == code) {
            return TRUE;
        }
        if (points[middle] < code) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return FALSE;
}

guint32 characters_font_face_glyph(CharactersFontFace *face, guint32 code, gboolean symbol)
{
    if (!face || (symbol ? !has_symbol(face, code) : !characters_scalar_is_valid(code))) {
        return 0;
    }
    FT_CharMap map = symbol ? face->symbol_map : face->unicode_map;
    if (!map || FT_Set_Charmap(face->face, map) != 0) {
        return 0;
    }
    return FT_Get_Char_Index(face->face, code);
}

/** Select a scalable size or the closest actual bitmap strike. */
static gboolean set_size(FT_Face face, guint size_px)
{
    if (FT_Set_Pixel_Sizes(face, 0, size_px) == 0) {
        return TRUE;
    }
    gint best = -1;
    long distance = G_MAXLONG;
    for (gint index = 0; index < face->num_fixed_sizes; index++) {
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
static guint8 unpremultiply(guint8 channel, guint8 alpha)
{
    return alpha ? (guint8)MIN(255u, ((guint)channel * 255u + alpha / 2u) / alpha) : 0;
}

/** Convert one supported FreeType bitmap pixel, preserving native color when present. */
static void read_pixel(const FT_Bitmap *bitmap, const guint8 *row, guint x,
                       guint8 *pixel, guint8 red, guint8 green, guint8 blue)
{
    guint alpha = 0;
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
                MIN(255u, (guint)row[x] * 255u / (bitmap->num_grays - 1u)) : 0;
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
    pixel[3] = (guint8)alpha;
}

GBytes *characters_font_face_render(CharactersFontFace *face, guint32 code, gboolean symbol,
                                    guint size_px, guint8 red, guint8 green, guint8 blue)
{
    if (size_px < 8u || size_px > 192u) {
        return NULL;
    }
    guint32 glyph = characters_font_face_glyph(face, code, symbol);
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
    guint side = size_px * 2u;
    gsize length = (gsize)side * side * 4u;
    guint8 *pixels = g_malloc0(length);
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
    gsize row_bytes = bitmap->width;
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
    gint64 pitch = bitmap->pitch;
    if ((guint64)(pitch < 0 ? -pitch : pitch) < row_bytes ||
        (guint64)(pitch < 0 ? -pitch : pitch) * bitmap->rows > PTRDIFF_MAX) {
        g_free(pixels);
        return NULL;
    }
    guint width = bitmap->width;
    guint height = bitmap->rows;
    guint largest = MAX(width, height);
    if (largest > side) {
        width = MAX(1u, width * side / largest);
        height = MAX(1u, height * side / largest);
    }
    guint left = (side - width) / 2u;
    guint top = (side - height) / 2u;
    for (guint y = 0; y < height; y++) {
        guint source_y = y * bitmap->rows / height;
        /* FreeType pitch is the signed offset to the next visual row. */
        const guint8 *row = bitmap->buffer + (ptrdiff_t)source_y * bitmap->pitch;
        for (guint x = 0; x < width; x++) {
            guint source_x = x * bitmap->width / width;
            guint8 *pixel = pixels + ((gsize)(top + y) * side + left + x) * 4u;
            read_pixel(bitmap, row, source_x, pixel, red, green, blue);
        }
    }
    return g_bytes_new_take(pixels, length);
}
