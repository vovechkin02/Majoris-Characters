/** Internal emoji catalog bindings with exact sequence ownership. */
[CCode (cheader_filename = "emoji-catalog.h")]
namespace Characters.Emoji {
    /** Owns an immutable collection of exact UTF-8 variants. */
    [Compact]
    [CCode (cname = "CharactersEmojiVariants", free_function = "characters_emoji_variants_free",
        has_type_id = false)]
    public class Variants {
        /** Number of supported alternatives, including the selected identity. */
        [CCode (cname = "characters_emoji_variants_count")]
        public size_t count ();

        /** Borrowed exact copyable text, or null for an invalid position. */
        [CCode (cname = "characters_emoji_variants_text")]
        public unowned string? text (size_t index);

        /** Borrowed catalog label or presentation description. */
        [CCode (cname = "characters_emoji_variants_label")]
        public unowned string? label (size_t index);
    }

    /** Build alternatives for a catalog sequence or supported presentation variant. */
    [CCode (cname = "characters_emoji_variants_new")]
    public static Variants? create_variants (string text);

    /** Borrow the annotation database provenance, independent of Unicode properties. */
    [CCode (cname = "characters_emoji_annotation_source")]
    public static unowned string annotation_source ();

    /** Check whether a sequence is a known emoji or supported presentation variant. */
    [CCode (cname = "characters_emoji_is_candidate")]
    public static bool is_candidate (string text);
}
