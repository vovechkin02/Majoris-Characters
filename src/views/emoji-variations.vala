namespace Characters {
    /** Selects and copies exact catalog alternatives without altering the browser search. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/emoji-variations.ui")]
    internal sealed class EmojiVariations : Adw.Dialog {
        /** Confirms explicit clipboard operations. */
        [GtkChild]
        private unowned Adw.ToastOverlay toast_overlay;
        /** Displays the complete selected sequence using normal Pango font fallback. */
        [GtkChild]
        private unowned Gtk.Label preview;
        /** Catalog name or presentation description for the current selection. */
        [GtkChild]
        private unowned Gtk.Label preview_name;
        /** Exact code-point representation for the selected sequence. */
        [GtkChild]
        private unowned Gtk.Label preview_code;
        /** Explains unavailable glyphs without claiming that shaping was verified. */
        [GtkChild]
        private unowned Gtk.Label preview_note;
        /** Reports the number of available alternatives. */
        [GtkChild]
        private unowned Gtk.Label variant_count;
        /** Searchable native selection model, indexed into the immutable collection. */
        [GtkChild]
        private unowned Gtk.DropDown variant_filter;
        /** Copies only the selected original UTF-8 value. */
        [GtkChild]
        private unowned Gtk.Button copy_button;
        /** Opens the technical inspector for the exact selected sequence. */
        [GtkChild]
        private unowned Gtk.Button inspect_button;

        /** Adds the selected exact sequence to the session editor. */
        [GtkChild]
        private unowned Gtk.Button emoji_add;
        /** Non-owning editor reference for this and nested dialogs. */
        private weak ComposerView? composer;

        /** Immutable native collection owned by the dialog for its lifetime. */
        private Emoji.Variants? variants;

        /** Builds a searchable selection list; preserves the incoming identity if available. */
        public EmojiVariations (string text, ComposerView? composer = null) {
            this.composer = composer;
            emoji_add.visible = composer != null;
            emoji_add.clicked.connect (add_to_composer);
            variants = Emoji.create_variants (text);
            var model = new Gtk.StringList (null);
            uint selected = 0;
            if (variants != null) {
                for (size_t index = 0; index < variants.count (); index++) {
                    unowned string? value = variants.text (index);
                    unowned string? label = variants.label (index);
                    model.append ("%s — %s".printf (value ?? "", label ?? ""));
                    if (value == text) {
                        selected = (uint) index;
                    }
                }
            }
            variant_filter.model = model;
            variant_filter.selected = model.get_n_items () > 0 ? selected :
                Gtk.INVALID_LIST_POSITION;
            variant_count.label = _("Available variations: %u").printf (model.get_n_items ());
            variant_filter.notify["selected"].connect (update_preview);
            copy_button.clicked.connect (copy_selected);
            inspect_button.clicked.connect (inspect_selected);
            update_preview ();
        }

        /** Returns an owned exact sequence independently of the native collection's lifetime. */
        private string? selected_text () {
            if (variants == null || variant_filter.selected == Gtk.INVALID_LIST_POSITION) {
                return null;
            }
            return variants.text (variant_filter.selected);
        }

        /** Updates the preview, code points and action availability together. */
        private void update_preview () {
            var text = selected_text ();
            copy_button.sensitive = text != null;
            emoji_add.sensitive = text != null && composer != null;
            inspect_button.sensitive = text != null;
            if (text == null || variants == null) {
                preview.label = "";
                preview_name.label = _("No Emoji Variations Available");
                preview_code.label = "";
                preview_note.label = "";
                return;
            }
            preview.label = text;
            preview_name.label = variants.label (variant_filter.selected) ?? "";
            var snapshot = Inspector.from_text (text);
            preview_code.label = snapshot != null ?
                (snapshot.format (Inspector.Format.CODE) ?? "") : "";
            preview_note.label = preview.get_layout ().get_unknown_glyphs_count () > 0 ?
                _("Some glyphs are unavailable in the current fonts.") :
                _("Appearance depends on the installed emoji fonts.");
        }

        /** Publishes the exact selected sequence, including modifiers and selectors. */
        private void copy_selected () {
            var text = selected_text ();
            if (text != null) {
                get_clipboard ().set_text (text);
                toast_overlay.add_toast (new Adw.Toast (_("Copied to Clipboard")));
            }
        }

        /** Appends the chosen sequence without altering its modifiers or selectors. */
        private void add_to_composer () {
            var text = selected_text ();
            if (text != null && composer != null) {
                composer.append_text (text);
                toast_overlay.add_toast (new Adw.Toast (_("Added to Composer")));
            }
        }

        /** Opens an independent inspector without normalizing or modifying the selection. */
        private void inspect_selected () {
            var text = selected_text ();
            if (text == null) {
                return;
            }
            var snapshot = Inspector.from_text (text);
            if (snapshot != null) {
                new CharacterInspector ((owned) snapshot, composer).present (this);
            }
        }
    }
}
