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

/** Return a newly allocated property value or NULL for an unsupported query. */
gchar *characters_scalar_property(gunichar point, const gchar *property);

#endif
