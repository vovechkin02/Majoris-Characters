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
            var glyph = Search.display (id) ?? "";
            var code = Search.code (id) ?? "";
            var name = Search.name (id) ?? _("Unknown Character");
            glyph_label.label = glyph;
            code_label.label = code;
            tooltip_text = "%s\n%s".printf (name, code);
            update_property (Gtk.AccessibleProperty.LABEL, "%s, %s".printf (name, code));
        }

        /** Clears recycled content so an unbound cell cannot expose a stale character. */
        public void clear () {
            glyph_label.label = "";
            code_label.label = "";
            tooltip_text = null;
            reset_property (Gtk.AccessibleProperty.LABEL);
        }
    }
}
