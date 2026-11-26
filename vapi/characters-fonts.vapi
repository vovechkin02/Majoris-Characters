/** Installed font discovery and exact cmap inspection, independent of GTK selection. */
[CCode (cheader_filename = "font-catalog.h")]
namespace Characters.Fonts {
    /** Owns one immutable Fontconfig snapshot. */
    [Compact]
    [CCode (cname = "CharactersFontCatalog", free_function = "characters_fonts_free",
        has_type_id = false)]
    public class Catalog {
        /** Number of discovered patterns before application filtering and sorting. */
        [CCode (cname = "characters_fonts_count")]
        public size_t count ();

        /** Primary family label, borrowed until the snapshot is destroyed. */
        [CCode (cname = "characters_fonts_family")]
        public unowned string? family (size_t index);

        /** Raw style bytes, borrowed until snapshot destruction; validate before display. */
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
        [CCode (cname = "characters_fonts_points", array_length_type = "gsize")]
        public uint32[]? points (size_t index);

        /** Opens an independent face from its exact installed file and index. */
        [CCode (cname = "characters_fonts_open_face")]
        public Face? open_face (size_t index);

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

    /** Thread-confined exact FreeType face; owns its library and variation state. */
    [Compact]
    [CCode (cname = "CharactersFontFace", free_function = "characters_font_face_free",
        has_type_id = false)]
    public class Face {
        /** Number of adjustable axes. */
        [CCode (cname = "characters_font_face_axis_count")]
        public size_t axis_count ();

        /** Owned OpenType design-axis tag. */
        [CCode (cname = "characters_font_face_axis_tag")]
        public string? axis_tag (size_t index);

        /** Declared range, default and active coordinate of an axis. */
        [CCode (cname = "characters_font_face_axis_range")]
        public bool axis_range (size_t index, out double minimum, out double maximum,
                                out double default_value, out double current);

        /** Changes one coordinate while retaining all other axes. */
        [CCode (cname = "characters_font_face_set_axis")]
        public bool set_axis (size_t index, double value);

        /** Restores the originally opened face or named instance. */
        [CCode (cname = "characters_font_face_reset")]
        public bool reset ();

        /** Owned actual Unicode cmap coverage; empty for symbol-only faces. */
        [CCode (cname = "characters_font_face_unicode_points", array_length_type = "gsize")]
        public uint32[]? unicode_points ();

        /** Raw Microsoft Symbol code count, independent of Unicode coverage. */
        [CCode (cname = "characters_font_face_symbol_count")]
        public size_t symbol_count ();

        /** Raw mapped code by index; UINT32_MAX on invalid index. */
        [CCode (cname = "characters_font_face_symbol_code")]
        public uint32 symbol_code (size_t index);

        /** Actual mapped glyph index, zero for an absent mapping. */
        [CCode (cname = "characters_font_face_glyph")]
        public uint32 glyph (uint32 code, bool symbol);

        /** Owned straight RGBA square image, or null if exact rasterization fails. */
        [CCode (cname = "characters_font_face_render")]
        public GLib.Bytes? render (uint32 code, bool symbol, uint size_px,
                                   uint8 red, uint8 green, uint8 blue);
    }

    /** Opens an exact face from a native path; callers must supply a trusted installed/test path. */
    [CCode (cname = "characters_font_face_open")]
    public static Face? open_file (string file, int index);

    /** Loads a fresh catalog on a worker, returning null if discovery fails. */
    [CCode (cname = "characters_fonts_load")]
    public static Catalog? load ();
}
