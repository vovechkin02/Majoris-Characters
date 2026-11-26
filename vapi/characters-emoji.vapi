/** Internal emoji catalog bindings with exact sequence ownership. */
[CCode (cheader_filename = "emoji-catalog.h")]
namespace Characters.Emoji {
    /** Whether a scalar has standardized presentation selectors. */
    [CCode (cname = "characters_emoji_is_presentation_base")]
    public static bool is_presentation_base (uint32 point);

    /** Exact supplementary identity index, or SIZE_MAX when absent. */
    [CCode (cname = "characters_emoji_alias_index")]
    public static size_t alias_index (string text);

    /** Number of exact supplementary identities. */
    [CCode (cname = "characters_emoji_alias_count")]
    public static size_t alias_count ();

    /** Borrow the exact supplementary identity. */
    [CCode (cname = "characters_emoji_alias_text")]
    public static unowned string? alias_text (size_t index);

    /** Fully qualified catalog index or SIZE_MAX if no canonical entry exists. */
    [CCode (cname = "characters_emoji_alias_canonical")]
    public static size_t alias_canonical (size_t index);

    /** Borrow the associated catalog or presentation label. */
    [CCode (cname = "characters_emoji_alias_label")]
    public static unowned string? alias_label (size_t index);

    /** Borrow the annotation database provenance, independent of Unicode properties. */
    [CCode (cname = "characters_emoji_annotation_source")]
    public static unowned string annotation_source ();

    /** Check whether a sequence is a known emoji or supported presentation variant. */
    [CCode (cname = "characters_emoji_is_candidate")]
    public static bool is_candidate (string text);
}
