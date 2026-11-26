namespace Characters {
    /** Compact browser with bounded pages and explicit copy or client-selection actions. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/compact-picker.ui")]
    internal sealed class CompactPicker : Adw.ApplicationWindow {
        /** Maximum cells constructed for one page, independent of the repertoire size. */
        private const uint PAGE_SIZE = 80;
        /** Index of the first saved collection in the scope model. */
        private const uint SAVED_START = 3;
        /** Query across the selected catalog or saved collection. */
        [GtkChild]
        private unowned Gtk.SearchEntry picker_query;
        /** Catalog and saved collection scopes in stable order. */
        [GtkChild]
        private unowned Gtk.DropDown picker_scope;
        /** Category filter, indexed by native collection slots. */
        [GtkChild]
        private unowned Gtk.DropDown picker_category;
        /** Bounded keyboard-accessible grid of character identities. */
        [GtkChild]
        private unowned Gtk.FlowBox picker_results;
        /** Scroll container reset on each page change. */
        [GtkChild]
        private unowned Gtk.ScrolledWindow picker_scroll;
        /** Reports query state and explicit operation outcomes. */
        [GtkChild]
        private unowned Gtk.Label picker_status;
        /** Reports the visible range and total count. */
        [GtkChild]
        private unowned Gtk.Label picker_count;
        /** Displays the selected identity's name and code points. */
        [GtkChild]
        private unowned Gtk.Label picker_selection;
        /** Moves back one bounded page. */
        [GtkChild]
        private unowned Gtk.Button picker_previous;
        /** Moves forward one bounded page. */
        [GtkChild]
        private unowned Gtk.Button picker_next;
        /** Copies or returns the selected exact character to a requesting client. */
        [GtkChild]
        private unowned Gtk.Button picker_commit;
        /** Opens the shared variation selector. */
        [GtkChild]
        private unowned Gtk.Button picker_variations;
        /** Adds or removes the selected identity from Favorites. */
        [GtkChild]
        private unowned Gtk.Button picker_favorite;
        /** Opens the selected identity's inspector in standalone mode. */
        [GtkChild]
        private unowned Gtk.Button picker_inspect;
        /** Transfers the current scope, filter, query, page and selection. */
        [GtkChild]
        private unowned Gtk.Button expand_button;
        /** Shared preview preferences for the picker lifetime. */
        private GLib.Settings settings;
        /** Tracks the preference observer for deterministic cleanup. */
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
        /** Completed catalog result, retained without creating per-result widgets. */
        private SearchRequest? results;
        /** Filtered exact saved identities; bounded by the repository limit. */
        private string[] saved_results = {};
        /** Mapping of visible category rows to native slots. */
        private int[] category_slots = { -1 };
        /** First index of the currently displayed page. */
        private uint offset;
        /** Suppresses intermediate selection and filter callbacks during model changes. */
        private bool updating;
        /** True while the latest catalog request has not yet published. */
        private bool searching;

        /** Constructs a compact, theme-native picker in standalone or client mode. */
        public CompactPicker (Application application) {
            Object (application: application);
            owner = application;
            settings = new GLib.Settings (APP_ID);
            library = new LibraryStore (settings);
            controller = new SearchController ();
            completion_handler = controller.completed.connect (on_completed);
            library_handler = library.changed.connect (on_library_changed);
            settings_handler = settings.changed["grid-preview-size"].connect (refresh_preview);
            picker_scope.model = new Gtk.StringList ({ _("All Characters"), _("Emoji"),
                _("Symbols"), _("Favorites"), _("Recently Used"), _("Frequently Used") });
            picker_scope.selected = 1;
            picker_scope.notify["selected"].connect (on_scope_changed);
            picker_category.notify["selected"].connect (reload);
            picker_query.changed.connect (reload);
            picker_query.activate.connect (activate_first);
            picker_results.selected_children_changed.connect (update_selection);
            picker_results.child_activated.connect (activate_cell);
            picker_previous.clicked.connect (previous_page);
            picker_next.clicked.connect (next_page);
            picker_commit.clicked.connect (commit_selected);
            picker_variations.clicked.connect (open_variations);
            picker_favorite.clicked.connect (toggle_favorite);
            picker_inspect.clicked.connect (inspect_selected);
            expand_button.clicked.connect (expand);
            if (owner.is_client_picker ()) {
                picker_commit.label = _("Insert Character");
                expand_button.visible = false;
                picker_inspect.visible = false;
            }
            var keys = new Gtk.EventControllerKey ();
            keys.key_pressed.connect (on_key_pressed);
            add_controller (keys);
            on_scope_changed ();
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

        /** Escape closes only this picker; Ctrl+F returns to the query. */
        private bool on_key_pressed (uint keyval, uint keycode, Gdk.ModifierType state) {
            if (keyval == Gdk.Key.Escape) {
                close ();
                return true;
            }
            var modifiers = state & Gtk.accelerator_get_default_mod_mask ();
            if (keyval == Gdk.Key.f && modifiers == Gdk.ModifierType.CONTROL_MASK) {
                focus_query ();
                return true;
            }
            return false;
        }

        /** Maps category names to native slots without relying on translated labels. */
        private void on_scope_changed () {
            updating = true;
            var model = new Gtk.StringList ({ _("All Collections") });
            category_slots = { -1 };
            if (!is_saved ()) {
                var scope = (Search.Scope) picker_scope.selected;
                for (size_t index = 0; index < Search.filter_count (scope); index++) {
                    var name = Search.filter_name (scope, index);
                    if (name != null) {
                        model.append (_(name));
                        category_slots += (int) index;
                    }
                }
            }
            picker_category.model = model;
            picker_category.selected = 0;
            picker_category.visible = category_slots.length > 1;
            updating = false;
            reload ();
        }

        /** Returns whether the current scope is backed by the local saved repository. */
        private bool is_saved () {
            return picker_scope.selected >= SAVED_START;
        }

        /** Resolves the selected saved collection's existing ordering contract. */
        private LibraryMode saved_mode () {
            return (LibraryMode) (picker_scope.selected - SAVED_START);
        }

        /** Resolves the actual native category slot or the unfiltered sentinel. */
        private int current_filter () {
            var index = picker_category.selected;
            return index < (uint) category_slots.length ? category_slots[index] : -1;
        }

        /** Returns the current number of matches, even beyond the displayed page. */
        private uint total_count () {
            return is_saved () ? (uint) saved_results.length :
                (results != null ? results.count () : 0);
        }

        /** Resolves selected page-local position to an absolute result index. */
        private uint selected_position () {
            foreach (var child in picker_results.get_selected_children ()) {
                return offset + (uint) child.get_index ();
            }
            return uint.MAX;
        }

        /** Returns an owned exact Unicode snapshot for the selected identity, including NUL. */
        private Inspector.Snapshot? selected_snapshot () {
            var position = selected_position ();
            if (position >= total_count ()) {
                return null;
            }
            if (is_saved ()) {
                return Inspector.from_text (saved_results[position]);
            }
            return results != null ? Inspector.from_id (results.id (position)) : null;
        }

        /** Rebuilds only the bounded saved list, with the same matching as its full dialog. */
        private void load_saved () {
            string[] values = {};
            if (saved_mode () == LibraryMode.FAVORITES) {
                values = library.favorites ();
            } else {
                foreach (var entry in library.history_entries (saved_mode () == LibraryMode.FREQUENT)) {
                    values += entry.text;
                }
            }
            var folded = picker_query.text.casefold ();
            saved_results = {};
            foreach (var text in values) {
                if (LibraryQuery.matches (text, folded)) {
                    saved_results += text;
                }
            }
        }

        /** Restores a saved selection after a mutation, even if its ordering has changed. */
        private void on_library_changed () {
            if (!is_saved () || picker_query.text.length > DesktopSearch.MAX_QUERY_BYTES ||
                !picker_query.text.validate ()) {
                update_selection ();
                return;
            }
            var snapshot = selected_snapshot ();
            var text = snapshot != null ? snapshot.text () : null;
            load_saved ();
            uint position = uint.MAX;
            if (text != null) {
                for (uint index = 0; index < saved_results.length; index++) {
                    if (saved_results[index] == text) {
                        position = index;
                        offset = index / PAGE_SIZE * PAGE_SIZE;
                        break;
                    }
                }
            }
            show_page (position);
        }

        /** Rebinds only the current page when preview preferences change. */
        private void refresh_preview () {
            if (!searching) {
                show_page (selected_position ());
            }
        }

        /** Cancels obsolete requests and resets paging on query or collection changes. */
        private void reload () {
            if (updating) {
                return;
            }
            controller.cancel ();
            searching = false;
            results = null;
            saved_results = {};
            offset = 0;
            show_page ();
            var query = picker_query.text;
            if (query.length > DesktopSearch.MAX_QUERY_BYTES || !query.validate ()) {
                picker_status.label = _("Search is too long or invalid");
                return;
            }
            if (is_saved ()) {
                load_saved ();
                show_page ();
                return;
            }
            picker_status.label = _("Searching…");
            searching = true;
            controller.submit ((Search.Scope) picker_scope.selected, current_filter (), -1, query);
        }

        /** Publishes only the newest completed catalog request, preserving its entire result. */
        private void on_completed (SearchRequest request) {
            searching = false;
            if (request.get_status () != Search.Status.OK) {
                picker_status.label = _("Search unavailable");
                return;
            }
            results = request;
            show_page ();
        }

        /** Replaces one bounded page, optionally restoring an absolute selection position. */
        private void show_page (uint position = uint.MAX) {
            updating = true;
            Gtk.Widget? child;
            while ((child = picker_results.get_first_child ()) != null) {
                picker_results.remove (child);
            }
            var total = total_count ();
            if (offset >= total) {
                offset = total == 0 ? 0 : (total - 1) / PAGE_SIZE * PAGE_SIZE;
            }
            var end = offset + PAGE_SIZE < total ? offset + PAGE_SIZE : total;
            for (uint index = offset; index < end; index++) {
                if (is_saved ()) {
                    var snapshot = Inspector.from_text (saved_results[index]);
                    append_result (snapshot != null ? (snapshot.display () ?? "·") : "·",
                        snapshot != null ? (snapshot.name () ?? "") : "",
                        snapshot != null ? (snapshot.format (Inspector.Format.CODE) ?? "") : "");
                } else if (results != null) {
                    var id = results.id (index);
                    append_result (Search.display (id) ?? "·", Search.name (id) ?? "",
                        Search.code (id) ?? "");
                }
            }
            if (position >= offset && position < end) {
                var selected = picker_results.get_child_at_index ((int) (position - offset));
                if (selected != null) {
                    picker_results.select_child (selected);
                }
            }
            updating = false;
            picker_scroll.vadjustment.value = picker_scroll.vadjustment.lower;
            picker_previous.sensitive = offset > 0;
            picker_next.sensitive = end < total;
            picker_count.label = total == 0 ? _("0 Characters") :
                _("Showing %u–%u of %u").printf (offset + 1, end, total);
            picker_status.label = total == 0 ? _("No matching characters") :
                _("Select a character or activate it to use it.");
            update_selection ();
        }

        /** Creates an accessible selectable cell; display text is never used as its identity. */
        private void append_result (string display, string name, string code) {
            var label = new Gtk.Label (display);
            var attributes = new Pango.AttrList ();
            var size_pt = settings.get_int ("grid-preview-size");
            attributes.insert (new Pango.AttrSize (size_pt * Pango.SCALE));
            label.attributes = attributes;
            var cell_size_px = size_pt * 2 > 48 ? size_pt * 2 : 48;
            label.width_request = cell_size_px;
            label.height_request = cell_size_px;
            var cell = new Gtk.FlowBoxChild ();
            cell.child = label;
            var detail = "%s\n%s".printf (name, code);
            cell.tooltip_text = detail;
            cell.update_property (Gtk.AccessibleProperty.LABEL, detail);
            picker_results.append (cell);
        }

        /** Moves to the previous page without repeating native search. */
        private void previous_page () {
            if (offset >= PAGE_SIZE) {
                offset -= PAGE_SIZE;
                show_page ();
                picker_count.announce (picker_count.label, Gtk.AccessibleAnnouncementPriority.MEDIUM);
            }
        }

        /** Moves to the next page without repeating native search. */
        private void next_page () {
            if (offset + PAGE_SIZE < total_count ()) {
                offset += PAGE_SIZE;
                show_page ();
                picker_count.announce (picker_count.label, Gtk.AccessibleAnnouncementPriority.MEDIUM);
            }
        }

        /** Reports whether the exact selected text is independently saved. */
        private bool is_favorite (string text) {
            foreach (var value in library.favorites ()) {
                if (value == text) {
                    return true;
                }
            }
            return false;
        }

        /** Synchronizes all actions with the current selected identity. */
        private void update_selection () {
            if (updating) {
                return;
            }
            var snapshot = selected_snapshot ();
            var text = snapshot != null ? snapshot.text () : null;
            var usable = text != null && LibraryStore.is_valid (text);
            picker_commit.sensitive = usable;
            picker_favorite.sensitive = usable;
            picker_inspect.sensitive = snapshot != null;
            picker_variations.sensitive = usable && Emoji.is_candidate (text ?? "");
            picker_favorite.label = usable && is_favorite (text ?? "") ?
                _("Remove Favorite") : _("Add to Favorites");
            picker_selection.label = snapshot != null ? "%s\n%s".printf (
                snapshot.name () ?? "", snapshot.format (Inspector.Format.CODE) ?? "") :
                _("No Character Selected");
        }

        /** Enter in the search entry commits the selection or the first displayed identity. */
        private void activate_first () {
            if (selected_position () == uint.MAX) {
                var first = picker_results.get_child_at_index (0);
                if (first != null) {
                    picker_results.select_child (first);
                }
            }
            commit_selected ();
        }

        /** Double-click or keyboard activation commits the exact activated cell. */
        private void activate_cell (Gtk.FlowBoxChild child) {
            picker_results.select_child (child);
            commit_selected ();
        }

        /** Copies or returns the selected text after validating the actual identity. */
        private void commit_selected () {
            var snapshot = selected_snapshot ();
            var text = snapshot != null ? snapshot.text () : null;
            if (text != null) {
                commit_text (text);
            }
        }

        /** Delivers the exact text, recording history only for an actual standalone copy. */
        private void commit_text (string text) {
            if (!LibraryStore.is_valid (text)) {
                return;
            }
            if (owner.is_client_picker ()) {
                owner.finish_client_pick (text);
                close ();
                return;
            }
            owner.copy_from_picker (get_clipboard (), text);
            var saved = library.record_use (text);
            picker_status.label = saved ? _("Character copied. Paste it into your application.") :
                _("Copied to Clipboard; Usage History Could Not Be Saved");
            picker_status.announce (picker_status.label, Gtk.AccessibleAnnouncementPriority.MEDIUM);
        }

        /** Opens the existing emoji family selector, with the same delivery path as the grid. */
        private void open_variations () {
            var snapshot = selected_snapshot ();
            var text = snapshot != null ? snapshot.text () : null;
            if (text != null && Emoji.is_candidate (text)) {
                var dialog = new EmojiVariations (text, null, library, true,
                    owner.is_client_picker ());
                dialog.chosen.connect (commit_text);
                dialog.present (this);
            }
        }

        /** Toggles independent favorites without counting a usage event. */
        private void toggle_favorite () {
            var snapshot = selected_snapshot ();
            var text = snapshot != null ? snapshot.text () : null;
            if (text != null) {
                var removing = is_favorite (text);
                var saved = removing ? library.remove_favorite (text) : library.add_favorite (text);
                picker_status.label = saved ? (removing ? _("Character Removed") :
                    _("Saved to Favorites")) : _("Unable to Update Favorites");
                update_selection ();
            }
        }

        /** Opens the existing inspector without losing the picker selection. */
        private void inspect_selected () {
            var snapshot = selected_snapshot ();
            if (snapshot != null && !owner.is_client_picker ()) {
                new CharacterInspector ((owned) snapshot, null, library).present (this);
            }
        }

        /** Transfers the saved collection or exact catalog context into the full application. */
        private void expand () {
            if (owner.is_client_picker ()) {
                return;
            }
            var position = selected_position ();
            if (is_saved ()) {
                var snapshot = selected_snapshot ();
                owner.open_library_context (saved_mode (), picker_query.text,
                    snapshot != null ? snapshot.text () : null, offset);
            } else {
                var id = results != null && position < results.count () ?
                    results.id (position) : uint32.MAX;
                owner.open_picker_context ((Search.Scope) picker_scope.selected, current_filter (),
                    picker_query.text, position == uint.MAX ? offset : position, id);
            }
            close ();
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
