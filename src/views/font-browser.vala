namespace Characters {
    /** Presentation modes share the same grid but never confuse face IDs with scalar IDs. */
    internal enum FontBrowserMode {
        /** Explore the actual mapped repertoire of an installed face. */
        FONTS,
        /** Explore strict byte mappings for one selected legacy character set. */
        ENCODINGS,
        /** Compare one selected scalar across all installed faces mapping it. */
        VARIATIONS
    }

    /** Coordinates installed-font browsing, legacy mappings and bounded font comparison pages. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/font-browser.ui")]
    internal sealed class FontBrowser : Adw.Dialog {
        /** Bound on model entries in a mapped-character page. */
        private const uint PAGE_SIZE = 256;
        /** Smaller page for font comparisons, which resolve independent face descriptions. */
        private const uint VARIATION_PAGE_SIZE = 48;
        /** Seconds between installed-font freshness checks while this dialog is open. */
        private const uint REFRESH_SECONDS = 30;

        /** Confirms explicit clipboard actions. */
        [GtkChild]
        private unowned Adw.ToastOverlay font_toasts;
        /** Chooses the active browsing source. */
        [GtkChild]
        private unowned Gtk.DropDown source_mode;
        /** Searchable installed family selection. */
        [GtkChild]
        private unowned Gtk.DropDown family_filter;
        /** Searchable installed style and file selection. */
        [GtkChild]
        private unowned Gtk.DropDown face_filter;
        /** Family and face controls, hidden outside installed-font browsing. */
        [GtkChild]
        private unowned Gtk.Box face_controls;
        /** Requests an immediate inventory reload. */
        [GtkChild]
        private unowned Gtk.Button refresh_fonts;
        /** Chooses an explicitly supported legacy encoding. */
        [GtkChild]
        private unowned Gtk.DropDown encoding_filter;
        /** Searches the current scalar repertoire or comparison face names. */
        [GtkChild]
        private unowned Gtk.SearchEntry font_query;
        /** Restricts the current scalar repertoire by Unicode category. */
        [GtkChild]
        private unowned Gtk.DropDown font_category;
        /** Controls glyph preview size without changing the character repertoire. */
        [GtkChild]
        private unowned Gtk.SpinButton preview_size;
        /** Allows Pango fallback during preview, never during coverage inspection. */
        [GtkChild]
        private unowned Gtk.CheckButton font_fallback;
        /** Explains the source and the distinction between coverage and rendering. */
        [GtkChild]
        private unowned Gtk.Label source_note;
        /** Virtualized cells for either mapped scalars or face comparisons. */
        [GtkChild]
        private unowned Gtk.GridView font_grid;
        /** Restores scroll position at a page boundary. */
        [GtkChild]
        private unowned Gtk.ScrolledWindow font_scroll;
        /** Reports loading, errors, empty results and page counts. */
        [GtkChild]
        private unowned Gtk.Label font_status;
        /** Animated only while source discovery or search is pending. */
        [GtkChild]
        private unowned Gtk.Spinner font_spinner;
        /** Navigates to the previous bounded page. */
        [GtkChild]
        private unowned Gtk.Button font_previous;
        /** Navigates to the next bounded page. */
        [GtkChild]
        private unowned Gtk.Button font_next;
        /** Selected Unicode identity or font variation. */
        [GtkChild]
        private unowned Gtk.Label font_selection;
        /** Selected font file or legacy hexadecimal mapping. */
        [GtkChild]
        private unowned Gtk.Label font_detail;
        /** Opens the existing character inspector. */
        [GtkChild]
        private unowned Gtk.Button font_inspect;
        /** Copies exact Unicode text, never the rendered placeholder. */
        [GtkChild]
        private unowned Gtk.Button font_copy;
        /** Copies all exact hexadecimal alternatives in the current encoding. */
        [GtkChild]
        private unowned Gtk.Button bytes_copy;

        /** Adds the mapped scalar, never the encoded bytes or the preview placeholder. */
        [GtkChild]
        private unowned Gtk.Button font_add;
        /** Non-owning editor reference passed on to the inspector. */
        private weak ComposerView? composer;

        /** Serializes source discovery and extraction without blocking GTK. */
        private FontSourceLoader loader = new FontSourceLoader ();
        /** Reuses cancellable, latest-request-only native Unicode search. */
        private SearchController controller = new SearchController ();
        /** Last successfully discovered immutable installed-face inventory. */
        private FontInventory? inventory;
        /** Current scalar repertoire, never inferred from Pango fallback. */
        private uint32[]? repertoire;
        /** Current strict legacy byte mapping, independent of the Unicode search request. */
        private Legacy.Table? mapping;
        /** Current successful scalar search, with bounded grid pages. */
        private SearchRequest? results;
        /** Family dropdown offsets into the sorted inventory, ending with a sentinel. */
        private uint[] family_starts = {};
        /** Count of initialized family offsets, excluding the sentinel. */
        private uint family_count = 0;
        /** Face index in the current immutable catalog, or GTK's invalid position. */
        private uint current_face = Gtk.INVALID_LIST_POSITION;
        /** Immutable scalar requested by the inspector for font comparisons. */
        private uint32 compare_point;
        /** Filtered face IDs for the comparison mode. */
        private uint[] comparison_ids = {};
        /** Number of valid comparison identities in the preallocated vector. */
        private uint comparison_count = 0;
        /** GTK model selection and cell recycling lifetime. */
        private Gtk.SingleSelection selection;
        /** Active source; keys have distinct interpretations in each mode. */
        private FontBrowserMode mode = FontBrowserMode.FONTS;
        /** First result index in the visible page. */
        private uint offset = 0;
        /** Suppresses signal recursion while replacing dropdowns and models. */
        private bool updating = false;
        /** Blocks preview and source changes during an explicit inventory reload. */
        private bool inventory_refreshing = false;
        /** Permanent close flag; no callbacks may touch widgets afterward. */
        private bool stopped = false;
        /** Main-context refresh timer, removed on dialog close. */
        private uint refresh_timer = 0;
        /** Source publication handler, disconnected before shutdown. */
        private ulong source_handler = 0;
        /** Search publication handler, disconnected before shutdown. */
        private ulong search_handler = 0;

        /** Opens the font or encoding browser, optionally comparing a selected Unicode scalar. */
        public FontBrowser (bool legacy_mode = false, uint32 compare_point = uint32.MAX,
                            ComposerView? composer = null) {
            this.composer = composer;
            font_add.visible = composer != null;
            font_add.sensitive = false;
            font_add.clicked.connect (add_to_composer);
            this.compare_point = compare_point;
            configure_grid ();
            configure_controls (legacy_mode);
            source_handler = loader.completed.connect (on_source_completed);
            search_handler = controller.completed.connect (on_search_completed);
            closed.connect (stop);
            refresh_timer = Timeout.add_seconds (REFRESH_SECONDS, check_refresh);
            change_mode ();
        }

        /** Releases workers and timer even if the dialog is disposed without a close signal. */
        public override void dispose () {
            stop ();
            base.dispose ();
        }

        /** Initializes a recyclable grid using the same accessible character tile as the browser. */
        private void configure_grid () {
            selection = new Gtk.SingleSelection (null);
            selection.autoselect = false;
            selection.can_unselect = true;
            selection.notify["selected-item"].connect (update_selection);
            var factory = new Gtk.SignalListItemFactory ();
            factory.setup.connect (setup_tile);
            factory.bind.connect (bind_tile);
            factory.unbind.connect (unbind_tile);
            font_grid.factory = factory;
            font_grid.model = selection;
            font_grid.activate.connect (activate_tile);
        }

        /** Initializes dropdowns before installing callbacks for user-driven changes. */
        private void configure_controls (bool legacy_mode) {
            var modes = new Gtk.StringList ({ _("Installed Fonts"), _("Legacy Character Sets") });
            if (compare_point != uint32.MAX) {
                modes.append (_("Font Variations"));
            }
            source_mode.model = modes;
            source_mode.selected = compare_point != uint32.MAX ? 2u : (legacy_mode ? 1u : 0u);
            var encodings = new Gtk.StringList (null);
            for (size_t index = 0; index < Legacy.count (); index++) {
                encodings.append (Legacy.name (index) ?? "");
            }
            encoding_filter.model = encodings;
            encoding_filter.selected = 0;
            var categories = new Gtk.StringList ({ _("All General Categories") });
            for (size_t index = 0; index < Search.category_count (); index++) {
                categories.append (Search.category_name (index) ?? "");
            }
            font_category.model = categories;
            font_category.selected = 0;
            source_mode.notify["selected"].connect (change_mode);
            family_filter.notify["selected"].connect (change_family);
            face_filter.notify["selected"].connect (change_face);
            encoding_filter.notify["selected"].connect (change_encoding);
            font_query.changed.connect (submit_search);
            font_category.notify["selected"].connect (submit_search);
            preview_size.value_changed.connect (refresh_previews);
            font_fallback.toggled.connect (refresh_previews);
            refresh_fonts.clicked.connect (force_refresh);
            font_previous.clicked.connect (previous_page);
            font_next.clicked.connect (next_page);
            font_inspect.clicked.connect (inspect_selected);
            font_copy.clicked.connect (copy_character);
            bytes_copy.clicked.connect (copy_bytes);
        }

        /** Changes sources atomically, invalidating any old selection and queued publication. */
        private void change_mode () {
            if (updating || stopped) {
                return;
            }
            loader.cancel ();
            controller.cancel ();
            clear_page ();
            updating = true;
            inventory_refreshing = false;
            face_controls.sensitive = true;
            mode = (FontBrowserMode) source_mode.selected;
            face_controls.visible = mode == FontBrowserMode.FONTS;
            encoding_filter.visible = mode == FontBrowserMode.ENCODINGS;
            refresh_fonts.visible = mode != FontBrowserMode.ENCODINGS;
            font_category.visible = mode != FontBrowserMode.VARIATIONS;
            bytes_copy.visible = mode == FontBrowserMode.ENCODINGS;
            font_fallback.visible = mode != FontBrowserMode.ENCODINGS;
            font_category.selected = 0;
            font_query.text = "";
            font_query.placeholder_text = mode == FontBrowserMode.VARIATIONS ?
                _("Search font families and faces") : mode == FontBrowserMode.ENCODINGS ?
                _("Search by name, code point, or byte:80") :
                _("Search characters by name or code point");
            repertoire = null;
            mapping = null;
            comparison_ids = {};
            comparison_count = 0;
            updating = false;
            if (mode == FontBrowserMode.ENCODINGS) {
                change_encoding ();
            } else if (inventory == null) {
                load_inventory (true, true);
            } else if (mode == FontBrowserMode.VARIATIONS) {
                submit_search ();
            } else {
                rebuild_families (null, -1);
            }
        }

        /** Loads the installed inventory; a periodic check retains current results until changed. */
        private void load_inventory (bool force, bool show_loading) {
            if (show_loading) {
                inventory_refreshing = true;
                face_controls.sensitive = false;
                comparison_count = 0;
                controller.cancel ();
                repertoire = null;
                clear_page ();
                set_loading (_("Discovering Installed Fonts…"));
                source_note.label = _("Fontconfig supplies the installed font inventory.");
            }
            loader.submit (new FontSourceRequest.for_inventory (inventory, force));
        }

        /** Checks freshness only when no source operation is outstanding. */
        private bool check_refresh () {
            if (stopped) {
                return Source.REMOVE;
            }
            if (mode != FontBrowserMode.ENCODINGS && inventory != null && loader.is_idle ()) {
                load_inventory (false, false);
            }
            return Source.CONTINUE;
        }

        /** Manually refreshes font discovery without requiring an application restart. */
        private void force_refresh () {
            if (!stopped && mode != FontBrowserMode.ENCODINGS) {
                load_inventory (true, true);
            }
        }

        /** Consumes only the newest source operation and transfers its immutable results. */
        private void on_source_completed (FontSourceRequest request, bool success) {
            if (stopped) {
                return;
            }
            if (request.kind == FontSourceKind.INVENTORY) {
                inventory_refreshing = false;
                face_controls.sensitive = true;
            }
            if (!success) {
                if (request.kind == FontSourceKind.INVENTORY) {
                    inventory = null;
                    current_face = Gtk.INVALID_LIST_POSITION;
                    family_count = 0;
                    updating = true;
                    family_filter.model = null;
                    face_filter.model = null;
                    updating = false;
                }
                comparison_count = 0;
                controller.cancel ();
                clear_page ();
                repertoire = null;
                set_status (request.kind == FontSourceKind.ENCODING ?
                    _("The selected encoding is unavailable in ICU.") :
                    _("Font discovery or coverage failed. Refresh Fonts to retry."));
                return;
            }
            switch (request.kind) {
                case FontSourceKind.INVENTORY:
                    if (request.inventory != null && request.inventory != inventory) {
                        apply_inventory (request.inventory);
                    }
                    break;
                case FontSourceKind.FACE:
                    repertoire = request.take_points ();
                    update_face_note ();
                    submit_search ();
                    break;
                case FontSourceKind.ENCODING:
                    repertoire = request.take_points ();
                    mapping = request.take_table ();
                    submit_search ();
                    break;
            }
        }

        /** Replaces discovery atomically and preserves a still-installed file/face selection. */
        private void apply_inventory (FontInventory next) {
            string? previous_file = null;
            int previous_index = -1;
            if (inventory != null && current_face != Gtk.INVALID_LIST_POSITION) {
                unowned Fonts.Catalog old = inventory.get_catalog ();
                previous_file = old.file (current_face);
                previous_index = old.face_index (current_face);
            }
            controller.cancel ();
            clear_page ();
            repertoire = null;
            inventory = next;
            unowned Fonts.Catalog catalog = next.get_catalog ();
            catalog.apply_config (get_pango_context ().get_font_map ());
            current_face = Gtk.INVALID_LIST_POSITION;
            if (mode == FontBrowserMode.VARIATIONS) {
                submit_search ();
            } else {
                rebuild_families (previous_file, previous_index);
            }
        }

        /** Groups sorted exact faces by primary family and restores a matching face if possible. */
        private void rebuild_families (string? previous_file, int previous_index) {
            if (inventory == null) {
                return;
            }
            updating = true;
            unowned Fonts.Catalog catalog = inventory.get_catalog ();
            var families = new Gtk.StringList (null);
            family_starts = new uint[(int) catalog.count () + 1];
            family_count = 0;
            uint preferred_family = 0;
            uint preferred_face = 0;
            string last = "";
            for (uint index = 0; index < catalog.count (); index++) {
                unowned string? family = catalog.family (index);
                if (family_count == 0 || family != last) {
                    family_starts[family_count++] = index;
                    families.append (family ?? _("Unknown Font"));
                    last = family ?? "";
                }
                if (previous_file != null && previous_file == catalog.file (index) &&
                    previous_index == catalog.face_index (index)) {
                    preferred_family = family_count - 1;
                    preferred_face = index - family_starts[preferred_family];
                }
            }
            family_starts[family_count] = (uint) catalog.count ();
            family_filter.model = families;
            family_filter.selected = family_count == 0 ? Gtk.INVALID_LIST_POSITION :
                preferred_family;
            rebuild_faces (preferred_face);
            updating = false;
            if (family_count == 0) {
                set_status (_("No Installed Font Faces Found"));
                source_note.label = _("Install a font using the system font manager, then refresh.");
                return;
            }
            change_face ();
        }

        /** Replaces the face dropdown for a single family without intermediate requests. */
        private void rebuild_faces (uint preferred) {
            var faces = new Gtk.StringList (null);
            if (inventory != null && family_filter.selected < family_count) {
                unowned Fonts.Catalog catalog = inventory.get_catalog ();
                var start = family_starts[family_filter.selected];
                var end = family_starts[family_filter.selected + 1];
                for (uint index = start; index < end; index++) {
                    var file = catalog.file (index) ?? "";
                    faces.append ("%s — %s [%d]".printf (catalog.style (index) ?? _("Regular"),
                        Filename.display_basename (file), catalog.face_index (index)));
                }
            }
            face_filter.model = faces;
            face_filter.selected = preferred < faces.get_n_items () ? preferred :
                (faces.get_n_items () > 0 ? 0 : Gtk.INVALID_LIST_POSITION);
        }

        /** Selects a new family and requests its first exact face. */
        private void change_family () {
            if (updating || stopped || inventory_refreshing || mode != FontBrowserMode.FONTS) {
                return;
            }
            updating = true;
            rebuild_faces (0);
            updating = false;
            change_face ();
        }

        /** Extracts the selected face's actual cmap coverage on a source worker. */
        private void change_face () {
            if (updating || stopped || inventory_refreshing || mode != FontBrowserMode.FONTS ||
                inventory == null ||
                family_filter.selected >= family_count ||
                face_filter.selected == Gtk.INVALID_LIST_POSITION) {
                return;
            }
            var index = family_starts[family_filter.selected] + face_filter.selected;
            if (index >= family_starts[family_filter.selected + 1]) {
                return;
            }
            current_face = index;
            controller.cancel ();
            repertoire = null;
            clear_page ();
            set_loading (_("Reading Font Coverage…"));
            source_note.label = _("Coverage uses the selected face's Unicode cmap without fallback.");
            loader.submit (new FontSourceRequest.for_face (inventory, current_face));
        }

        /** Builds the strict legacy byte table, allowing the newest selection to supersede it. */
        private void change_encoding () {
            if (updating || stopped || mode != FontBrowserMode.ENCODINGS ||
                encoding_filter.selected >= Legacy.count ()) {
                return;
            }
            controller.cancel ();
            repertoire = null;
            mapping = null;
            clear_page ();
            source_note.label = _("ICU byte mappings; search byte:80 or byte:82 A0 for exact bytes. " +
                "Undefined, truncated and multi-scalar mappings are omitted.");
            set_loading (_("Reading Character Set…"));
            loader.submit (new FontSourceRequest.for_encoding (encoding_filter.selected));
        }

        /** Submits native repertoire search or filters installed comparison faces by their names. */
        private void submit_search () {
            if (updating || stopped || inventory_refreshing) {
                return;
            }
            if (mode == FontBrowserMode.VARIATIONS) {
                rebuild_comparisons ();
                return;
            }
            if (repertoire == null) {
                return;
            }
            controller.cancel ();
            clear_page ();
            var query = font_query.text;
            var byte_query = query.strip ();
            if (mode == FontBrowserMode.ENCODINGS && byte_query.down ().has_prefix ("byte:")) {
                var point = mapping != null ? mapping.decode (byte_query.substring (5).strip ()) :
                    uint32.MAX;
                if (point == uint32.MAX) {
                    set_status (_("Invalid or Unmapped Byte Sequence"));
                    return;
                }
                query = "U+%04X".printf (point);
            }
            var category = font_category.selected == Gtk.INVALID_LIST_POSITION ? -1 :
                (int) font_category.selected - 1;
            set_loading (_("Searching…"));
            controller.submit_points (repertoire, category, query);
        }

        /** Builds compact comparison IDs using exact scalar coverage, not successful fallback. */
        private void rebuild_comparisons () {
            if (inventory == null) {
                return;
            }
            clear_page ();
            unowned Fonts.Catalog catalog = inventory.get_catalog ();
            comparison_ids = new uint[(int) catalog.count ()];
            comparison_count = 0;
            var query = font_query.text.strip ().casefold ();
            for (uint index = 0; index < catalog.count (); index++) {
                if (!catalog.contains (index, compare_point)) {
                    continue;
                }
                if (query != "" && !face_name (catalog, index).casefold ().contains (query)) {
                    continue;
                }
                comparison_ids[comparison_count++] = index;
            }
            source_note.label = _("%s — compare faces that map this scalar. " +
                "Sequence shaping is not inferred from scalar coverage.").printf (
                Search.code (compare_point) ?? "");
            show_page ();
        }

        /** Receives only the latest successful native search and displays its first page. */
        private void on_search_completed (SearchRequest request) {
            if (stopped || mode == FontBrowserMode.VARIATIONS || repertoire == null) {
                return;
            }
            if (request.get_status () != Search.Status.OK) {
                set_status (request.get_status () == Search.Status.INVALID ?
                    _("Invalid Search Query") : _("Search Unavailable"));
                return;
            }
            results = request;
            show_page ();
        }

        /** Returns the correct page capacity for this mode's rendering cost. */
        private uint page_size () {
            return mode == FontBrowserMode.VARIATIONS ? VARIATION_PAGE_SIZE : PAGE_SIZE;
        }

        /** Counts only results belonging to the active mode. */
        private uint result_total () {
            return mode == FontBrowserMode.VARIATIONS ? comparison_count :
                (results != null ? results.count () : 0);
        }

        /** Replaces the model with a bounded page; the grid creates only visible widgets. */
        private void show_page () {
            var total = result_total ();
            if (total == 0) {
                set_status (mode == FontBrowserMode.VARIATIONS ? _("No Matching Font Faces") :
                    _("No Matching Characters"));
                return;
            }
            var capacity = page_size ();
            var length = total - offset < capacity ? total - offset : capacity;
            var keys = new string[(int) length];
            for (uint index = 0; index < length; index++) {
                var id = mode == FontBrowserMode.VARIATIONS ? comparison_ids[offset + index] :
                    results.id (offset + index);
                keys[index] = id.to_string ();
            }
            selection.model = new Gtk.StringList (keys);
            selection.selected = Gtk.INVALID_LIST_POSITION;
            clear_selection ();
            font_scroll.vadjustment.value = font_scroll.vadjustment.lower;
            font_previous.sensitive = offset > 0;
            font_next.sensitive = offset + length < total;
            set_status (_("Showing %u–%u of %u").printf (offset + 1, offset + length, total));
        }

        /** Clears source-dependent selection and scalar results before any replacement. */
        private void clear_page () {
            results = null;
            offset = 0;
            selection.model = null;
            font_previous.sensitive = false;
            font_next.sensitive = false;
            clear_selection ();
        }

        /** Requests a spinner and message without changing the active source. */
        private void set_loading (string message) {
            font_spinner.visible = true;
            font_spinner.start ();
            font_status.label = message;
        }

        /** Ends loading and displays the exact outcome of the most recent operation. */
        private void set_status (string message) {
            font_spinner.stop ();
            font_spinner.visible = false;
            font_status.label = message;
        }

        /** Moves to the preceding page without rerunning the source or search. */
        private void previous_page () {
            if (offset >= page_size ()) {
                offset -= page_size ();
                show_page ();
            }
        }

        /** Moves to the next page of the current immutable results. */
        private void next_page () {
            if (offset + page_size () < result_total ()) {
                offset += page_size ();
                show_page ();
            }
        }

        /** Rebinds only the current page when preview preferences change, retaining selection. */
        private void refresh_previews () {
            if (updating || stopped || selection.model == null) {
                return;
            }
            var selected = selection.selected;
            update_face_note ();
            show_page ();
            selection.selected = selected;
        }

        /** Creates one reusable tile, independent of the represented identity. */
        private static void setup_tile (GLib.Object object) {
            var item = (Gtk.ListItem) object;
            item.child = new CharacterTile ();
        }

        /** Renders a recycled tile for the current mode using escaped preview descriptions. */
        private void bind_tile (GLib.Object object) {
            var item = (Gtk.ListItem) object;
            var value = (Gtk.StringObject) item.item;
            var tile = (CharacterTile) item.child;
            var id = (uint32) uint.parse (value.get_string ());
            var point = mode == FontBrowserMode.VARIATIONS ? compare_point : id;
            var caption = Search.code (point) ?? "";
            var detail = "%s\n%s".printf (Search.name (point) ?? _("Unknown Character"), caption);
            string? description = null;
            bool fallback = mode == FontBrowserMode.ENCODINGS || font_fallback.active;
            if (mode != FontBrowserMode.ENCODINGS && inventory != null) {
                unowned Fonts.Catalog catalog = inventory.get_catalog ();
                var face = mode == FontBrowserMode.VARIATIONS ? id : current_face;
                description = catalog.description (face);
                if (mode == FontBrowserMode.VARIATIONS) {
                    caption = face_name (catalog, face);
                    detail = caption + "\n" + detail;
                    if (!matches_preview (catalog, face, description)) {
                        caption += _(" (Substituted)");
                        detail += "\n" + _("Renderer resolved another face.");
                    }
                }
            } else if (mapping != null) {
                var bytes = mapping.bytes (point);
                if (bytes != null) {
                    detail += "\n" + bytes;
                    caption = bytes;
                }
            }
            tile.bind_preview (point, description, preview_size.get_value_as_int (), fallback,
                caption, detail, mode == FontBrowserMode.VARIATIONS);
        }

        /** Releases stale accessibility and display data before a cell is recycled. */
        private static void unbind_tile (GLib.Object object) {
            var tile = (CharacterTile) ((Gtk.ListItem) object).child;
            tile.clear ();
        }

        /** Activating a tile selects and inspects its Unicode identity, not a font index. */
        private void activate_tile (uint position) {
            selection.selected = position;
            inspect_selected ();
        }

        /** Obtains the selected compact scalar or face identity, with an explicit invalid value. */
        private uint32 selected_id () {
            var item = selection.selected_item as Gtk.StringObject;
            return item == null ? uint32.MAX : (uint32) uint.parse (item.get_string ());
        }

        /** Obtains the actual selected scalar even in font comparison mode. */
        private uint32 selected_point () {
            var id = selected_id ();
            return id == uint32.MAX ? id : mode == FontBrowserMode.VARIATIONS ? compare_point : id;
        }

        /** Provides stable family/style labels from the installed native inventory. */
        private static string face_name (Fonts.Catalog catalog, uint face) {
            return "%s — %s".printf (catalog.family (face) ?? _("Unknown Font"),
                catalog.style (face) ?? _("Regular"));
        }

        /** Verifies the nominal Pango face identity without treating fallback as coverage. */
        private bool matches_preview (Fonts.Catalog catalog, uint face, string? description) {
            if (description == null) {
                return false;
            }
            var requested = Pango.FontDescription.from_string (description);
            requested.set_size (preview_size.get_value_as_int () * Pango.SCALE);
            var resolved = get_pango_context ().load_font (requested);
            return catalog.matches (face, resolved);
        }

        /** Explains coverage and renderer substitution even before a tile is selected. */
        private void update_face_note () {
            if (mode != FontBrowserMode.FONTS || inventory == null || repertoire == null) {
                return;
            }
            unowned Fonts.Catalog catalog = inventory.get_catalog ();
            var matched = matches_preview (catalog, current_face,
                catalog.description (current_face));
            source_note.label = _("%d mapped scalars. Coverage excludes fallback; " +
                "preview uses %s. %s").printf (repertoire.length,
                font_fallback.active ? _("font fallback") : _("the requested font without fallback"),
                matched ? "" : _("The renderer resolved a different face or no face."));
        }

        /** Shows exact mapped identity, face metadata, and whether the nominal preview resolved. */
        private void update_selection () {
            var point = selected_point ();
            if (point == uint32.MAX) {
                clear_selection ();
                return;
            }
            font_selection.label = "%s — %s".printf (Search.name (point) ?? _("Unknown Character"),
                Search.code (point) ?? "");
            font_inspect.sensitive = true;
            font_copy.sensitive = Search.text (point) != null;
            font_add.sensitive = font_copy.sensitive && composer != null;
            bytes_copy.sensitive = false;
            if (mode == FontBrowserMode.ENCODINGS && mapping != null) {
                var bytes = mapping.bytes (point);
                font_detail.label = _("Encoded bytes: %s").printf (bytes ?? _("Unavailable"));
                bytes_copy.sensitive = bytes != null;
            } else if (inventory != null) {
                unowned Fonts.Catalog catalog = inventory.get_catalog ();
                var face = mode == FontBrowserMode.VARIATIONS ? selected_id () : current_face;
                var flags = _("Variable: %s · Color glyphs: %s").printf (
                    catalog.is_variable (face) ? _("Yes") : _("No"),
                    catalog.is_color (face) ? _("Yes") : _("No"));
                var matches = matches_preview (catalog, face, catalog.description (face));
                font_detail.label = "%s\n%s [%d]\n%s\n%s".printf (face_name (catalog, face),
                    Filename.display_name (catalog.file (face) ?? ""), catalog.face_index (face), flags,
                    matches ? _("Nominal preview face matches the installed file.") :
                    _("Preview substitution: renderer resolved a different face or no face."));
            }
        }

        /** Removes selection-dependent commands and metadata before any source or model change. */
        private void clear_selection () {
            font_selection.label = _("No Character Selected");
            font_detail.label = "";
            font_inspect.sensitive = false;
            font_copy.sensitive = false;
            font_add.sensitive = false;
            bytes_copy.sensitive = false;
        }

        /** Opens the existing inspector for the exact scalar, including unassigned font mappings. */
        private void inspect_selected () {
            var point = selected_point ();
            if (point == uint32.MAX) {
                return;
            }
            var snapshot = Inspector.from_code ("U+%04X".printf (point));
            if (snapshot != null) {
                new CharacterInspector ((owned) snapshot, composer).present (this);
            }
        }

        /** Copies the actual scalar, never the font preview, on explicit user action. */
        private void copy_character () {
            var text = Search.text (selected_point ());
            if (text != null) {
                copy_text (text);
            }
        }

        /** Appends a mapped scalar to the session editor without changing source selection. */
        private void add_to_composer () {
            var text = Search.text (selected_point ());
            if (text != null && composer != null) {
                composer.append_text (text);
                font_toasts.add_toast (new Adw.Toast (_("Added to Composer")));
            }
        }

        /** Copies exact byte notation, not a lossy conversion into the system clipboard format. */
        private void copy_bytes () {
            if (mapping != null && mode == FontBrowserMode.ENCODINGS) {
                var bytes = mapping.bytes (selected_point ());
                if (bytes != null) {
                    copy_text (bytes);
                }
            }
        }

        /** Uses the native display clipboard and confirms the completed local action. */
        private void copy_text (string text) {
            get_clipboard ().set_text (text);
            font_toasts.add_toast (new Adw.Toast (_("Copied to Clipboard")));
        }

        /** Disconnects callbacks and releases timers before closing; workers are never joined here. */
        private void stop () {
            if (stopped) {
                return;
            }
            stopped = true;
            if (refresh_timer != 0) {
                Source.remove (refresh_timer);
                refresh_timer = 0;
            }
            if (source_handler != 0) {
                loader.disconnect (source_handler);
                source_handler = 0;
            }
            if (search_handler != 0) {
                controller.disconnect (search_handler);
                search_handler = 0;
            }
            loader.shutdown ();
            controller.shutdown ();
        }
    }
}
