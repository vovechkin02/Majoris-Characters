namespace Characters {
    /** Owns the XML browser, selection, search lifecycle, and persistent window geometry. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/main-window.ui")]
    internal sealed class MainWindow : Adw.ApplicationWindow {
        /** Maximum number of model objects held by the virtualized grid per page. */
        private const uint PAGE_SIZE = 512;

        /** Collapsible collection sidebar and its toggle. */
        [GtkChild]
        private unowned Adw.OverlaySplitView split_view;
        /** Header control for opening or hiding the sidebar. */
        [GtkChild]
        private unowned Gtk.ToggleButton sidebar_toggle;
        /** Primary scope rows, ordered according to the native Scope enum. */
        [GtkChild]
        private unowned Gtk.ListBox scope_list;
        /** Query input, with changes immediately superseding older work. */
        [GtkChild]
        private unowned Gtk.SearchEntry search_entry;
        /** Contextual filter for blocks, scripts, emoji or semantic collections. */
        [GtkChild]
        private unowned Gtk.DropDown collection_filter;
        /** Optional scalar general-category constraint. */
        [GtkChild]
        private unowned Gtk.DropDown category_filter;
        /** Loading, results, empty and error presentation. */
        [GtkChild]
        private unowned Gtk.Stack results_stack;
        /** Animated only during an outstanding search. */
        [GtkChild]
        private unowned Gtk.Spinner loading_spinner;
        /** Explains invalid queries and worker failures. */
        [GtkChild]
        private unowned Adw.StatusPage error_page;
        /** GTK virtualized grid; only visible cells are bound to metadata. */
        [GtkChild]
        private unowned Gtk.GridView character_grid;
        /** Scroll container reset when a new result page is displayed. */
        [GtkChild]
        private unowned Gtk.ScrolledWindow grid_scroll;
        /** Navigates to the preceding bounded page. */
        [GtkChild]
        private unowned Gtk.Button previous_button;
        /** Navigates to the following bounded page. */
        [GtkChild]
        private unowned Gtk.Button next_button;
        /** Reports the visible range and total matched identities. */
        [GtkChild]
        private unowned Gtk.Label result_count;
        /** Selected character or sequence name. */
        [GtkChild]
        private unowned Gtk.Label selected_name;
        /** Full selected code-point sequence. */
        [GtkChild]
        private unowned Gtk.Label selected_code;

        /** Opens the full inspector for the current selection. */
        [GtkChild]
        private unowned Gtk.Button inspect_button;

        /** Opens the exact selected emoji family, independent of browser results. */
        [GtkChild]
        private unowned Gtk.Button emoji_button;

        /** Displays confirmation for direct browser clipboard operations. */
        [GtkChild]
        private unowned Adw.ToastOverlay browser_toasts;
        /** Copies the selected original text, not its grid preview. */
        [GtkChild]
        private unowned Gtk.Button browser_copy;
        /** Adds the selected original text to the session composition. */
        [GtkChild]
        private unowned Gtk.Button browser_add;
        /** Hosts the independently defined XML composer view. */
        [GtkChild]
        private unowned Gtk.Box composer_slot;
        /** One session-only editor shared by browser and transient dialogs. */
        private ComposerView composer;

        /** Owns persistent geometry settings for this window. */
        private GLib.Settings settings;
        /** Serializes native worker jobs independently of GTK widgets. */
        private SearchController controller;
        /** GTK-owned selection, containing one bounded page at a time. */
        private Gtk.SingleSelection selection;
        /** Last successfully published result; contains only compact native IDs. */
        private SearchRequest? results;
        /** Primary collection selected in the sidebar. */
        private Search.Scope scope = Search.Scope.ALL;
        /** Mapping from visible dropdown positions to native filter slots. */
        private int[] filter_slots = {};
        /** Zero-based first result index of the current page. */
        private uint offset = 0;
        /** Guards programmatic filter changes against recursive searches. */
        private bool updating_filters = false;
        /** Signal handler ID, disconnected before shutdown. */
        private ulong completion_handler = 0;

        /** Constructs the XML-backed browser, restores geometry and starts initial browsing. */
        public MainWindow (Application application) {
            Object (application: application);
            settings = new GLib.Settings (APP_ID);
            set_default_size (settings.get_int ("window-width"),
                settings.get_int ("window-height"));
            if (settings.get_boolean ("window-maximized")) {
                maximize ();
            }
            composer = new ComposerView ();
            composer_slot.append (composer);
            controller = new SearchController ();
            completion_handler = controller.completed.connect (on_search_completed);
            configure_grid ();
            configure_filters ();
            configure_actions ();
            submit_search ();
        }

        /** Stops future search publication and persists window geometry through normal closing. */
        public override bool close_request () {
            stop_search ();
            int width_px;
            int height_px;
            get_default_size (out width_px, out height_px);
            settings.set_int ("window-width", width_px.clamp (320, 8192));
            settings.set_int ("window-height", height_px.clamp (320, 8192));
            settings.set_boolean ("window-maximized", is_maximized ());
            GLib.Settings.sync ();
            return false;
        }

        /** Cancels work if the window is disposed without a normal close request. */
        public override void dispose () {
            if (controller != null) {
                stop_search ();
            }
            base.dispose ();
        }

        /** Creates a reusable selection model and native virtualized cell factory. */
        private void configure_grid () {
            selection = new Gtk.SingleSelection (null);
            selection.autoselect = false;
            selection.can_unselect = true;
            selection.notify["selected-item"].connect (update_selection);
            var factory = new Gtk.SignalListItemFactory ();
            factory.setup.connect (setup_tile);
            factory.bind.connect (bind_tile);
            factory.unbind.connect (unbind_tile);
            character_grid.factory = factory;
            character_grid.model = selection;
            character_grid.activate.connect (on_grid_activate);
            previous_button.clicked.connect (previous_page);
            next_button.clicked.connect (next_page);
        }

        /** Activating a cell with Enter or double-click opens its inspector. */
        private void on_grid_activate (uint position) {
            selection.selected = position;
            inspect_selected (null);
        }

        /** Creates one reusable XML tile for a list item, independent of result identity. */
        private static void setup_tile (GLib.Object object) {
            var item = (Gtk.ListItem) object;
            item.child = new CharacterTile ();
        }

        /** Populates a recycled tile using its opaque numeric model identity. */
        private static void bind_tile (GLib.Object object) {
            var item = (Gtk.ListItem) object;
            var value = (Gtk.StringObject) item.item;
            var tile = (CharacterTile) item.child;
            tile.bind_id ((uint32) uint.parse (value.get_string ()));
        }

        /** Clears the tile before reuse; no result objects are retained by the cell. */
        private static void unbind_tile (GLib.Object object) {
            var item = (Gtk.ListItem) object;
            var tile = (CharacterTile) item.child;
            tile.clear ();
        }

        /** Initializes filter models before connecting user-driven signals. */
        private void configure_filters () {
            var categories = new Gtk.StringList (null);
            categories.append (_("All General Categories"));
            for (size_t index = 0; index < Search.category_count (); index++) {
                categories.append (Search.category_name (index) ?? "");
            }
            category_filter.model = categories;
            category_filter.selected = 0;
            rebuild_collection_filter ();
            scope_list.select_row (scope_list.get_row_at_index (0));
            scope_list.row_selected.connect (on_scope_selected);
            scope_list.row_activated.connect (on_source_activated);
            collection_filter.notify["selected"].connect (submit_search);
            category_filter.notify["selected"].connect (submit_search);
            search_entry.changed.connect (submit_search);
            split_view.bind_property ("show-sidebar", sidebar_toggle, "active",
                BindingFlags.BIDIRECTIONAL | BindingFlags.SYNC_CREATE);
        }

        /** Registers the window-local search focus action for the application accelerator. */
        private void configure_actions () {
            var focus_action = new SimpleAction ("search", null);
            focus_action.activate.connect (focus_search);
            add_action (focus_action);
            var reset_action = new SimpleAction ("reset-search", null);
            reset_action.activate.connect (reset_search);
            add_action (reset_action);
            var inspect_action = new SimpleAction ("inspect-character", null);
            inspect_action.activate.connect (inspect_selected);
            add_action (inspect_action);
            var emoji_action = new SimpleAction ("emoji-variations", null);
            emoji_action.activate.connect (show_emoji_variations);
            add_action (emoji_action);
            var identify_action = new SimpleAction ("identify-character", null);
            identify_action.activate.connect (identify_character);
            add_action (identify_action);
            var fonts_action = new SimpleAction ("browse-fonts", null);
            fonts_action.activate.connect (browse_fonts);
            add_action (fonts_action);
            var encodings_action = new SimpleAction ("browse-encodings", null);
            encodings_action.activate.connect (browse_encodings);
            add_action (encodings_action);
            var composer_action = new SimpleAction ("open-composer", null);
            composer_action.activate.connect (open_composer);
            add_action (composer_action);
            var add = new SimpleAction ("add-character", null);
            add.activate.connect (add_selected);
            add_action (add);
            var copy = new SimpleAction ("copy-character", null);
            copy.activate.connect (copy_selected);
            add_action (copy);
        }

        /** Opens the editor without modifying the current browser selection. */
        private void open_composer (Variant? parameter) {
            composer.open ();
        }

        /** Returns the exact selected UTF-8 identity or null for an unavailable text value. */
        private string? selected_text () {
            var item = selection.selected_item as Gtk.StringObject;
            return item == null ? null : Search.text ((uint32) uint.parse (item.get_string ()));
        }

        /** Appends the exact selected identity, keeping the browsing context intact. */
        private void add_selected (Variant? parameter) {
            var text = selected_text ();
            if (text != null) {
                composer.append_text (text);
            }
        }

        /** Copies a browser identity directly without opening the inspector. */
        private void copy_selected (Variant? parameter) {
            var text = selected_text ();
            if (text != null) {
                get_clipboard ().set_text (text);
                browser_toasts.add_toast (new Adw.Toast (_("Copied to Clipboard")));
            }
        }

        /** Opens the installed-font browser without replacing the main search. */
        private void browse_fonts (Variant? parameter) {
            new FontBrowser (false, uint32.MAX, composer).present (this);
        }

        /** Opens legacy character-set browsing without changing the selected Unicode collection. */
        private void browse_encodings (Variant? parameter) {
            new FontBrowser (true, uint32.MAX, composer).present (this);
        }

        /** The final two sidebar rows open independent source browsers, not native search scopes. */
        private void on_source_activated (Gtk.ListBoxRow row) {
            var index = row.get_index ();
            if (index == (int) Search.Scope.NAMED + 1) {
                browse_fonts (null);
            } else if (index == (int) Search.Scope.NAMED + 2) {
                browse_encodings (null);
            }
        }

        /** Opens a snapshot independent of the current search result or selection lifetime. */
        private void inspect_selected (Variant? parameter) {
            var item = selection.selected_item as Gtk.StringObject;
            if (item == null) {
                return;
            }
            var initial = Inspector.from_id ((uint32) uint.parse (item.get_string ()));
            if (initial != null) {
                var dialog = new CharacterInspector ((owned) initial, composer);
                dialog.present (this);
            }
        }

        /** Opens the variation picker for the selected exact sequence. */
        private void show_emoji_variations (Variant? parameter) {
            var item = selection.selected_item as Gtk.StringObject;
            if (item == null) {
                return;
            }
            var text = Search.text ((uint32) uint.parse (item.get_string ()));
            if (text != null && Emoji.is_candidate (text)) {
                new EmojiVariations (text, composer).present (this);
            }
        }

        /** Opens identification without requiring a character to be selected. */
        private void identify_character (Variant? parameter) {
            var dialog = new CharacterInspector (null, composer);
            dialog.present (this);
        }

        /** Focuses the query field without changing the selected collection. */
        private void focus_search (Variant? parameter) {
            search_entry.grab_focus ();
        }

        /** Clears the query and all collection/category constraints as one request. */
        private void reset_search (Variant? parameter) {
            updating_filters = true;
            scope = Search.Scope.ALL;
            scope_list.select_row (scope_list.get_row_at_index (0));
            rebuild_collection_filter ();
            category_filter.visible = true;
            category_filter.selected = 0;
            search_entry.text = "";
            updating_filters = false;
            submit_search ();
        }

        /** Changes scope and rebuilds valid filters without publishing an intermediate request. */
        private void on_scope_selected (Gtk.ListBoxRow? row) {
            if (row == null || updating_filters || row.get_index () > (int) Search.Scope.NAMED) {
                return;
            }
            scope = (Search.Scope) row.get_index ();
            updating_filters = true;
            rebuild_collection_filter ();
            category_filter.selected = 0;
            category_filter.visible = scope != Search.Scope.EMOJI && scope != Search.Scope.NAMED;
            updating_filters = false;
            if (split_view.collapsed) {
                split_view.show_sidebar = false;
            }
            submit_search ();
        }

        /** Builds a compact dropdown mapping, skipping unsupported ICU script slots. */
        private void rebuild_collection_filter () {
            var model = new Gtk.StringList (null);
            model.append (_("All Collections"));
            filter_slots = { -1 };
            for (size_t index = 0; index < Search.filter_count (scope); index++) {
                unowned string? label = Search.filter_name (scope, index);
                if (label != null) {
                    model.append (label);
                    filter_slots += (int) index;
                }
            }
            collection_filter.model = model;
            collection_filter.selected = 0;
            collection_filter.visible = filter_slots.length > 1;
        }

        /** Clears stale results, requests cancellation, and submits current controls. */
        private void submit_search () {
            if (updating_filters) {
                return;
            }
            var index = collection_filter.selected;
            var filter = index < (uint) filter_slots.length ? filter_slots[index] : -1;
            var category = category_filter.visible &&
                category_filter.selected != Gtk.INVALID_LIST_POSITION ?
                (int) category_filter.selected - 1 : -1;
            results = null;
            offset = 0;
            selection.model = null;
            clear_selection ();
            previous_button.sensitive = false;
            next_button.sensitive = false;
            result_count.label = _("Searching…");
            results_stack.visible_child_name = "loading";
            loading_spinner.start ();
            controller.submit (scope, filter, category, search_entry.text);
        }

        /** Receives only the latest request; restores a bounded page or a useful status state. */
        private void on_search_completed (SearchRequest request) {
            loading_spinner.stop ();
            if (request.get_status () != Search.Status.OK) {
                error_page.description = request.get_status () == Search.Status.INVALID ?
                    _("Enter valid code points or a search of up to 512 UTF-8 bytes and 64 words.") :
                    _("The search could not be completed. Change the query to try again.");
                result_count.label = _("Search Unavailable");
                results_stack.visible_child_name = "error";
                return;
            }
            results = request;
            if (request.count () == 0) {
                result_count.label = _("0 Characters");
                results_stack.visible_child_name = "empty";
                return;
            }
            results_stack.visible_child_name = "results";
            show_page ();
        }

        /** Replaces the model atomically with at most PAGE_SIZE identities and resets selection. */
        private void show_page () {
            if (results == null) {
                return;
            }
            var total = results.count ();
            var length = total - offset < PAGE_SIZE ? total - offset : PAGE_SIZE;
            var keys = new string[(int) length];
            for (uint index = 0; index < length; index++) {
                keys[index] = results.id (offset + index).to_string ();
            }
            selection.model = new Gtk.StringList (keys);
            selection.selected = Gtk.INVALID_LIST_POSITION;
            clear_selection ();
            grid_scroll.vadjustment.value = grid_scroll.vadjustment.lower;
            previous_button.sensitive = offset > 0;
            next_button.sensitive = offset + length < total;
            result_count.label = _("Showing %u–%u of %u").printf (
                offset + 1, offset + length, total);
        }

        /** Moves back one page, retaining the original immutable search results. */
        private void previous_page () {
            if (offset >= PAGE_SIZE) {
                offset -= PAGE_SIZE;
                show_page ();
            }
        }

        /** Moves forward only when the current request has more results. */
        private void next_page () {
            if (results != null && offset + PAGE_SIZE < results.count ()) {
                offset += PAGE_SIZE;
                show_page ();
            }
        }

        /** Updates the selection summary and enables the inspector for the selected identity. */
        private void update_selection () {
            var item = selection.selected_item as Gtk.StringObject;
            if (item == null) {
                clear_selection ();
                return;
            }
            var id = (uint32) uint.parse (item.get_string ());
            selected_name.label = Search.name (id) ?? _("Unknown Character");
            selected_code.label = Search.code (id) ?? "";
            inspect_button.sensitive = true;
            var text = Search.text (id);
            emoji_button.visible = text != null && Emoji.is_candidate (text);
            browser_copy.sensitive = text != null;
            browser_add.sensitive = text != null;
        }

        /** Removes the old selection summary before a new page or search is displayed. */
        private void clear_selection () {
            selected_name.label = _("No Character Selected");
            selected_code.label = "";
            inspect_button.sensitive = false;
            emoji_button.visible = false;
            browser_copy.sensitive = false;
            browser_add.sensitive = false;
        }

        /** Disconnects the window from the worker before cancellation; closing never joins GTK. */
        private void stop_search () {
            updating_filters = true;
            if (completion_handler != 0) {
                controller.disconnect (completion_handler);
                completion_handler = 0;
            }
            controller.shutdown ();
        }
    }
}
