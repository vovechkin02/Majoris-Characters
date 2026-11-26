namespace Characters {
    /** Selects and copies exact catalog alternatives without altering the browser search. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/emoji-variations.ui")]
    internal sealed class EmojiVariations : Adw.Dialog {
        /** Emits an explicitly chosen exact sequence instead of writing to the clipboard. */
        public signal void chosen (string text);
        /** True when the invoking picker owns the final delivery operation. */
        private bool delegated;
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
        /** Optional shared local collection repository. */
        private LibraryStore? library;
        /** Saves the exact selected identity into Favorites. */
        [GtkChild]
        private unowned Gtk.Button emoji_favorite;

        /** Immutable variation collection owned by the dialog for its lifetime. */
        private Emoji.Variants? variants;

        /** Builds a searchable selection list; preserves the incoming identity if available. */
        public EmojiVariations (string text, ComposerView? composer = null,
                                LibraryStore? library = null, bool delegated = false,
                                bool client_mode = false) {
            this.delegated = delegated;
            if (delegated) {
                copy_button.label = client_mode ? _("Insert Character") : _("Copy Character");
                // The picker owns delivery and clipboard lifetime; inspect at the picker level.
                inspect_button.visible = false;
            }
            this.composer = composer;
            this.library = library;
            emoji_favorite.visible = library != null;
            emoji_favorite.sensitive = false;
            emoji_favorite.clicked.connect (save_favorite);
            emoji_add.visible = composer != null;
            emoji_add.clicked.connect (add_to_composer);
            variants = Emoji.create_variants (text);
            var model = new Gtk.StringList (null);
            uint selected = 0;
            if (variants != null) {
                for (size_t index = 0; index < variants.count (); index++) {
                    unowned string? value = variants.text (index);
                    unowned string? label = variants.label (index);
                    model.append ("%s — %s".printf (value ?? "",
                        PropertyLabels.emoji_variant_label (label ?? "")));
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

        /** Returns an owned exact sequence independently of the variation collection's lifetime. */
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
            emoji_favorite.sensitive = text != null && library != null;
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
            preview_name.label = PropertyLabels.emoji_variant_label (
                variants.label (variant_filter.selected) ?? "");
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
                if (delegated) {
                    // Close before notifying, because a client pick also closes its window.
                    if (close ()) {
                        chosen (text);
                    }
                    return;
                }
                Application.copy_widget_text (this, text);
                var saved = library == null || library.record_use (text);
                toast_overlay.add_toast (new Adw.Toast (saved ? _("Copied to Clipboard") :
                    _("Copied to Clipboard; Usage History Could Not Be Saved")));
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
                new CharacterInspector ((owned) snapshot, composer, library).present (this);
            }
        }

        /** Saves the exact selected identity without recording a copy or insertion. */
        private void save_favorite () {
            var text = selected_text ();
            if (text != null && library != null) {
                var message = library.add_favorite (text) ? _("Saved to Favorites") :
                    _("Unable to Save Favorite. Check the Limit or Settings Permissions.");
                toast_overlay.add_toast (new Adw.Toast (message));
            }
        }
    }
}
