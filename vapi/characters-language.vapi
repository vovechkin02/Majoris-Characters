/** Immutable language registry and ICU locale-to-script adapter. */
[CCode (cheader_filename = "language-collections.h")]
namespace Characters.LanguageData {
    /** Borrow a named collection's locale identifier; special slots have no tag. */
    [CCode (cname = "characters_language_tag")]
    public static unowned string? tag (size_t index);

    /** Borrow space-separated ISO language codes for one exact XKB registry ID. */
    [CCode (cname = "characters_language_layout_languages")]
    public static unowned string? layout_languages (string id);

    /** OR ICU script IDs into a caller-owned 256-byte mask. */
    [CCode (cname = "characters_language_add_locale")]
    public static void add_locale (string locale,
        [CCode (array_length_type = "gsize")] uint8[] scripts);

    /** Number of stable language collection slots. */
    [CCode (cname = "characters_language_count")]
    public static size_t count ();

    /** Borrow the stable English language label at a slot. */
    [CCode (cname = "characters_language_name")]
    public static unowned string? name (size_t index);
}
