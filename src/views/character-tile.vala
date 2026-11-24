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

        /** Exact bound text; cleared when GTK unbinds the recycled tile. */
        private string? drag_text;

        /** Provides copy-only plain-text drag data, captured at drag preparation. */
        construct {
            var source = new Gtk.DragSource ();
            source.actions = Gdk.DragAction.COPY;
            source.prepare.connect (prepare_drag);
            add_controller (source);
        }

        /** Returns immutable text content, or null when no copyable identity is bound. */
        private Gdk.ContentProvider? prepare_drag (double x, double y) {
            if (drag_text == null) {
                return null;
            }
            var value = Value (typeof (string));
            value.set_string (drag_text);
            return new Gdk.ContentProvider.for_value (value);
        }

        /** Replaces all data when GTK binds this recycled cell to a new identity. */
        public void bind_id (uint32 id, int size_pt = 24) {
            var code = Search.code (id) ?? "";
            var name = Search.name (id) ?? _("Unknown Character");
            bind_preview (id, null, size_pt, true, code, "%s\n%s".printf (name, code));
        }

        /** Renders a scalar using typed Pango attributes, never markup or injected font labels. */
        public void bind_preview (uint32 id, string? description, int size_pt, bool fallback,
                                  string caption, string detail, bool wide_caption = false) {
            drag_text = Search.text (id);
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
            var glyph_width_px = size_pt * 2 + 16;
            var minimum_width_px = wide_caption ? 176 : 72;
            width_request = glyph_width_px > minimum_width_px ? glyph_width_px : minimum_width_px;
            height_request = size_pt > 24 ? size_pt * 2 + 16 : 64;
            code_label.set_text (caption);
            tooltip_text = detail;
            update_property (Gtk.AccessibleProperty.LABEL, detail);
        }

        /** Clears recycled content so an unbound cell cannot expose a stale character. */
        public void clear () {
            drag_text = null;
            glyph_label.set_text ("");
            glyph_label.attributes = null;
            code_label.label = "";
            tooltip_text = null;
            reset_property (Gtk.AccessibleProperty.LABEL);
        }
    }
}
