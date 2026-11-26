#include "scalar-properties.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <unicode/uchar.h>

#include "../allocation.h"
#include "../scalar-validity.h"


/** An ISO 15924 code and its standardized long script name. */
struct ScriptEntry {
    const gchar *code;
    const gchar *name;
};

/** One inclusive range sharing all exposed scalar property values. */
struct PropertyRange {
    guint32 start;
    guint32 end;
    guint8 category;
    guint8 script;
    guint16 extensions;
    guint8 age;
    guint8 combining;
    guint8 bidi;
    guint8 decomposition;
    guint8 width;
    guint8 flags;
};

#define N_(message) (message)
#include "generated/properties.h"

/** Binary search comparator for inclusive scalar ranges. */
static gint compare_range(gconstpointer key, gconstpointer item)
{
    gunichar point = *(const guint32 *)key;
    const struct PropertyRange *range = item;
    return (point > range->end) - (point < range->start);
}

/** Borrow the immutable property row for a valid scalar. */
static const struct PropertyRange *lookup(gunichar point)
{
    if (!characters_scalar_is_valid(point)) {
        return NULL;
    }
    return bsearch(&point, property_ranges, G_N_ELEMENTS(property_ranges),
                   sizeof(property_ranges[0]), compare_range);
}

/** Return the Unicode 17 general category in ICU UCharCategory numeric order. */
gint characters_scalar_category(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    return row ? row->category : U_UNASSIGNED;
}

/** Return the canonical combining class; zero for invalid code points. */
guint characters_scalar_combining_class(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    return row ? row->combining : 0;
}

/** Return the stable snapshot script ID or -1 for an invalid point. */
gint characters_scalar_script(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    return row ? row->script : -1;
}

/** Return the number of primary script identifiers in the snapshot. */
gsize characters_scalar_script_count(void)
{
    return G_N_ELEMENTS(script_entries);
}

/** Return the borrowed long script name or NULL for an invalid index. */
const gchar *characters_scalar_script_name(gsize index)
{
    return index < G_N_ELEMENTS(script_entries) ? script_entries[index].name : NULL;
}

/** Resolve a four-letter ISO 15924 code to its snapshot ID. */
gint characters_scalar_script_index(const gchar *code)
{
    if (!code) {
        return -1;
    }
    for (gsize index = 0; index < G_N_ELEMENTS(script_entries); index++) {
        if (strcmp(code, script_entries[index].code) == 0) {
            return (gint)index;
        }
    }
    return -1;
}

/** Match primary Script or a space-delimited four-letter extension code. */
gboolean characters_scalar_has_script(gunichar point, gint script)
{
    const struct PropertyRange *row = lookup(point);
    if (!row || script < 0 || (gsize)script >= G_N_ELEMENTS(script_entries)) {
        return FALSE;
    }
    if (row->script == script) {
        return TRUE;
    }
    const gchar *extensions = extension_sets[row->extensions];
    if (extensions) {
        for (const gchar *code = extensions; *code; code += 5) {
            if (strncmp(code, script_entries[script].code, 4) == 0) {
                return TRUE;
            }
            if (!code[4]) {
                break;
            }
        }
    }
    return FALSE;
}

/** Match one of the selected script IDs without allocating property strings. */
gboolean characters_scalar_matches_scripts(gunichar point, const gboolean *scripts, gsize length)
{
    const struct PropertyRange *row = lookup(point);
    if (!row || !scripts) {
        return FALSE;
    }
    if (row->script < length && scripts[row->script]) {
        return TRUE;
    }
    const gchar *extensions = extension_sets[row->extensions];
    if (extensions) {
        for (const gchar *code = extensions; *code; code += 5) {
            gchar tag[5];
            memcpy(tag, code, 4);
            tag[4] = '\0';
            gint index = characters_scalar_script_index(tag);
            if (index >= 0 && (gsize)index < length && scripts[index]) {
                return TRUE;
            }
            if (!code[4]) {
                break;
            }
        }
    }
    return FALSE;
}

/** Return the borrowed snapshot Unicode version. */
const gchar *characters_scalar_version(void)
{
    return PROPERTY_VERSION;
}

/** Return whether the scalar has the Unicode Default_Ignorable_Code_Point property. */
gboolean characters_scalar_is_ignorable(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    return row && (row->flags & (1u << 4));
}

/** Return the Unicode 17 decomposition type without allocating a property string. */
gint characters_scalar_decomposition_type(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    return row ? row->decomposition : U_DT_NONE;
}

/** Return a newly allocated algorithmic name or NULL when not applicable. */
gchar *characters_scalar_algorithmic_name(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    if (!row || !row->category) {
        return NULL;
    }
    const gchar *prefix = NULL;
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
    gchar text[64];
    g_snprintf(text, sizeof(text), "%s-%04X", prefix, (guint)point);
    return characters_try_strdup(text);
}

/** Return a newly allocated property value or NULL for an unsupported query. */
gchar *characters_scalar_property(gunichar point, const gchar *property)
{
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
    const gchar *value = NULL;
    gchar number[8];
    if (strcmp(property, "Age") == 0) {
        g_snprintf(number, sizeof(number), "%u.%u", row->age / 10u, row->age % 10u);
        value = number;
    } else if (strcmp(property, "General_Category") == 0) {
        value = category_names[row->category];
    } else if (strcmp(property, "Script") == 0) {
        value = script_entries[row->script].code;
    } else if (strcmp(property, "Script_Extensions") == 0) {
        value = row->extensions ? extension_sets[row->extensions] :
                script_entries[row->script].code;
    } else if (strcmp(property, "Canonical_Combining_Class") == 0) {
        g_snprintf(number, sizeof(number), "%u", row->combining);
        value = number;
    } else if (strcmp(property, "Bidi_Class") == 0) {
        value = bc_names[row->bidi];
    } else if (strcmp(property, "Decomposition_Type") == 0) {
        value = dt_names[row->decomposition];
    } else if (strcmp(property, "East_Asian_Width") == 0) {
        value = ea_names[row->width];
    } else {
        static const gchar *const flags[] = {
            "Bidi_Mirrored", "Emoji", "Emoji_Presentation", "Emoji_Modifier"
        };
        for (gsize index = 0; index < G_N_ELEMENTS(flags); index++) {
            if (strcmp(property, flags[index]) == 0) {
                value = row->flags & (1u << index) ? "Y" : "N";
                break;
            }
        }
    }
    return value ? characters_try_strdup(value) : NULL;
}
