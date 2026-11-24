/** Private bindings for the immutable native inspection snapshot. */
[CCode (cheader_filename = "character-inspector.h")]
namespace Characters.Inspector {
    /** Supported numeric representations; host languages may reinterpret some values. */
    [CCode (cname = "CharactersInspectorFormat", cprefix = "CHARACTERS_INSPECTOR_",
        has_type_id = false)]
    public enum Format {
        /** Unicode code point notation. */
        CODE,
        /** Decimal scalar values. */
        DECIMAL,
        /** UTF-8 bytes in hexadecimal. */
        UTF8,
        /** UTF-16 code units in hexadecimal. */
        UTF16,
        /** UTF-32 code units in hexadecimal. */
        UTF32,
        /** Decimal numeric character references. */
        HTML_DECIMAL,
        /** Hexadecimal numeric character references. */
        HTML_HEX,
        /** Fixed-width Unicode escapes (C and Python style). */
        ESCAPE,
        /** CSS hexadecimal escapes with terminating spaces. */
        CSS,
    }

    /** Owns one independent sequence snapshot; all access occurs on the GTK main context. */
    [Compact]
    [CCode (cname = "CharactersInspector", free_function = "characters_inspector_free",
        has_type_id = false)]
    public class Snapshot {
        /** Number of constituent scalars, or zero for NULL. */
        [CCode (cname = "characters_inspector_count")]
        public size_t count ();

        /** Scalar or UINT32_MAX for an invalid position. */
        [CCode (cname = "characters_inspector_point")]
        public uint32 point (size_t index);

        /** Canonical scalar or catalog sequence name, or NULL when unavailable. */
        [CCode (cname = "characters_inspector_name")]
        public string? name ();

        /** Exact UTF-8 text, or NULL for embedded NUL or allocation failure. */
        [CCode (cname = "characters_inspector_text")]
        public string? text ();

        /** Safe preview, never the source for copying, or NULL on failure. */
        [CCode (cname = "characters_inspector_display")]
        public string? display ();

        /** Complete representation or NULL for an invalid format or failure. */
        [CCode (cname = "characters_inspector_format")]
        public string? format (Format format);

        /**
         * Normalized scalar notation, including NUL, or null when properties are unavailable
         * or normalization fails.
         */
        [CCode (cname = "characters_inspector_normalize")]
        public string? normalize (string form);

        /** Owned alternatives, or null when unavailable; Vala releases the returned vector. */
        [CCode (cname = "characters_inspector_related", array_length = false,
            array_null_terminated = true)]
        public string[]? related ();
    }

    /** Opaque browsing identity. Snapshot or NULL for an invalid identity or allocation failure. */
    [CCode (cname = "characters_inspector_from_id")]
    public static Snapshot? from_id (uint32 id);

    /**
     * Creates a snapshot of 1 to 32 scalars from exact UTF-8, or null for invalid,
     * empty or oversized input or allocation failure.
     */
    [CCode (cname = "characters_inspector_from_text")]
    public static Snapshot? from_text (string text);

    /**
     * Creates a snapshot from explicit U+, 0x, or dec: notation for 1 to 32 scalars,
     * or null for invalid input or allocation failure.
     */
    [CCode (cname = "characters_inspector_from_code")]
    public static Snapshot? from_code (string code);
}
