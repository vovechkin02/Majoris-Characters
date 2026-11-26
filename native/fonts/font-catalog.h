#ifndef CHARACTERS_FONT_CATALOG_H
#define CHARACTERS_FONT_CATALOG_H

#include <glib.h>

#include <pango/pango.h>

#include "font-face.h"

/** Immutable installed-face inventory, with retained Fontconfig patterns and configuration. */
typedef struct CharactersFontCatalog CharactersFontCatalog;

/**
 * characters_fonts_load:
 *
 * Returns: (transfer full) (nullable): Fresh raw installed font snapshot, or NULL on failure.
 * Perform discovery off the GTK main context.
 */
CharactersFontCatalog *characters_fonts_load(void);

/**
 * characters_fonts_free:
 * @catalog: (transfer full) (nullable): Inventory no longer used by any consumer.
 */
void characters_fonts_free(CharactersFontCatalog *catalog);

/**
 * characters_fonts_count:
 * @catalog: Borrowed inventory.
 *
 * Returns: Number of raw discovered patterns, before application filtering or sorting.
 */
gsize characters_fonts_count(const CharactersFontCatalog *catalog);

/**
 * characters_fonts_family:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: (transfer none) (nullable): Primary font family; valid until inventory destruction.
 */
const gchar *characters_fonts_family(const CharactersFontCatalog *catalog, gsize index);

/**
 * characters_fonts_style:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: (transfer none) (nullable): Raw Fontconfig style label; validate UTF-8 before display.
 */
const gchar *characters_fonts_style(const CharactersFontCatalog *catalog, gsize index);

/**
 * characters_fonts_file:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: (transfer none) (nullable): Installed font path, not a user-supplied path.
 */
const gchar *characters_fonts_file(const CharactersFontCatalog *catalog, gsize index);

/**
 * characters_fonts_face_index:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: Fontconfig face/named-instance index, or -1 for an invalid face.
 */
gint characters_fonts_face_index(const CharactersFontCatalog *catalog, gsize index);

/**
 * characters_fonts_description:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: (transfer full) (nullable): Pango description, released with g_free().
 * Resolve it on the main context; family/style descriptions are not unique file IDs.
 */
gchar *characters_fonts_description(const CharactersFontCatalog *catalog, gsize index);

/**
 * characters_fonts_is_variable:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: Whether Fontconfig advertises this face as variable.
 */
gboolean characters_fonts_is_variable(const CharactersFontCatalog *catalog, gsize index);

/**
 * characters_fonts_is_color:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: Whether Fontconfig advertises color glyph support.
 */
gboolean characters_fonts_is_color(const CharactersFontCatalog *catalog, gsize index);

/**
 * characters_fonts_contains:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 * @point: Unicode scalar.
 *
 * Returns: Whether the exact installed file and face currently have a nonzero
 * glyph index in the FreeType-selected Unicode cmap. Fontconfig coverage is not
 * authoritative; symbol-only faces and unreadable files are excluded. Run on a
 * worker because opening installed font files performs I/O. No fallback is used.
 * This does not guarantee a visible glyph, variation-selector mapping, or sequence shaping.
 */
gboolean characters_fonts_contains(const CharactersFontCatalog *catalog, gsize index,
                               gunichar point);

/**
 * characters_fonts_points:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 * @length: (out): Number of returned Unicode scalars, zero on failure.
 *
 * Returns: (transfer full) (array length=length) (nullable): Sorted scalar coverage,
 * including private-use and unassigned mappings. Uses the selected FreeType Unicode
 * cmap; a symbol-only face yields an empty array. Free with g_free().
 */
gunichar *characters_fonts_points(const CharactersFontCatalog *catalog, gsize index,
                                 gsize *length);

/**
 * characters_fonts_is_current:
 * @catalog: Borrowed inventory.
 *
 * Returns: Whether Fontconfig considers the retained configuration up to date.
 * Check on a worker, not in a GTK callback.
 */
gboolean characters_fonts_is_current(const CharactersFontCatalog *catalog);

/**
 * characters_fonts_apply_config:
 * @catalog: Borrowed inventory.
 * @map: (nullable): The application's Pango font map, borrowed.
 *
 * Updates the Linux PangoFc renderer with this snapshot's configuration on the GTK main context.
 * Returns: Whether the font map supports Fontconfig configuration.
 */
gboolean characters_fonts_apply_config(const CharactersFontCatalog *catalog, PangoFontMap *map);

/**
 * characters_fonts_matches:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 * @font: (nullable): Resolved Pango font, borrowed.
 *
 * Returns: Whether Pango resolved the same installed file and Fontconfig face index.
 * Non-Fontconfig renderers and unresolved fonts return false.
 */
gboolean characters_fonts_matches(const CharactersFontCatalog *catalog, gsize index, PangoFont *font);

/**
 * characters_fonts_open_face:
 * @catalog: Borrowed inventory.
 * @index: Raw pattern index within this snapshot.
 *
 * Returns: (transfer full) (nullable): Independent exact FreeType face.
 */
CharactersFontFace *characters_fonts_open_face(const CharactersFontCatalog *catalog, gsize index);

#endif
