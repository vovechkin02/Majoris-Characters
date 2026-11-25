#ifndef CHARACTERS_SCALAR_PROPERTIES_H
#define CHARACTERS_SCALAR_PROPERTIES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Return the Unicode 17 general category in ICU UCharCategory numeric order. */
int characters_scalar_category(uint32_t point);

/** Return the Unicode 17 decomposition type in ICU UDecompositionType order. */
int characters_scalar_decomposition_type(uint32_t point);

/** Return the canonical combining class; zero for invalid code points. */
unsigned characters_scalar_combining_class(uint32_t point);

/** Return the stable snapshot script ID or -1 for an invalid point. */
int characters_scalar_script(uint32_t point);

/** Return the number of primary script identifiers in the snapshot. */
size_t characters_scalar_script_count(void);

/** Return the borrowed long script name or NULL for an invalid index. */
const char *characters_scalar_script_name(size_t index);

/** Resolve a four-letter ISO 15924 code to the snapshot ID, or -1. */
int characters_scalar_script_index(const char *code);

/** Match primary Script or any explicitly declared Script_Extensions member. */
bool characters_scalar_has_script(uint32_t point, int script);

/** Match a bounded mask of snapshot script IDs against primary or extension scripts. */
bool characters_scalar_matches_scripts(uint32_t point, const bool *scripts, size_t length);

/** Return the borrowed snapshot Unicode version. */
const char *characters_scalar_version(void);

/** Return whether the scalar has the Unicode Default_Ignorable_Code_Point property. */
bool characters_scalar_is_ignorable(uint32_t point);

/** Return a newly allocated algorithmic name or NULL when not applicable. */
char *characters_scalar_algorithmic_name(uint32_t point);

/** Return a newly allocated property value or NULL for an unsupported query. */
char *characters_scalar_property(uint32_t point, const char *property);

#endif
