#ifndef CHARACTERS_UNIHAN_H
#define CHARACTERS_UNIHAN_H

#include <stdint.h>

#include <glib.h>

/** Field identifiers in the immutable generated record order. */
typedef enum {
    /** Mandarin reading. */
    CHARACTERS_HAN_VALUE_MANDARIN,
    /** Cantonese reading. */
    CHARACTERS_HAN_VALUE_CANTONESE,
    /** Japanese on reading. */
    CHARACTERS_HAN_VALUE_JAPANESE_ON,
    /** Japanese kun reading. */
    CHARACTERS_HAN_VALUE_JAPANESE_KUN,
    /** Japanese reading. */
    CHARACTERS_HAN_VALUE_JAPANESE,
    /** Korean reading. */
    CHARACTERS_HAN_VALUE_KOREAN,
    /** Hangul reading. */
    CHARACTERS_HAN_VALUE_HANGUL,
    /** English definition. */
    CHARACTERS_HAN_VALUE_DEFINITION,
    /** Official radical and residual strokes. */
    CHARACTERS_HAN_VALUE_RADICAL,
    /** Official total strokes. */
    CHARACTERS_HAN_VALUE_STROKES,
    /** Separately attributed Mandarin fallback. */
    CHARACTERS_HAN_VALUE_MANDARIN_FALLBACK,
    /** Separately attributed Hangul fallback. */
    CHARACTERS_HAN_VALUE_HANGUL_FALLBACK,
    /** Exclusive upper bound; not a field. */
    CHARACTERS_HAN_VALUE_COUNT
} CharactersHanValue;

/** Borrow a field without replacing its provenance; NULL when absent. */
const gchar *characters_unihan_value(gunichar point, CharactersHanValue field);

/** Return the raw collation fallback group, or zero when absent. */
guint characters_unihan_collation(gunichar point, gboolean radical);

/** Return the official data release, or "none" when only fallback data is bundled. */
const gchar *characters_unihan_version(void);

/** Prepared Han predicate kinds shared with the Vala query policy. */
typedef enum {
    /** Ordinary metadata term; other scalar metadata may also satisfy it. */
    CHARACTERS_HAN_ORDINARY,
    /** Explicit Han reading or definition field mask. */
    CHARACTERS_HAN_FIELD,
    /** Exact numeric radical group. */
    CHARACTERS_HAN_RADICAL,
    /** Exact numeric stroke group. */
    CHARACTERS_HAN_STROKES,
    /** Invalid explicit field value; never matches. */
    CHARACTERS_HAN_REJECT
} CharactersHanKind;

/** Bit positions of textual fields in the immutable HanRecord field order. */
#define CHARACTERS_HAN_TEXT_FIELDS UINT32_C(0xCFF)

/** Return owned ICU-folded reading text or NULL on invalid input or overflow. */
gchar *characters_unihan_fold_text(const gchar *term);

/** Match a nonempty folded term against the selected immutable textual fields. */
gboolean characters_unihan_match_fields(gunichar point, guint32 fields, const gchar *term);

/** Match a positive numeric group in official data or labeled fallback. */
gboolean characters_unihan_match_group(gunichar point, gboolean radical, guint group);

/** Number of available radical or stroke filter slots. */
gsize characters_unihan_group_count(gboolean radical);

/** Borrow a stable group label, or NULL outside the available range. */
const gchar *characters_unihan_group_name(gboolean radical, gsize index);

/** Match a group index; -1 matches any known group in the selected classification. */
gboolean characters_unihan_group_matches(gunichar point, gboolean radical, gint index);

#endif
