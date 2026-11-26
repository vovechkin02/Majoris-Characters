/** Length-aware ICU operations used by Vala domain services. */
[CCode (cheader_filename = "icu-adapter.h")]
namespace Characters.Icu {
    /** Count clusters after the caller validates the bounded UTF-8 input. */
    [CCode (cname = "characters_icu_graphemes")]
    public static int graphemes (string text);

    /** Write a direct decomposition into a caller-owned array of 32 scalars. */
    [CCode (cname = "characters_icu_decomposition")]
    public static size_t decomposition (uint32 point,
        [CCode (array_length = false)] uint32[] points);

    /** Pinned Unicode 17 simple uppercase mapping. */
    [CCode (cname = "characters_icu_uppercase")]
    public static uint32 uppercase (uint32 point);

    /** Pinned Unicode 17 simple lowercase mapping. */
    [CCode (cname = "characters_icu_lowercase")]
    public static uint32 lowercase (uint32 point);

    /** Pinned Unicode 17 simple titlecase mapping. */
    [CCode (cname = "characters_icu_titlecase")]
    public static uint32 titlecase (uint32 point);

    /** ICU bidirectional mirror mapping. */
    [CCode (cname = "characters_icu_mirror")]
    public static uint32 mirror (uint32 point);

    /** Write recursive canonical decomposition into a caller-owned 32-scalar array. */
    [CCode (cname = "characters_icu_canonical_decomposition")]
    public static size_t canonical_decomposition (uint32 point,
        [CCode (array_length = false)] uint32[] points);

    /** Return an owned scalar array with explicit length, including embedded NUL. */
    [CCode (cname = "characters_icu_normalize_points", array_length_type = "gint")]
    public static uint32[]? normalize_points (uint32[] points, string form);
}

/** Read-only scalar property access for preview and decomposition policies. */
[CCode (cheader_filename = "scalar-properties.h")]
namespace Characters.Scalar {
    /** ICU general category values used for safe previews. */
    [CCode (cname = "int", has_type_id = false, cprefix = "U_",
        cheader_filename = "unicode/uchar.h")]
    public enum Category {
        /** Reserved scalar. */
        UNASSIGNED,
        /** Nonspacing combining mark. */
        NON_SPACING_MARK,
        /** Spacing combining mark. */
        COMBINING_SPACING_MARK,
        /** Enclosing mark. */
        ENCLOSING_MARK,
        /** Control scalar. */
        CONTROL_CHAR,
        /** Formatting scalar. */
        FORMAT_CHAR,
        /** Line separator. */
        LINE_SEPARATOR,
        /** Paragraph separator. */
        PARAGRAPH_SEPARATOR,
        /** Space separator. */
        SPACE_SEPARATOR,
        /** Private-use scalar. */
        PRIVATE_USE_CHAR
    }

    /** Return the Unicode 17 category rather than the system GLib category. */
    [CCode (cname = "characters_scalar_category")]
    public static Category category (uint32 point);

    /** Return whether the scalar has Default_Ignorable_Code_Point. */
    [CCode (cname = "characters_scalar_is_ignorable")]
    public static bool is_ignorable (uint32 point);

    /** Return the Unicode 17 decomposition type, with zero denoting no mapping. */
    [CCode (cname = "characters_scalar_decomposition_type")]
    public static int decomposition_type (uint32 point);

    /** Return the primary script ID from the Unicode 17 snapshot, or -1 if invalid. */
    [CCode (cname = "characters_scalar_script")]
    public static int script (uint32 point);

    /** Number of snapshot script slots. */
    [CCode (cname = "characters_scalar_script_count")]
    public static size_t script_count ();

    /** Borrow the long script name from the bundled snapshot. */
    [CCode (cname = "characters_scalar_script_name")]
    public static unowned string? script_name (size_t index);

    /** Stable selector for borrowed property abbreviations. */
    [CCode (cname = "CharactersScalarText", cprefix = "CHARACTERS_SCALAR_TEXT_",
        has_type_id = false)]
    public enum Text {
        /** General category abbreviation. */
        CATEGORY,
        /** ISO 15924 primary script code. */
        SCRIPT,
        /** Space-delimited script extension codes. */
        EXTENSIONS,
        /** Bidirectional class abbreviation. */
        BIDI,
        /** Decomposition type abbreviation. */
        DECOMPOSITION,
        /** East Asian width abbreviation. */
        WIDTH
    }

    /** Borrow the standardized value for an assigned scalar. */
    [CCode (cname = "characters_scalar_text")]
    public static unowned string? text (uint32 point, Text field);

    /** Return the raw encoded age or -1 for absent data. */
    [CCode (cname = "characters_scalar_age")]
    public static int age (uint32 point);

    /** Return the raw canonical combining class. */
    [CCode (cname = "characters_scalar_combining_class")]
    public static uint combining_class (uint32 point);

    /** Return the four standardized Boolean bits in property order. */
    [CCode (cname = "characters_scalar_boolean_flags")]
    public static uint boolean_flags (uint32 point);

    /** Borrow a supported canonical ICU alias key, or null. */
    [CCode (cname = "characters_scalar_property_alias")]
    public static unowned string? property_alias (string property);
}
