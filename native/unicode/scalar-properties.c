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

/** Borrow an assigned scalar's standardized value without allocating. */
const gchar *characters_scalar_text(gunichar point, CharactersScalarText field)
{
    const struct PropertyRange *row = lookup(point);
    if (!row || !row->category) {
        return NULL;
    }
    switch (field) {
        case CHARACTERS_SCALAR_TEXT_CATEGORY:
            return category_names[row->category];
        case CHARACTERS_SCALAR_TEXT_SCRIPT:
            return script_entries[row->script].code;
        case CHARACTERS_SCALAR_TEXT_EXTENSIONS:
            return row->extensions ? extension_sets[row->extensions] :
            script_entries[row->script].code;
        case CHARACTERS_SCALAR_TEXT_BIDI:
            return bc_names[row->bidi];
        case CHARACTERS_SCALAR_TEXT_DECOMPOSITION:
            return dt_names[row->decomposition];
        case CHARACTERS_SCALAR_TEXT_WIDTH:
            return ea_names[row->width];
        default:
            return NULL;
    }
}

/** Return the raw age number without formatting. */
gint characters_scalar_age(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    return row && row->category ? row->age : -1;
}

/** Return Bidi_Mirrored, Emoji, Emoji_Presentation and Emoji_Modifier in bits 0–3. */
guint characters_scalar_boolean_flags(gunichar point)
{
    const struct PropertyRange *row = lookup(point);
    return row && row->category ? row->flags & 0xFu : 0;
}

/** Resolve only the ICU aliases historically supported by the metadata interface. */
const gchar *characters_scalar_property_alias(const gchar *property)
{
    if (!property) {
        return NULL;
    }
    switch (u_getPropertyEnum(property)) {
        case UCHAR_GENERAL_CATEGORY:
            return "General_Category";
        case UCHAR_SCRIPT:
            return "Script";
        case UCHAR_BIDI_CLASS:
            return "Bidi_Class";
        case UCHAR_BIDI_MIRRORED:
            return "Bidi_Mirrored";
        case UCHAR_DECOMPOSITION_TYPE:
            return "Decomposition_Type";
        case UCHAR_EAST_ASIAN_WIDTH:
            return "East_Asian_Width";
        case UCHAR_EMOJI:
            return "Emoji";
        case UCHAR_EMOJI_PRESENTATION:
            return "Emoji_Presentation";
        case UCHAR_EMOJI_MODIFIER:
            return "Emoji_Modifier";
        default:
            return NULL;
    }
}
