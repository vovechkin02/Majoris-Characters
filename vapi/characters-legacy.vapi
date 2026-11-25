/** Strict bounded legacy encoding mappings, backed by ICU. */
[CCode (cheader_filename = "legacy-encoding.h")]
namespace Characters.Legacy {
    /** Owns an immutable scalar repertoire and independent mapping table for one supported encoding. */
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

        /** Owned independent byte alternatives or canonical contextual bytes, or null. */
        [CCode (cname = "characters_legacy_bytes")]
        public string? bytes (uint32 point);
    }

    /** Number of supported legacy encodings. */
    [CCode (cname = "characters_legacy_count")]
    public static size_t count ();

    /** Borrowed stable encoding label, or null for an invalid slot. */
    [CCode (cname = "characters_legacy_name")]
    public static unowned string? name (size_t index);

    /** True when the encoding uses a contextual, canonical standalone repertoire. */
    [CCode (cname = "characters_legacy_is_contextual")]
    public static bool is_contextual (size_t index);

    /** Decodes 1–128 exact bytes into 1–32 scalars, including NUL; null on failure. */
    [CCode (cname = "characters_legacy_decode_sequence", array_length_type = "size_t")]
    public static uint32[]? decode_sequence (size_t encoding, string hex);

    /** Returns canonical bytes only when strict decoding reproduces the exact input. */
    [CCode (cname = "characters_legacy_encode_sequence", array_length_type = "size_t")]
    public static string? encode_sequence (size_t encoding, uint32[] points);

    /** Builds a strict mapping table off the GTK main context. */
    [CCode (cname = "characters_legacy_load")]
    public static Table? load (size_t index);
}
