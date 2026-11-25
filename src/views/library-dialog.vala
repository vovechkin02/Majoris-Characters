namespace Characters {
    /** Selects the ordering and mutation rules for a saved character collection. */
    internal enum LibraryMode {
        /** User-ordered independent favorites. */
        FAVORITES,
        /** Most recently recorded identities first. */
        RECENT,
        /** Descending retained count, with recency as the tie breaker. */
        FREQUENT,
    }

    /** Browses one bounded saved collection using a recyclable native list. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/library-dialog.ui")]
    internal sealed class LibraryDialog : Adw.Dialog {
        /** Confirms mutations and clipboard operations. */
        [GtkChild]
        private unowned Adw.ToastOverlay library_toasts;
        /** Searches exact text, Unicode names and code notation within this collection. */
        [GtkChild]
        private unowned Gtk.SearchEntry library_query;
        /** Virtualized collection list. */
        [GtkChild]
        private unowned Gtk.ListView library_list;
        /** Provides collection size and empty-state guidance. */
        [GtkChild]
        private unowned Gtk.Label library_status;
        /** Copies the selected identity. */
        [GtkChild]
        private unowned Gtk.Button library_copy;
        /** Adds the selected identity to the session editor. */
        [GtkChild]
        private unowned Gtk.Button library_add;
        /** Opens the technical inspector. */
        [GtkChild]
        private unowned Gtk.Button library_inspect;
        /** Saves a history identity to the independent favorite collection. */
        [GtkChild]
        private unowned Gtk.Button library_save;
        /** Removes the selected identity from this collection. */
        [GtkChild]
        private unowned Gtk.Button library_remove;
        /** Moves a favorite up in manual order. */
        [GtkChild]
        private unowned Gtk.Button library_up;
        /** Moves a favorite down in manual order. */
        [GtkChild]
        private unowned Gtk.Button library_down;
        /** Requests confirmation before clearing the shared usage history. */
        [GtkChild]
        private unowned Gtk.Button library_clear;

        /** Repository shared with the parent and nested inspector. */
        private LibraryStore library;
        /** Editor owned by the main window. */
        private weak ComposerView? composer;
        /** Collection represented by this dialog. */
        private LibraryMode mode;
        /** Owns the current filtered list and its selection. */
        private Gtk.SingleSelection selection;
        /** Count lookup for currently retained history identities. */
        private HashTable<string, uint> counts = new HashTable<string, uint> (str_hash, str_equal);
        /** Repository subscription, disconnected on close or disposal. */
        private ulong change_handler;

        /** Opens the selected collection and observes local changes until the dialog closes. */
        public LibraryDialog (LibraryStore library, LibraryMode mode, ComposerView? composer) {
            this.library = library;
            this.mode = mode;
            this.composer = composer;
            title = mode == LibraryMode.FAVORITES ? _("Favorites") :
                mode == LibraryMode.RECENT ? _("Recently Used") : _("Frequently Used");
            selection = new Gtk.SingleSelection (null);
            selection.autoselect = false;
            selection.can_unselect = true;
            var factory = new Gtk.SignalListItemFactory ();
            factory.setup.connect (setup_row);
            factory.bind.connect (bind_row);
            library_list.factory = factory;
            library_list.model = selection;
            selection.notify["selected-item"].connect (update_actions);
            library_list.activate.connect (activate_row);
            library_query.changed.connect (reload);
            library_copy.clicked.connect (copy_selected);
            library_add.clicked.connect (add_selected);
            library_inspect.clicked.connect (inspect_selected);
            library_save.clicked.connect (save_selected);
            library_remove.clicked.connect (remove_selected);
            library_up.clicked.connect (move_up);
            library_down.clicked.connect (move_down);
            library_clear.clicked.connect (confirm_clear);
            library_add.visible = composer != null;
            library_save.visible = mode != LibraryMode.FAVORITES;
            library_up.visible = mode == LibraryMode.FAVORITES;
            library_down.visible = mode == LibraryMode.FAVORITES;
            library_clear.visible = mode != LibraryMode.FAVORITES;
            change_handler = library.changed.connect (reload);
            closed.connect (stop);
            reload ();
        }

        /** Releases the repository subscription even without a normal dialog close. */
        public override void dispose () {
            stop ();
            base.dispose ();
        }

        /** Disconnects the longer-lived repository before the dialog releases its widgets. */
        private void stop () {
            if (change_handler != 0) {
                library.disconnect (change_handler);
                change_handler = 0;
            }
        }

        /** Creates one reusable text row for runtime-provided character data. */
        private static void setup_row (GLib.Object object) {
            var item = (Gtk.ListItem) object;
            var label = new Gtk.Label (null);
            label.xalign = 0;
            label.wrap = true;
            label.wrap_mode = Pango.WrapMode.WORD_CHAR;
            label.margin_top = 8;
            label.margin_bottom = 8;
            label.margin_start = 8;
            label.margin_end = 8;
            item.child = label;
        }

        /** Replaces every displayed value; placeholders are never stored or copied. */
        private void bind_row (GLib.Object object) {
            var item = (Gtk.ListItem) object;
            var text = ((Gtk.StringObject) item.item).get_string ();
            var label = (Gtk.Label) item.child;
            var snapshot = Inspector.from_text (text);
            if (snapshot == null) {
                label.label = _("Unavailable Character");
                return;
            }
            var name = snapshot.name () ?? _("Character Sequence");
            var code = snapshot.format (Inspector.Format.CODE) ?? "";
            var detail = "%s  %s\n%s".printf (snapshot.display () ?? "", name, code);
            if (mode != LibraryMode.FAVORITES) {
                detail += "\n" + _("Recorded uses: %u").printf (counts.lookup (text));
            }
            label.label = detail;
        }

        /** Rebuilds at most 512 identities and preserves the selected text when possible. */
        private void reload () {
            var previous = selected_text ();
            string[] values = {};
            counts.remove_all ();
            if (mode == LibraryMode.FAVORITES) {
                values = library.favorites ();
            } else {
                foreach (var entry in library.history_entries (mode == LibraryMode.FREQUENT)) {
                    values += entry.text;
                    counts.insert (entry.text, entry.count);
                }
            }
            var query = library_query.text.casefold ();
            string[] visible = {};
            uint selected = Gtk.INVALID_LIST_POSITION;
            foreach (var text in values) {
                if (LibraryQuery.matches (text, query)) {
                    if (previous == text) {
                        selected = (uint) visible.length;
                    }
                    visible += text;
                }
            }
            selection.model = new Gtk.StringList (visible);
            selection.selected = selected;
            library_status.label = visible.length == 0 ? _("No Characters in This View") :
                _("Showing %d of %d Characters").printf (visible.length, values.length);
            library_clear.sensitive = values.length > 0;
            update_actions ();
        }

        /** Restores the compact picker's filter, selected identity and approximate scroll position. */
        public void restore_context (string query, string? text, uint offset) {
            library_query.text = query;
            var model = selection.model;
            if (model == null || model.get_n_items () == 0) {
                return;
            }
            uint position = offset < model.get_n_items () ? offset : model.get_n_items () - 1;
            if (text != null) {
                for (uint index = 0; index < model.get_n_items (); index++) {
                    var item = model.get_item (index) as Gtk.StringObject;
                    if (item != null && item.get_string () == text) {
                        selection.selected = index;
                        position = index;
                        break;
                    }
                }
            }
            library_list.scroll_to (position, Gtk.ListScrollFlags.NONE, null);
        }

        /** Returns the exact selected identity, independent of the current model lifetime. */
        private string? selected_text () {
            var item = selection.selected_item as Gtk.StringObject;
            return item == null ? null : item.get_string ();
        }

        /** Enables reordering only when visible positions represent the entire favorite order. */
        private void update_actions () {
            bool selected = selected_text () != null;
            library_copy.sensitive = selected;
            library_add.sensitive = selected && composer != null;
            library_inspect.sensitive = selected;
            library_save.sensitive = selected;
            library_remove.sensitive = selected;
            bool can_move = selected && library_query.text == "" && mode == LibraryMode.FAVORITES;
            library_up.sensitive = can_move && selection.selected > 0;
            library_down.sensitive = can_move && selection.model != null &&
                selection.selected + 1 < selection.model.get_n_items ();
        }

        /** Activates the selected character using the ordinary inspector. */
        private void activate_row (uint position) {
            selection.selected = position;
            inspect_selected ();
        }

        /** Copies original text and records the explicit use, not metadata or display glyphs. */
        private void copy_selected () {
            var text = selected_text ();
            if (text != null) {
                get_clipboard ().set_text (text);
                var saved = library.record_use (text);
                notify_result (true, saved ? _("Copied to Clipboard") :
                    _("Copied to Clipboard; Usage History Could Not Be Saved"));
            }
        }

        /** Appends the exact selected text using the shared editor's usage accounting. */
        private void add_selected () {
            var text = selected_text ();
            if (text != null && composer != null) {
                composer.append_text (text);
                notify_result (true, _("Added to Composer"));
            }
        }

        /** Opens an owned snapshot without changing the collection or its filter. */
        private void inspect_selected () {
            var text = selected_text ();
            if (text != null) {
                var snapshot = Inspector.from_text (text);
                if (snapshot != null) {
                    new CharacterInspector ((owned) snapshot, composer, library).present (this);
                }
            }
        }

        /** Adds an independent favorite from the history collection. */
        private void save_selected () {
            var text = selected_text ();
            if (text != null) {
                notify_result (library.add_favorite (text), _("Saved to Favorites"));
            }
        }

        /** Removes only the selected entry from the active collection. */
        private void remove_selected () {
            var text = selected_text ();
            if (text != null) {
                bool success = mode == LibraryMode.FAVORITES ? library.remove_favorite (text) :
                    library.remove_history (text);
                notify_result (success, _("Character Removed"));
            }
        }

        /** Moves the selected favorite one position earlier. */
        private void move_up () {
            move_selected (-1);
        }

        /** Moves the selected favorite one position later. */
        private void move_down () {
            move_selected (1);
        }

        /** Updates manual ordering only when no filter obscures adjacent favorites. */
        private void move_selected (int delta) {
            var text = selected_text ();
            if (text != null && library_query.text == "") {
                notify_result (library.move_favorite (text, delta), _("Favorite Moved"));
            }
        }

        /** Clears the shared history only after explicit confirmation. */
        private void confirm_clear () {
            var dialog = new Adw.AlertDialog (_("Clear Character History?"),
                _("Recently and frequently used characters will be removed. Favorites are kept."));
            dialog.add_response ("cancel", _("Cancel"));
            dialog.add_response ("clear", _("Clear History"));
            dialog.set_response_appearance ("clear", Adw.ResponseAppearance.DESTRUCTIVE);
            dialog.set_default_response ("cancel");
            dialog.set_close_response ("cancel");
            dialog.response.connect ((response) => {
                if (response == "clear") {
                    notify_result (library.clear_history (), _("History Cleared"));
                }
            });
            dialog.present (this);
        }

        /** Reports failed persistence rather than falsely confirming a saved mutation. */
        private void notify_result (bool success, string message) {
            library_toasts.add_toast (new Adw.Toast (success ? message :
                _("Unable to Update Collection. Check the Limit or Settings Permissions.")));
        }
    }
}
