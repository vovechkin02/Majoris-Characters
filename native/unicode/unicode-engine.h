#ifndef CHARACTERS_UNICODE_ENGINE_H
#define CHARACTERS_UNICODE_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "icu-adapter.h"

/** Maximum number of scalars in a supported catalog sequence. */
#define CHARACTERS_UNICODE_MAX_SEQUENCE_LENGTH 32

/**
 * characters_unicode_is_scalar:
 * @point: Numeric Unicode code point.
 *
 * Returns: Whether @point is within Unicode and not a surrogate.
 */
bool characters_unicode_is_scalar(uint32_t point);

/**
 * characters_unicode_is_assigned:
 * @point: Numeric Unicode code point.
 *
 * Returns: Whether the Unicode 17 property snapshot assigns the scalar.
 * Private-use scalars count as assigned.
 */
bool characters_unicode_is_assigned(uint32_t point);

/**
 * characters_unicode_properties_version:
 *
 * Returns: (transfer full): The Unicode version of the bundled scalar property snapshot.
 */
char *characters_unicode_properties_version(void);

/**
 * characters_unicode_catalog_version:
 *
 * Returns: (transfer none): The immutable GNOME-derived catalog version.
 */
const char *characters_unicode_catalog_version(void);

/**
 * characters_unicode_supplement_version:
 *
 * Returns: (transfer none): The immutable alias and named-sequence data version.
 */
const char *characters_unicode_supplement_version(void);

/**
 * characters_unicode_unihan_version:
 *
 * Returns: (transfer none): Official Unihan version, or "none" for fallback-only data.
 */
const char *characters_unicode_unihan_version(void);

/**
 * characters_unicode_name:
 * @point: Numeric Unicode scalar.
 *
 * Returns: (transfer full) (nullable): Canonical name, or NULL when unavailable.
 * Controls, private-use characters and unnamed scalars do not have canonical names.
 */
char *characters_unicode_name(uint32_t point);

/**
 * characters_unicode_aliases:
 * @point: Numeric Unicode scalar.
 *
 * Returns: (transfer full) (nullable): Semicolon-separated official aliases,
 * or NULL when none are available. No alias is represented as a canonical name.
 */
char *characters_unicode_aliases(uint32_t point);

/**
 * characters_unicode_script_name:
 * @code: ISO 15924 script code.
 *
 * Returns: (transfer none) (nullable): Stable script name, or NULL for an unknown code.
 */
const char *characters_unicode_script_name(const char *code);

/**
 * characters_unicode_property:
 * @point: Numeric Unicode scalar.
 * @property: Supported UCD or Han metadata key (see the Unicode engine documentation).
 *
 * Returns: (transfer full) (nullable): Property value, or NULL for unsupported
 * queries, invalid scalars, or scalars not assigned in the scalar property snapshot.
 * Plane is available for all valid scalars.
 */
char *characters_unicode_property(uint32_t point, const char *property);

/**
 * characters_unicode_block_count:
 *
 * Returns: Number of blocks in the independently versioned catalog.
 */
size_t characters_unicode_block_count(void);

/**
 * characters_unicode_block_name:
 * @index: Zero-based block index.
 *
 * Returns: (transfer none) (nullable): Catalog block name, or NULL out of range.
 */
const char *characters_unicode_block_name(size_t index);

/**
 * characters_unicode_block_start:
 * @index: Zero-based block index.
 *
 * Returns: Inclusive start or UINT32_MAX out of range.
 */
uint32_t characters_unicode_block_start(size_t index);

/**
 * characters_unicode_block_end:
 * @index: Zero-based block index.
 *
 * Returns: Inclusive end or UINT32_MAX out of range.
 */
uint32_t characters_unicode_block_end(size_t index);

/**
 * characters_unicode_find_block:
 * @point: Numeric Unicode code point, including surrogate code points.
 *
 * Returns: (transfer none) (nullable): Block name or NULL outside a defined block.
 */
const char *characters_unicode_find_block(uint32_t point);

/**
 * characters_unicode_sequence_name:
 * @points: (array length=length): Unicode scalars.
 * @length: Number of scalars, from 1 to 32.
 *
 * Returns: (transfer full) (nullable): Canonical scalar name, named sequence
 * name or emoji label; NULL for unknown sequences. Scalar emoji labels are
 * available separately through the emoji catalog.
 */
char *characters_unicode_sequence_name(const uint32_t *points, size_t length);

/**
 * characters_unicode_emoji_group_count:
 *
 * Returns: Number of semantic emoji groups.
 */
size_t characters_unicode_emoji_group_count(void);

/**
 * characters_unicode_emoji_group_name:
 * @group: Zero-based group index.
 *
 * Returns: (transfer none) (nullable): Stable English group name or NULL.
 */
const char *characters_unicode_emoji_group_name(size_t group);

/**
 * characters_unicode_emoji_group_size:
 * @group: Zero-based group index.
 *
 * Returns: Number of entries, zero for invalid group.
 */
size_t characters_unicode_emoji_group_size(size_t group);

/**
 * characters_unicode_emoji_group_index:
 * @group: Zero-based group index.
 * @index: Position in the group's display order.
 *
 * Returns: Catalog index or SIZE_MAX for invalid arguments.
 */
size_t characters_unicode_emoji_group_index(size_t group, size_t index);

/**
 * characters_unicode_emoji_index:
 * @points: (array length=length): Exact Unicode sequence.
 * @length: Number of scalars in the sequence.
 *
 * Returns: Catalog index, or SIZE_MAX if the sequence is not an exact entry.
 */
size_t characters_unicode_emoji_index(const uint32_t *points, size_t length);

/**
 * characters_unicode_emoji_count:
 *
 * Returns: Number of fully qualified emoji sequences in the catalog.
 */
size_t characters_unicode_emoji_count(void);

/**
 * characters_unicode_emoji_text:
 * @index: Zero-based catalog index.
 *
 * Returns: (transfer full) (nullable): Exact UTF-8 sequence or NULL.
 */
char *characters_unicode_emoji_text(size_t index);

/**
 * characters_unicode_emoji_label:
 * @index: Zero-based catalog index.
 *
 * Returns: (transfer none) (nullable): Catalog label or NULL.
 */
const char *characters_unicode_emoji_label(size_t index);

/**
 * characters_unicode_named_count:
 *
 * Returns: Number of standardized named sequences in the supplement.
 */
size_t characters_unicode_named_count(void);

/**
 * characters_unicode_named_name:
 * @index: Zero-based named-sequence index.
 *
 * Returns: (transfer none) (nullable): Standardized name or NULL.
 */
const char *characters_unicode_named_name(size_t index);

/**
 * characters_unicode_named_text:
 * @index: Zero-based named-sequence index.
 *
 * Returns: (transfer full) (nullable): Exact UTF-8 sequence or NULL.
 */
char *characters_unicode_named_text(size_t index);

/**
 * characters_unicode_related_scalars:
 * @point: Valid Unicode scalar.
 * @count: (out): Number of returned scalar candidates.
 *
 * Returns: (transfer full) (array length=count) (nullable): Raw scalar candidates
 * from case, mirror, canonical and confusable lookups. Duplicates and the input
 * can occur; no presentation or sequence policy is applied. Empty results are
 * allocated empty vectors; NULL indicates invalid input or allocation failure.
 */
uint32_t *characters_unicode_related_scalars(uint32_t point, int *count);

#endif
