#include "scalar-properties.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/uchar.h>

#include "../scalar-validity.h"

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))

/** An ISO 15924 code and its standardized long script name. */
struct ScriptEntry {
    const char *code;
    const char *name;
};

/** One inclusive range sharing all exposed scalar property values. */
struct PropertyRange {
    uint32_t start;
    uint32_t end;
    uint8_t category;
    uint8_t script;
    uint16_t extensions;
    uint8_t age;
    uint8_t combining;
    uint8_t bidi;
    uint8_t decomposition;
    uint8_t width;
    uint8_t flags;
};

#define N_(message) (message)
#include "generated/properties.h"

/** Binary search comparator for inclusive scalar ranges. */
static int compare_range(const void *key, const void *item) {
    uint32_t point = *(const uint32_t *)key;
    const struct PropertyRange *range = item;
    return (point > range->end) - (point < range->start);
}

/** Borrow the immutable property row for a valid scalar. */
static const struct PropertyRange *lookup(uint32_t point) {
    if (!characters_scalar_is_valid(point)) {
        return NULL;
    }
    return bsearch(&point, property_ranges, COUNT(property_ranges),
                   sizeof(property_ranges[0]), compare_range);
}

/** Return the Unicode 17 general category in ICU UCharCategory numeric order. */
int characters_scalar_category(uint32_t point) {
    const struct PropertyRange *row = lookup(point);
    return row ? row->category : U_UNASSIGNED;
}

/** Return the canonical combining class; zero for invalid code points. */
unsigned characters_scalar_combining_class(uint32_t point) {
    const struct PropertyRange *row = lookup(point);
    return row ? row->combining : 0;
}

/** Return the stable snapshot script ID or -1 for an invalid point. */
int characters_scalar_script(uint32_t point) {
    const struct PropertyRange *row = lookup(point);
    return row ? row->script : -1;
}

/** Return the number of primary script identifiers in the snapshot. */
size_t characters_scalar_script_count(void) {
    return COUNT(script_entries);
}

/** Return the borrowed long script name or NULL for an invalid index. */
const char *characters_scalar_script_name(size_t index) {
    return index < COUNT(script_entries) ? script_entries[index].name : NULL;
}

/** Resolve a four-letter ISO 15924 code to its snapshot ID. */
int characters_scalar_script_index(const char *code) {
    if (!code) {
        return -1;
    }
    for (size_t index = 0; index < COUNT(script_entries); index++) {
        if (strcmp(code, script_entries[index].code) == 0) {
            return (int)index;
        }
    }
    return -1;
}

/** Match primary Script or a space-delimited four-letter extension code. */
bool characters_scalar_has_script(uint32_t point, int script) {
    const struct PropertyRange *row = lookup(point);
    if (!row || script < 0 || (size_t)script >= COUNT(script_entries)) {
        return false;
    }
    if (row->script == script) {
        return true;
    }
    const char *extensions = extension_sets[row->extensions];
    if (extensions) {
        for (const char *code = extensions; *code; code += 5) {
            if (strncmp(code, script_entries[script].code, 4) == 0) {
                return true;
            }
            if (!code[4]) {
                break;
            }
        }
    }
    return false;
}

/** Match one of the selected script IDs without allocating property strings. */
bool characters_scalar_matches_scripts(uint32_t point, const bool *scripts, size_t length) {
    const struct PropertyRange *row = lookup(point);
    if (!row || !scripts) {
        return false;
    }
    if (row->script < length && scripts[row->script]) {
        return true;
    }
    const char *extensions = extension_sets[row->extensions];
    if (extensions) {
        for (const char *code = extensions; *code; code += 5) {
            char tag[5];
            memcpy(tag, code, 4);
            tag[4] = '\0';
            int index = characters_scalar_script_index(tag);
            if (index >= 0 && (size_t)index < length && scripts[index]) {
                return true;
            }
            if (!code[4]) {
                break;
            }
        }
    }
    return false;
}

/** Return the borrowed snapshot Unicode version. */
const char *characters_scalar_version(void) {
    return PROPERTY_VERSION;
}

