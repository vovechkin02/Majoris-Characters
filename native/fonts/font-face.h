#ifndef CHARACTERS_FONT_FACE_H
#define CHARACTERS_FONT_FACE_H

#include <glib.h>

/** Mutable, thread-confined FreeType face with independent variation coordinates. */
typedef struct CharactersFontFace CharactersFontFace;

/**
 * characters_font_face_open:
 * @file: Installed font path (native filesystem encoding).
 * @index: Exact FreeType face/named-instance index.
 *
 * Returns: (transfer full) (nullable): Independent face or NULL on failure.
 */
CharactersFontFace *characters_font_face_open(const gchar *file, gint index);

/**
 * characters_font_face_free:
 * @face: (transfer full) (nullable): Face and its FreeType library.
 */
void characters_font_face_free(CharactersFontFace *face);

/**
 * characters_font_face_axis_count:
 * @face: Borrowed face.
 *
 * Returns: Number of adjustable design axes.
 */
gsize characters_font_face_axis_count(const CharactersFontFace *face);

/**
 * characters_font_face_axis_tag:
 * @face: Borrowed face.
 * @index: Axis index.
 *
 * Returns: (transfer full) (nullable): Four-character OpenType tag.
 */
gchar *characters_font_face_axis_tag(const CharactersFontFace *face, gsize index);

/**
 * characters_font_face_axis_range:
 * @face: Borrowed face.
 * @index: Axis index.
 * @minimum: (out): Minimum design coordinate.
 * @maximum: (out): Maximum design coordinate.
 * @default_value: (out): Font's declared default coordinate.
 * @current: (out): Current coordinate (initially the selected instance's value).
 *
 * Returns: Whether the axis and output arguments are valid.
 */
gboolean characters_font_face_axis_range(const CharactersFontFace *face, gsize index,
                                     gdouble *minimum, gdouble *maximum,
                                     gdouble *default_value, gdouble *current);

/**
 * characters_font_face_set_axis:
 * @face: Borrowed mutable face.
 * @index: Axis index.
 * @value: Finite design coordinate within declared limits.
 *
 * Returns: Whether FreeType accepted the complete coordinate vector.
 */
gboolean characters_font_face_set_axis(CharactersFontFace *face, gsize index, gdouble value);

/**
 * characters_font_face_reset:
 * @face: Borrowed mutable face.
 *
 * Returns: Whether the selected named instance or base coordinates were restored.
 */
gboolean characters_font_face_reset(CharactersFontFace *face);

/**
 * characters_font_face_unicode_points:
 * @face: Borrowed mutable face.
 * @length: (out): Number of mapped Unicode scalars.
 *
 * Returns: (transfer full) (array length=length) (nullable): Sorted exact cmap scalars;
 * empty for a symbol-only face. Free with g_free().
 */
gunichar *characters_font_face_unicode_points(CharactersFontFace *face, gsize *length);

/**
 * characters_font_face_symbol_count:
 * @face: Borrowed face.
 *
 * Returns: Number of exact raw symbol mappings.
 */
gsize characters_font_face_symbol_count(const CharactersFontFace *face);

/**
 * characters_font_face_symbol_code:
 * @face: Borrowed face.
 * @index: Raw mapping index.
 *
 * Returns: Raw mapped code or G_MAXUINT32 if the index is invalid.
 */
guint32 characters_font_face_symbol_code(const CharactersFontFace *face, gsize index);

/**
 * characters_font_face_glyph:
 * @face: Borrowed mutable face.
 * @code: Unicode scalar or exact raw symbol code.
 * @symbol: Select Microsoft Symbol cmap instead of Unicode.
 *
 * Returns: Actual glyph index, or zero when not mapped.
 */
guint32 characters_font_face_glyph(CharactersFontFace *face, guint32 code, gboolean symbol);

/**
 * characters_font_face_render:
 * @face: Borrowed mutable face.
 * @code: Unicode scalar or exact raw symbol code.
 * @symbol: Select the Microsoft Symbol cmap.
 * @size_px: Requested pixel size, 8–192.
 * @red: Foreground red component for monochrome outlines.
 * @green: Foreground green component.
 * @blue: Foreground blue component.
 *
 * Returns: (transfer full) (nullable): Straight RGBA bytes, square side 2 * size_px,
 * tightly packed. NULL means unavailable, never a substituted face. An empty mapped
 * glyph returns a transparent image. Bitmap strikes may be scaled to fit.
 */
GBytes *characters_font_face_render(CharactersFontFace *face, guint32 code, gboolean symbol,
                                    guint size_px, guint8 red, guint8 green, guint8 blue);

#endif
