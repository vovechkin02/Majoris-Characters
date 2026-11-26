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
        SPACE_SEPARATOR
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
}
