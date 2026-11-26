#ifndef CHARACTERS_SCALAR_PROPERTIES_H
#define CHARACTERS_SCALAR_PROPERTIES_H

#include <glib.h>

/** Return the Unicode 17 general category in ICU UCharCategory numeric order. */
gint characters_scalar_category(gunichar point);

/** Return the Unicode 17 decomposition type in ICU UDecompositionType order. */
gint characters_scalar_decomposition_type(gunichar point);

/** Return the canonical combining class; zero for invalid code points. */
guint characters_scalar_combining_class(gunichar point);

/** Return the stable snapshot script ID or -1 for an invalid point. */
gint characters_scalar_script(gunichar point);

/** Return the number of primary script identifiers in the snapshot. */
gsize characters_scalar_script_count(void);

/** Return the borrowed long script name or NULL for an invalid index. */
const gchar *characters_scalar_script_name(gsize index);

/** Resolve a four-letter ISO 15924 code to the snapshot ID, or -1. */
gint characters_scalar_script_index(const gchar *code);

/** Match primary Script or any explicitly declared Script_Extensions member. */
gboolean characters_scalar_has_script(gunichar point, gint script);

/** Match a bounded mask of snapshot script IDs against primary or extension scripts. */
gboolean characters_scalar_matches_scripts(gunichar point, const gboolean *scripts, gsize length);

/** Return the borrowed snapshot Unicode version. */
const gchar *characters_scalar_version(void);

/** Return whether the scalar has the Unicode Default_Ignorable_Code_Point property. */
gboolean characters_scalar_is_ignorable(gunichar point);

/** Return a newly allocated algorithmic name or NULL when not applicable. */
gchar *characters_scalar_algorithmic_name(gunichar point);

/** Select an immutable standardized scalar property string. */
typedef enum {
    /** General category abbreviation. */
    CHARACTERS_SCALAR_TEXT_CATEGORY,
    /** ISO 15924 primary script code. */
    CHARACTERS_SCALAR_TEXT_SCRIPT,
    /** Space-delimited script extension codes. */
    CHARACTERS_SCALAR_TEXT_EXTENSIONS,
    /** Bidirectional class abbreviation. */
    CHARACTERS_SCALAR_TEXT_BIDI,
    /** Decomposition type abbreviation. */
    CHARACTERS_SCALAR_TEXT_DECOMPOSITION,
    /** East Asian width abbreviation. */
    CHARACTERS_SCALAR_TEXT_WIDTH
} CharactersScalarText;

/** Borrow an assigned scalar's standardized value; NULL for invalid or absent data. */
const gchar *characters_scalar_text(gunichar point, CharactersScalarText field);

/** Return age encoded as major * 10 + minor, or -1 for an unassigned or invalid scalar. */
gint characters_scalar_age(gunichar point);

/** Return the four exposed Boolean property bits, or zero for an invalid scalar. */
guint characters_scalar_boolean_flags(gunichar point);

/** Borrow the canonical key of a supported ICU property alias, or NULL. */
const gchar *characters_scalar_property_alias(const gchar *property);

#endif
