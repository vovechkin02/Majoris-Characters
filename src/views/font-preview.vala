namespace Characters {
    /** Converts exact native glyph pixels to a GTK texture without font substitution. */
    internal sealed class FontPreview : Object {
        /** Returns a texture using the widget's current text color for monochrome glyphs. */
        public static Gdk.Texture? render (Fonts.Face face, uint32 code, bool symbol,
                                           int size_pt, Gtk.Widget widget) {
            var color = widget.get_color ();
            var size_px = (uint) (size_pt * 4 / 3);
            var pixels = face.render (code, symbol, size_px, (uint8) (color.red * 255),
                (uint8) (color.green * 255), (uint8) (color.blue * 255));
            if (pixels == null) {
                return null;
            }
            var side = (int) (size_px * 2);
            return new Gdk.MemoryTexture (side, side, Gdk.MemoryFormat.R8G8B8A8,
                pixels, (size_t) side * 4);
        }
    }
}