/** Return whether the scalar has the Unicode Default_Ignorable_Code_Point property. */
bool characters_scalar_is_ignorable(uint32_t point) {
    const struct PropertyRange *row = lookup(point);
    return row && (row->flags & (1u << 4));
}

/** Return the Unicode 17 decomposition type without allocating a property string. */
int characters_scalar_decomposition_type(uint32_t point) {
    const struct PropertyRange *row = lookup(point);
    return row ? row->decomposition : U_DT_NONE;
}

/** Return a newly allocated algorithmic name or NULL when not applicable. */
char *characters_scalar_algorithmic_name(uint32_t point) {
    const struct PropertyRange *row = lookup(point);
    if (!row || !row->category) {
        return NULL;
    }
    const char *prefix = NULL;
    if (row->flags & (1u << 5)) {
        prefix = "CJK UNIFIED IDEOGRAPH";
    } else if (strcmp(script_entries[row->script].code, "Tang") == 0 &&
               ((point >= 0x17000 && point <= 0x187FF) ||
                (point >= 0x18D00 && point <= 0x18D7F))) {
        prefix = "TANGUT IDEOGRAPH";
    }
    if (!prefix) {
        return NULL;
    }
    char text[64];
    snprintf(text, sizeof(text), "%s-%04X", prefix, (unsigned)point);
    return strdup(text);
}

/** Return a newly allocated property value or NULL for an unsupported query. */
char *characters_scalar_property(uint32_t point, const char *property) {
    const struct PropertyRange *row = lookup(point);
    if (!row || !row->category || !property) {
        return NULL;
    }
    /* The earlier engine accepted ICU aliases for its enumerated properties. */
    switch (u_getPropertyEnum(property)) {
        case UCHAR_GENERAL_CATEGORY: property = "General_Category"; break;
        case UCHAR_SCRIPT: property = "Script"; break;
        case UCHAR_BIDI_CLASS: property = "Bidi_Class"; break;
        case UCHAR_BIDI_MIRRORED: property = "Bidi_Mirrored"; break;
        case UCHAR_DECOMPOSITION_TYPE: property = "Decomposition_Type"; break;
        case UCHAR_EAST_ASIAN_WIDTH: property = "East_Asian_Width"; break;
        case UCHAR_EMOJI: property = "Emoji"; break;
        case UCHAR_EMOJI_PRESENTATION: property = "Emoji_Presentation"; break;
        case UCHAR_EMOJI_MODIFIER: property = "Emoji_Modifier"; break;
        default: break;
    }
    const char *value = NULL;
    char number[8];
    if (strcmp(property, "Age") == 0) {
        snprintf(number, sizeof(number), "%u.%u", row->age / 10u, row->age % 10u);
        value = number;
    } else if (strcmp(property, "General_Category") == 0) {
        value = category_names[row->category];
    } else if (strcmp(property, "Script") == 0) {
        value = script_entries[row->script].code;
    } else if (strcmp(property, "Script_Extensions") == 0) {
        value = row->extensions ? extension_sets[row->extensions] :
                script_entries[row->script].code;
    } else if (strcmp(property, "Canonical_Combining_Class") == 0) {
        snprintf(number, sizeof(number), "%u", row->combining);
        value = number;
    } else if (strcmp(property, "Bidi_Class") == 0) {
        value = bc_names[row->bidi];
    } else if (strcmp(property, "Decomposition_Type") == 0) {
        value = dt_names[row->decomposition];
    } else if (strcmp(property, "East_Asian_Width") == 0) {
        value = ea_names[row->width];
    } else {
        static const char *const flags[] = {
            "Bidi_Mirrored", "Emoji", "Emoji_Presentation", "Emoji_Modifier"
        };
        for (size_t index = 0; index < COUNT(flags); index++) {
            if (strcmp(property, flags[index]) == 0) {
                value = row->flags & (1u << index) ? "Y" : "N";
                break;
            }
        }
    }
    return value ? strdup(value) : NULL;
}
