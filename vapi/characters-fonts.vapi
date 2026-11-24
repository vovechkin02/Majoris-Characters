/** Installed font discovery and exact cmap inspection, independent of GTK selection. */
[CCode (cheader_filename = "font-catalog.h")]
namespace Characters.Fonts {
    /** Owns one immutable Fontconfig snapshot. */
    [Compact]
    [CCode (cname = "CharactersFontCatalog", free_function = "characters_fonts_free",
        has_type_id = false)]
    public class Catalog {
        /** Number of usable installed faces. */
        [CCode (cname = "characters_fonts_count")]
        public size_t count ();

        /** Primary family label, borrowed until the snapshot is destroyed. */
        [CCode (cname = "characters_fonts_family")]
        public unowned string? family (size_t index);

        /** Primary style label, borrowed until the snapshot is destroyed. */
        [CCode (cname = "characters_fonts_style")]
        public unowned string? style (size_t index);

        /** Actual installed font path, borrowed until snapshot destruction. */
        [CCode (cname = "characters_fonts_file")]
        public unowned string? file (size_t index);

        /** Fontconfig face and named-instance index, or -1. */
        [CCode (cname = "characters_fonts_face_index")]
        public int face_index (size_t index);

        /** Owned Pango description for rendering a requested face. */
        [CCode (cname = "characters_fonts_description")]
        public string? description (size_t index);

        /** Whether the installed pattern advertises variable font support. */
        [CCode (cname = "characters_fonts_is_variable")]
        public bool is_variable (size_t index);

        /** Whether the installed pattern advertises color glyph support. */
        [CCode (cname = "characters_fonts_is_color")]
        public bool is_color (size_t index);

        /** Checks exact face coverage without fallback. */
        [CCode (cname = "characters_fonts_contains")]
        public bool contains (size_t index, uint32 point);

        /** Owned sorted mapped scalars, excluding surrogates and invalid code points. */
        [CCode (cname = "characters_fonts_points", array_length_type = "size_t")]
        public uint32[]? points (size_t index);

        /** Tests font discovery freshness on a worker. */
        [CCode (cname = "characters_fonts_is_current")]
        public bool is_current ();

        /** Applies the inventory's configuration on the GTK main context. */
        [CCode (cname = "characters_fonts_apply_config")]
        public bool apply_config (Pango.FontMap? map);

        /** Verifies a resolved renderer face against its file and named-instance identity. */
        [CCode (cname = "characters_fonts_matches")]
        public bool matches (size_t index, Pango.Font? font);
    }

    /** Loads a fresh catalog on a worker, returning null if discovery fails. */
    [CCode (cname = "characters_fonts_load")]
    public static Catalog? load ();
}
