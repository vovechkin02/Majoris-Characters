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

        /** Activating a cell with Enter or double-click selects its identity. */
        private void on_grid_activate (uint position) {
            selection.selected = position;
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
            if (row == null || updating_filters) {
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

        /** Updates the basic name/code selection summary; full inspector follows in Stage 4. */
        private void update_selection () {
            var item = selection.selected_item as Gtk.StringObject;
            if (item == null) {
                clear_selection ();
                return;
            }
            var id = (uint32) uint.parse (item.get_string ());
            selected_name.label = Search.name (id) ?? _("Unknown Character");
            selected_code.label = Search.code (id) ?? "";
        }

        /** Removes the old selection summary before a new page or search is displayed. */
        private void clear_selection () {
            selected_name.label = _("No Character Selected");
            selected_code.label = "";
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
