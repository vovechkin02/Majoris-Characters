namespace Characters {
    /** Recyclable grid cell; its safe display is never used as the character identity. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/character-tile.ui")]
    internal sealed class CharacterTile : Gtk.Box {
        /** Rendered glyph or a visible placeholder for a nonprinting scalar. */
        [GtkChild]
        private unowned Gtk.Label glyph_label;
        /** Code point caption, visually shortened for long sequences. */
        [GtkChild]
        private unowned Gtk.Label code_label;

        /** Replaces all data when GTK binds this recycled cell to a new identity. */
        public void bind_id (uint32 id) {
            var code = Search.code (id) ?? "";
            var name = Search.name (id) ?? _("Unknown Character");
            bind_preview (id, null, 24, true, code, "%s\n%s".printf (name, code));
        }

        /** Renders a scalar using typed Pango attributes, never markup or injected font labels. */
        public void bind_preview (uint32 id, string? description, int size_pt, bool fallback,
                                  string caption, string detail, bool wide_caption = false) {
            var attributes = new Pango.AttrList ();
            if (description != null) {
                var font = Pango.FontDescription.from_string (description);
                attributes.insert (Pango.attr_font_desc_new (font));
            }
            attributes.insert (new Pango.AttrSize (size_pt * Pango.SCALE));
            attributes.insert (Pango.attr_fallback_new (fallback));
            glyph_label.attributes = attributes;
            glyph_label.set_text (Search.display (id) ?? "");
            code_label.max_width_chars = wide_caption ? 24 : 10;
            width_request = wide_caption ? 176 : 72;
            height_request = size_pt > 24 ? size_pt * 2 + 16 : 64;
            code_label.set_text (caption);
            tooltip_text = detail;
            update_property (Gtk.AccessibleProperty.LABEL, detail);
        }

        /** Clears recycled content so an unbound cell cannot expose a stale character. */
        public void clear () {
            glyph_label.set_text ("");
            glyph_label.attributes = null;
            code_label.label = "";
            tooltip_text = null;
            reset_property (Gtk.AccessibleProperty.LABEL);
        }
    }
}
