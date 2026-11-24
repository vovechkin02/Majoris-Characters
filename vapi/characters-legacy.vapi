/** Strict bounded legacy encoding mappings, backed by ICU. */
[CCode (cheader_filename = "legacy-encoding.h")]
namespace Characters.Legacy {
    /** Owns a complete immutable mapping table for one supported encoding. */
    [Compact]
    [CCode (cname = "CharactersLegacy", free_function = "characters_legacy_free",
        has_type_id = false)]
    public class Table {
        /** Owned sorted unique scalars; null on failure. */
        [CCode (cname = "characters_legacy_points", array_length_type = "size_t")]
        public uint32[]? points ();

        /** Strict hexadecimal byte lookup, with UINT32_MAX for invalid/unmapped input. */
        [CCode (cname = "characters_legacy_decode")]
        public uint32 decode (string hex);

        /** Owned hexadecimal byte alternatives, or null when not mapped. */
        [CCode (cname = "characters_legacy_bytes")]
        public string? bytes (uint32 point);
    }

    /** Number of supported one- and two-byte encodings. */
    [CCode (cname = "characters_legacy_count")]
    public static size_t count ();

    /** Borrowed stable encoding label, or null for an invalid slot. */
    [CCode (cname = "characters_legacy_name")]
    public static unowned string? name (size_t index);

    /** Builds a strict mapping table off the GTK main context. */
    [CCode (cname = "characters_legacy_load")]
    public static Table? load (size_t index);
}
