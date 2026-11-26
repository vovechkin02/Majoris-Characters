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

        /** Stable converter index used for explicit-length operations. */
        [CCode (cname = "characters_legacy_encoding_index")]
        public size_t encoding_index ();

        /** Checks membership without allocating or reconstructing the repertoire. */
        [CCode (cname = "characters_legacy_contains")]
        public bool contains (uint32 point);

        /** Owned sorted packed independent byte alternatives, or null. */
        [CCode (cname = "characters_legacy_keys", array_length_type = "size_t")]
        public uint64[]? keys (uint32 point);
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

    /** Strict ICU decoding from an exact byte array, including zero bytes. */
    [CCode (cname = "characters_legacy_decode_raw", array_length_type = "size_t")]
    public static uint32[]? decode_raw (size_t encoding,
        [CCode (array_length_type = "size_t")] uint8[] bytes);

    /** Strict ICU encoding; the caller must validate exact reverse conversion. */
    [CCode (cname = "characters_legacy_encode_raw", array_length_type = "size_t")]
    public static uint8[]? encode_raw (size_t encoding,
        [CCode (array_length_type = "size_t")] uint32[] points);

    /** Builds a strict mapping table off the GTK main context. */
    [CCode (cname = "characters_legacy_load")]
    public static Table? load (size_t index);
}
