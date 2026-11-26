#ifndef CHARACTERS_UNICODE_ENGINE_H
#define CHARACTERS_UNICODE_ENGINE_H

#include <glib.h>

#include "icu-adapter.h"

/** Maximum number of scalars in a supported catalog sequence. */
#define CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH 32

/**
 * characters_unicode_is_scalar:
 * @point: Numeric Unicode code point.
 *
 * Returns: Whether @point is within Unicode and not a surrogate.
 */
gboolean characters_unicode_is_scalar(gunichar point);

/**
 * characters_unicode_is_assigned:
 * @point: Numeric Unicode code point.
 *
 * Returns: Whether the Unicode 17 property snapshot assigns the scalar.
 * Private-use scalars count as assigned.
 */
gboolean characters_unicode_is_assigned(gunichar point);

/**
 * characters_unicode_properties_version:
 *
 * Returns: (transfer full): The Unicode version of the bundled scalar property snapshot.
 */
gchar *characters_unicode_properties_version(void);

/**
 * characters_unicode_catalog_version:
 *
 * Returns: (transfer none): The immutable GNOME-derived catalog version.
 */
const gchar *characters_unicode_catalog_version(void);

/**
 * characters_unicode_supplement_version:
 *
 * Returns: (transfer none): The immutable alias and named-sequence data version.
 */
const gchar *characters_unicode_supplement_version(void);

/**
 * characters_unicode_unihan_version:
 *
 * Returns: (transfer none): Official Unihan version, or "none" for fallback-only data.
 */
const gchar *characters_unicode_unihan_version(void);

/**
 * characters_unicode_name:
 * @point: Numeric Unicode scalar.
 *
 * Returns: (transfer full) (nullable): Canonical name, or NULL when unavailable.
 * Controls, private-use characters and unnamed scalars do not have canonical names.
 */
gchar *characters_unicode_name(gunichar point);

/**
 * characters_unicode_aliases:
 * @point: Numeric Unicode scalar.
 *
 * Returns: (transfer full) (nullable): Semicolon-separated official aliases,
 * or NULL when none are available. No alias is represented as a canonical name.
 */
gchar *characters_unicode_aliases(gunichar point);

/**
 * characters_unicode_script_name:
 * @code: ISO 15924 script code.
 *
 * Returns: (transfer none) (nullable): Stable script name, or NULL for an unknown code.
 */
const gchar *characters_unicode_script_name(const gchar *code);

/**
 * characters_unicode_block_count:
 *
 * Returns: Number of blocks in the independently versioned catalog.
 */
gsize characters_unicode_block_count(void);

/**
 * characters_unicode_block_name:
 * @index: Zero-based block index.
 *
 * Returns: (transfer none) (nullable): Catalog block name, or NULL out of range.
 */
const gchar *characters_unicode_block_name(gsize index);

/**
 * characters_unicode_block_start:
 * @index: Zero-based block index.
 *
 * Returns: Inclusive start or G_MAXUINT32 out of range.
 */
guint32 characters_unicode_block_start(gsize index);

/**
 * characters_unicode_block_end:
 * @index: Zero-based block index.
 *
 * Returns: Inclusive end or G_MAXUINT32 out of range.
 */
guint32 characters_unicode_block_end(gsize index);

/**
 * characters_unicode_find_block:
 * @point: Numeric Unicode code point, including surrogate code points.
 *
 * Returns: (transfer none) (nullable): Block name or NULL outside a defined block.
 */
const gchar *characters_unicode_find_block(gunichar point);

/**
 * characters_unicode_catalog_sequence_name:
 * @points: (array length=length): Unicode scalars.
 * @length: Number of scalars, from 1 to 32.
 *
 * Returns: (transfer full) (nullable): Canonical scalar name, named sequence
 * name or fully qualified emoji label; NULL for unknown or supplementary alias sequences.
 * Scalar emoji labels are
 * available separately through the emoji catalog.
 */
gchar *characters_unicode_catalog_sequence_name(const gunichar *points, gsize length);

/**
 * characters_unicode_emoji_group_count:
 *
 * Returns: Number of semantic emoji groups.
 */
gsize characters_unicode_emoji_group_count(void);

/**
 * characters_unicode_emoji_group_name:
 * @group: Zero-based group index.
 *
 * Returns: (transfer none) (nullable): Stable English group name or NULL.
 */
const gchar *characters_unicode_emoji_group_name(gsize group);

/**
 * characters_unicode_emoji_group_size:
 * @group: Zero-based group index.
 *
 * Returns: Number of entries, zero for invalid group.
 */
gsize characters_unicode_emoji_group_size(gsize group);

/**
 * characters_unicode_emoji_group_index:
 * @group: Zero-based group index.
 * @index: Position in the group's display order.
 *
 * Returns: Catalog index or G_MAXSIZE for invalid arguments.
 */
gsize characters_unicode_emoji_group_index(gsize group, gsize index);

/**
 * characters_unicode_emoji_index:
 * @points: (array length=length): Exact Unicode sequence.
 * @length: Number of scalars in the sequence.
 *
 * Returns: Catalog index, or G_MAXSIZE if the sequence is not an exact entry.
 */
gsize characters_unicode_emoji_index(const gunichar *points, gsize length);

/**
 * characters_unicode_emoji_count:
 *
 * Returns: Number of fully qualified emoji sequences in the catalog.
 */
gsize characters_unicode_emoji_count(void);

/**
 * characters_unicode_emoji_text:
 * @index: Zero-based catalog index.
 *
 * Returns: (transfer full) (nullable): Exact UTF-8 sequence or NULL.
 */
gchar *characters_unicode_emoji_text(gsize index);

/**
 * characters_unicode_emoji_label:
 * @index: Zero-based catalog index.
 *
 * Returns: (transfer none) (nullable): Catalog label or NULL.
 */
const gchar *characters_unicode_emoji_label(gsize index);

/**
 * characters_unicode_named_count:
 *
 * Returns: Number of standardized named sequences in the supplement.
 */
gsize characters_unicode_named_count(void);

/**
 * characters_unicode_named_name:
 * @index: Zero-based named-sequence index.
 *
 * Returns: (transfer none) (nullable): Standardized name or NULL.
 */
const gchar *characters_unicode_named_name(gsize index);

/**
 * characters_unicode_named_text:
 * @index: Zero-based named-sequence index.
 *
 * Returns: (transfer full) (nullable): Exact UTF-8 sequence or NULL.
 */
gchar *characters_unicode_named_text(gsize index);

/**
 * characters_unicode_confusables:
 * @point: Valid Unicode scalar.
 * @count: (out): Number of returned class members.
 *
 * Returns: (transfer full) (array length=count) (nullable): Exact members of
 * the immutable single-scalar confusable class, including the input if present.
 * Empty classes return an allocated empty vector; NULL indicates invalid input,
 * invalid table bounds or allocation failure. This is not a security test.
 */
gunichar *characters_unicode_confusables(gunichar point, gint *count);

#endif
