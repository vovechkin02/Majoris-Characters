namespace Characters {
    /** Small clipboard picker with bounded results and independently owned search work. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/compact-picker.ui")]
    internal sealed class CompactPicker : Adw.ApplicationWindow {
        /** Search across the full catalog or the selected local collection. */
        [GtkChild]
        private unowned Gtk.SearchEntry picker_query;
        /** All, emoji, symbols, favorites and recent scopes. */
        [GtkChild]
        private unowned Gtk.DropDown picker_scope;
        /** Bounded native grid of copy actions. */
        [GtkChild]
        private unowned Gtk.FlowBox picker_results;
        /** Announces loading, empty, truncated and clipboard states. */
        [GtkChild]
        private unowned Gtk.Label picker_status;
        /** Opens the full application without changing the composition. */
        [GtkChild]
        private unowned Gtk.Button expand_button;
        /** Maximum visible results, independent of native repertoire size. */
        private const uint LIMIT = 80;
        /** Owns shared preview preferences for the picker lifetime. */
        private GLib.Settings settings;
        /** Tracks the preference observer so it can be disconnected at close. */
        private ulong settings_handler;
        /** Repository for explicit copies and saved collections. */
        private LibraryStore library;
        /** Serializes search work and discards superseded requests. */
        private SearchController controller;
        /** Controller subscription released when the window closes. */
        private ulong completion_handler;
        /** Collection subscription released when the window closes. */
        private ulong library_handler;
        /** Weak application reference; GtkApplication owns its windows. */
        private weak Application owner;

        /** Constructs a compact, theme-native picker without opening the main browser. */
        public CompactPicker (Application application) {
            Object (application: application);
            owner = application;
            settings = new GLib.Settings (APP_ID);
            library = new LibraryStore (settings);
            controller = new SearchController ();
            completion_handler = controller.completed.connect (on_completed);
            library_handler = library.changed.connect (on_library_changed);
            settings_handler = settings.changed["grid-preview-size"].connect (reload);
            picker_scope.model = new Gtk.StringList ({ _("All Characters"), _("Emoji"),
                _("Symbols"), _("Favorites"), _("Recently Used") });
            picker_scope.selected = 1;
            picker_scope.notify["selected"].connect (reload);
            picker_query.changed.connect (reload);
            picker_query.activate.connect (activate_first);
            expand_button.clicked.connect (() => {
                owner.open_search (picker_query.text, 0);
                close ();
            });
            var keys = new Gtk.EventControllerKey ();
            keys.key_pressed.connect (on_key_pressed);
            add_controller (keys);
            reload ();
        }

        /** Focuses search when opening or reactivating the picker. */
        public void focus_query () {
            picker_query.grab_focus ();
        }

        /** Cancels the worker before GTK removes the window. */
        public override bool close_request () {
            stop ();
            return false;
        }

        /** Disconnects longer-lived sources even during exceptional disposal. */
        public override void dispose () {
            stop ();
            base.dispose ();
        }

        /** Escape closes only this picker, leaving any full window intact. */
        private bool on_key_pressed (uint keyval, uint keycode, Gdk.ModifierType state) {
            if (keyval == Gdk.Key.Escape) {
                close ();
                return true;
            }
            return false;
        }

        /** Activates the first visible result from the focused search entry. */
        private void activate_first () {
            var first = picker_results.get_child_at_index (0);
            if (first != null) {
                var button = first.get_child () as Gtk.Button;
                if (button != null) {
                    button.clicked ();
                }
            }
        }

        /** Clears old cells before any new query or saved collection is displayed. */
        private void clear_results () {
            Gtk.Widget? child;
            while ((child = picker_results.get_first_child ()) != null) {
                picker_results.remove (child);
            }
        }

        /** Updates local collections only when currently displayed. */
        private void on_library_changed () {
            if (picker_scope.selected >= 3) {
                reload ();
            }
        }

        /** Loads saved text locally or submits the current catalog scope off-thread. */
        private void reload () {
            controller.cancel ();
            clear_results ();
            var query = picker_query.text;
            if (query.length > DesktopSearch.MAX_QUERY_BYTES) {
                picker_status.label = _("Search is too long");
                return;
            }
            if (picker_scope.selected >= 3) {
                string[] values = {};
                if (picker_scope.selected == 3) {
                    values = library.favorites ();
                } else {
                    foreach (var entry in library.history_entries ()) {
                        values += entry.text;
                    }
                }
                uint shown = 0;
                var folded = query.casefold ();
                foreach (var text in values) {
                    var snapshot = Inspector.from_text (text);
                    if (snapshot == null) {
                        continue;
                    }
                    var name = snapshot.name () ?? "";
                    var code = snapshot.format (Inspector.Format.CODE) ?? "";
                    if (folded != "" && !text.casefold ().contains (folded) &&
                        !name.casefold ().contains (folded) && !code.casefold ().contains (folded)) {
                        continue;
                    }
                    append_result (text, snapshot.display () ?? text, name, code);
                    shown++;
                    if (shown == LIMIT) {
                        break;
                    }
                }
                show_count (shown);
                return;
            }
            var scope = (Search.Scope) picker_scope.selected;
            picker_status.label = _("Searching…");
            controller.submit (scope, -1, -1, query);
        }

        /** Publishes only the newest catalog result, with an explicit visible limit. */
        private void on_completed (SearchRequest request) {
            if (request.get_status () != Search.Status.OK) {
                picker_status.label = _("Search unavailable");
                return;
            }
            uint shown = 0;
            for (uint index = 0; index < request.count () && shown < LIMIT; index++) {
                var id = request.id (index);
                var text = Search.text (id);
                if (text == null) {
                    continue;
                }
                append_result (text, Search.display (id) ?? text, Search.name (id) ?? "",
                    Search.code (id) ?? "");
                shown++;
            }
            show_count (shown);
        }

        /** Adds one accessible action whose closure captures the exact original text. */
        private void append_result (string text, string display, string name, string code) {
            var label = new Gtk.Label (display);
            var attributes = new Pango.AttrList ();
            var size_pt = settings.get_int ("grid-preview-size");
            attributes.insert (new Pango.AttrSize (size_pt * Pango.SCALE));
            label.attributes = attributes;
            var button = new Gtk.Button ();
            button.child = label;
            var cell_size_px = size_pt * 2 > 48 ? size_pt * 2 : 48;
            button.width_request = cell_size_px;
            button.height_request = cell_size_px;
            var detail = "%s\n%s".printf (name, code);
            button.tooltip_text = detail;
            button.update_property (Gtk.AccessibleProperty.LABEL, detail);
            button.clicked.connect (() => {
                owner.copy_from_picker (get_clipboard (), text);
                library.record_use (text);
                picker_status.label = _("Character copied. Paste it into your application.");
            });
            picker_results.append (button);
        }

        /** Describes the bounded view without claiming that it is the whole repertoire. */
        private void show_count (uint count) {
            if (count == 0) {
                picker_status.label = _("No matching characters");
            } else if (count == LIMIT) {
                picker_status.label = _("Showing up to 80 characters. Refine your search for more.");
            } else {
                picker_status.label = _("Choose a character to copy");
            }
        }

        /** Releases observer connections before the owning widget tree disappears. */
        private void stop () {
            if (completion_handler != 0) {
                controller.disconnect (completion_handler);
                completion_handler = 0;
                controller.shutdown ();
            }
            if (settings_handler != 0) {
                settings.disconnect (settings_handler);
                settings_handler = 0;
            }
            if (library_handler != 0) {
                library.disconnect (library_handler);
                library_handler = 0;
            }
        }
    }
}
