/** Internal native Unicode engine bindings; no installed ABI is promised. */
[CCode (cheader_filename = "unicode-engine.h", cprefix = "characters_unicode_")]
namespace Characters.Unicode {
    /** Whether @point is within Unicode and not a surrogate. */
    [CCode (cname = "characters_unicode_is_scalar")]
    public static bool is_scalar (uint32 point);

    /**
     * Whether the Unicode 17 property snapshot assigns the scalar.
     * Private-use scalars count as assigned.
     */
    [CCode (cname = "characters_unicode_is_assigned")]
    public static bool is_assigned (uint32 point);

    /** The Unicode version of the bundled scalar property snapshot. */
    [CCode (cname = "characters_unicode_properties_version")]
    public static string? properties_version ();

    /** The immutable GNOME-derived catalog version. */
    [CCode (cname = "characters_unicode_catalog_version")]
    public static unowned string catalog_version ();

    /** The immutable alias and named-sequence data version. */
    [CCode (cname = "characters_unicode_supplement_version")]
    public static unowned string supplement_version ();

    /** Official Unihan release, or "none" for the separately attributed fallback. */
    [CCode (cname = "characters_unicode_unihan_version")]
    public static unowned string unihan_version ();

    /**
     * Canonical name, or NULL when unavailable. Controls, private-use characters and unnamed
     * scalars do not have canonical names.
     */
    [CCode (cname = "characters_unicode_name")]
    public static string? name (uint32 point);

    /**
     * Semicolon-separated official aliases, or NULL when none are available. No alias is
     * represented as a canonical name.
     */
    [CCode (cname = "characters_unicode_aliases")]
    public static string? aliases (uint32 point);

    /** Stable script name for an ISO 15924 code, or null for an unknown code. */
    [CCode (cname = "characters_unicode_script_name")]
    public static unowned string? script_name (string code);

    /**
     * Property value, or NULL for unsupported queries, invalid scalars, or scalars not assigned in
     * the scalar property snapshot. Plane is available for all valid scalars.
     */
    [CCode (cname = "characters_unicode_property")]
    public static string? get_property_value (uint32 point, string property);

    /** Number of blocks in the independently versioned catalog. */
    [CCode (cname = "characters_unicode_block_count")]
    public static size_t block_count ();

    /** Catalog block name, or NULL out of range. */
    [CCode (cname = "characters_unicode_block_name")]
    public static unowned string? block_name (size_t index);

    /** Inclusive start or UINT32_MAX out of range. */
    [CCode (cname = "characters_unicode_block_start")]
    public static uint32 block_start (size_t index);

    /** Inclusive end or UINT32_MAX out of range. */
    [CCode (cname = "characters_unicode_block_end")]
    public static uint32 block_end (size_t index);

    /** Block name or NULL outside a defined block. */
    [CCode (cname = "characters_unicode_find_block")]
    public static unowned string? find_block (uint32 point);

    /**
     * UTF-8 text, or NULL for invalid input, embedded NUL or allocation failure. No normalization
     * is performed.
     */
    [CCode (cname = "characters_unicode_encode")]
    public static string? encode ([CCode (array_length_type = "size_t")] uint32[] points);

    /**
     * Canonical scalar name, named sequence name or emoji label; NULL for unknown sequences.
     * Scalar emoji labels are available separately through the emoji catalog.
     */
    [CCode (cname = "characters_unicode_sequence_name")]
    public static string? sequence_name ([CCode (array_length_type = "size_t")] uint32[] points);

    /**
     * Normalized UTF-8, or NULL for invalid text, form, size or allocation failure. The original
     * text is not modified.
     */
    [CCode (cname = "characters_unicode_normalize")]
    public static string? normalize (string text, string form);

    /** Number of semantic emoji groups. */
    [CCode (cname = "characters_unicode_emoji_group_count")]
    public static size_t emoji_group_count ();

    /** Stable English group name or NULL. */
    [CCode (cname = "characters_unicode_emoji_group_name")]
    public static unowned string? emoji_group_name (size_t group);

    /** Number of entries, zero for invalid group. */
    [CCode (cname = "characters_unicode_emoji_group_size")]
    public static size_t emoji_group_size (size_t group);

    /** Catalog index or SIZE_MAX for invalid arguments. */
    [CCode (cname = "characters_unicode_emoji_group_index")]
    public static size_t emoji_group_index (size_t group, size_t index);

    /** Exact emoji catalog index, or SIZE_MAX for unknown sequences. */
    [CCode (cname = "characters_unicode_emoji_index")]
    public static size_t emoji_index ([CCode (array_length_type = "size_t")] uint32[] points);

    /** Number of fully qualified emoji sequences in the catalog. */
    [CCode (cname = "characters_unicode_emoji_count")]
    public static size_t emoji_count ();

    /** Exact UTF-8 sequence or NULL. */
    [CCode (cname = "characters_unicode_emoji_text")]
    public static string? emoji_text (size_t index);

    /** Catalog label or NULL. */
    [CCode (cname = "characters_unicode_emoji_label")]
    public static unowned string? emoji_label (size_t index);

    /** Number of standardized named sequences in the supplement. */
    [CCode (cname = "characters_unicode_named_count")]
    public static size_t named_count ();

    /** Standardized name or NULL. */
    [CCode (cname = "characters_unicode_named_name")]
    public static unowned string? named_name (size_t index);

    /** Exact UTF-8 sequence or NULL. */
    [CCode (cname = "characters_unicode_named_text")]
    public static string? named_text (size_t index);

    /** Owned raw scalar candidates; the Vala relationship policy sorts and deduplicates. */
    [CCode (cname = "characters_unicode_related_scalars", array_length_type = "int")]
    public static uint32[]? related_scalars (uint32 point);
}
