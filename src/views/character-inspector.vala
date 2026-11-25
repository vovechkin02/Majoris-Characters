namespace Characters {
    /** Presents an independent, bounded Unicode sequence and its technical representations. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/character-inspector.ui")]
    internal sealed class CharacterInspector : Adw.Dialog {
        /** Maximum number of scalar components accepted by the native engine. */
        private const int MAX_POINTS = 32;
        /** Maximum input characters extracted from the text buffer for explicit notation. */
        private const int MAX_INPUT = 512;
        /** Maximum number of previous snapshots retained as code notation. */
        private const uint HISTORY_LIMIT = 32;

        /** Displays confirmation for explicit clipboard operations. */
        [GtkChild]
        private unowned Adw.ToastOverlay toast_overlay;
        /** Opens the text/code identification workflow without replacing browser results. */
        [GtkChild]
        private unowned Gtk.Expander identify_expander;
        /** Multiline source input, including whitespace and combining characters. */
        [GtkChild]
        private unowned Gtk.TextView identify_input;
        /** Reports invalid input without replacing the current snapshot. */
        [GtkChild]
        private unowned Gtk.Label input_error;
        /** Hidden until there is a valid sequence to inspect. */
        [GtkChild]
        private unowned Gtk.Box details;
        /** Large safe glyph preview; never used as the clipboard source. */
        [GtkChild]
        private unowned Gtk.Label preview;
        /** Canonical name or standardized sequence name where available. */
        [GtkChild]
        private unowned Gtk.Label identity_name;
        /** Complete sequence code notation. */
        [GtkChild]
        private unowned Gtk.Label identity_code;
        /** Explains nonprinting previews, missing glyphs and embedded NUL limitations. */
        [GtkChild]
        private unowned Gtk.Label preview_note;
        /** Opens catalog-supported emoji alternatives for the current snapshot. */
        [GtkChild]
        private unowned Gtk.Button emoji_button;
        /** Copies the exact original sequence only when representable as GTK text. */
        [GtkChild]
        private unowned Gtk.Button copy_character;
        /** Adds the exact inspected text to the main-window editor. */
        [GtkChild]
        private unowned Gtk.Button inspect_add;
        /** Selects one constituent scalar for detailed property inspection. */
        [GtkChild]
        private unowned Gtk.DropDown component_filter;
        /** Rows for the selected component, never a misleading sequence-level property. */
        [GtkChild]
        private unowned Gtk.Box property_rows;
        /** Complete sequence encoding representations. */
        [GtkChild]
        private unowned Gtk.Box encoding_rows;
        /** Normalized code-point representations and optional raw text actions. */
        [GtkChild]
        private unowned Gtk.Box normalization_rows;
        /** Navigable related sequence identities. */
        [GtkChild]
        private unowned Gtk.Box related_rows;
        /** Independent catalog, property and supplement version provenance. */
        [GtkChild]
        private unowned Gtk.Label version_note;
        /** Back navigation reflects the bounded history. */
        [GtkChild]
        private unowned Gtk.Button back_button;

        /** Non-owning reference to the session editor; absent in standalone inspectors. */
        private weak ComposerView? composer;
        /** Optional shared local collection repository. */
        private LibraryStore? library;
        /** Saves the exact selected identity into Favorites. */
        [GtkChild]
        private unowned Gtk.Button inspect_favorite;
        /** Owns the inspected identity independently of the browser and its worker. */
        private Inspector.Snapshot? snapshot;
        /** Code notation for previously inspected identities; never persists on disk. */
        private GLib.Queue<string> history = new GLib.Queue<string> ();
        /** Prevents component callbacks during an atomic snapshot replacement. */
        private bool updating = false;
        /** Plain-text metadata for Copy Information, including every component. */
        private string report = "";

        /** Constructs an XML inspector, optionally taking ownership of a selected identity. */
        public CharacterInspector (owned Inspector.Snapshot? initial = null,
                                   ComposerView? composer = null, LibraryStore? library = null) {
            this.composer = composer;
            this.library = library;
            inspect_favorite.visible = library != null;
            inspect_favorite.sensitive = false;
            inspect_favorite.clicked.connect (save_favorite);
            inspect_add.visible = composer != null;
            inspect_add.sensitive = false;
            inspect_add.clicked.connect (add_to_composer);
            configure_actions ();
            component_filter.notify["selected"].connect (update_component);
            if (initial != null) {
                show_snapshot ((owned) initial, false);
            } else {
                identify_expander.expanded = true;
            }
        }

        /** Registers immediate native actions used by both static and generated rows. */
        private void configure_actions () {
            var actions = new SimpleActionGroup ();
            var copy = new SimpleAction ("copy", VariantType.STRING);
            copy.activate.connect (copy_value);
            actions.add_action (copy);
            var original = new SimpleAction ("copy-character", null);
            original.activate.connect (copy_original);
            actions.add_action (original);
            var information = new SimpleAction ("copy-information", null);
            information.activate.connect (copy_information);
            actions.add_action (information);
            var related = new SimpleAction ("related", VariantType.STRING);
            related.activate.connect (navigate_related);
            actions.add_action (related);
            var back = new SimpleAction ("back", null);
            back.activate.connect (navigate_back);
            actions.add_action (back);
            var text = new SimpleAction ("identify-text", null);
            text.activate.connect (identify_text);
            actions.add_action (text);
            var code = new SimpleAction ("identify-code", null);
            code.activate.connect (identify_code);
            actions.add_action (code);
            var fonts = new SimpleAction ("font-variations", null);
            fonts.activate.connect (show_font_variations);
            actions.add_action (fonts);
            var emoji = new SimpleAction ("emoji-variations", null);
            emoji.activate.connect (show_emoji_variations);
            actions.add_action (emoji);
            insert_action_group ("inspect", actions);
        }

        /** Reports a failed identification without changing the currently inspected identity. */
        private void show_input_error () {
            input_error.label = _("Enter 1–32 valid scalars or explicit U+, 0x, or dec: values. " +
                "Surrogates are not valid scalars.");
            input_error.visible = true;
        }

        /** Reads exact input, rejecting oversized buffers before allocating a copied string. */
        private string? read_input (int limit) {
            var buffer = identify_input.buffer;
            if (buffer.get_char_count () > limit) {
                return null;
            }
            Gtk.TextIter start;
            Gtk.TextIter end;
            buffer.get_bounds (out start, out end);
            return buffer.get_text (start, end, true);
        }

        /** Identifies literal text with no whitespace trimming or normalization. */
        private void identify_text (Variant? parameter) {
            var text = read_input (MAX_POINTS);
            if (text == null) {
                show_input_error ();
                return;
            }
            var next = Inspector.from_text (text);
            if (next == null) {
                show_input_error ();
                return;
            }
            show_snapshot ((owned) next, true);
        }

        /** Interprets explicit numeric tokens, including unassigned scalars and NUL. */
        private void identify_code (Variant? parameter) {
            var code = read_input (MAX_INPUT);
            if (code == null) {
                show_input_error ();
                return;
            }
            var next = Inspector.from_code (code);
            if (next == null) {
                show_input_error ();
                return;
            }
            show_snapshot ((owned) next, true);
        }

        /** Navigates to an exact alternative using code notation, not its preview glyph. */
        private void navigate_related (Variant? parameter) {
            if (parameter == null) {
                return;
            }
            var next = Inspector.from_code (parameter.get_string ());
            if (next != null) {
                show_snapshot ((owned) next, true);
            }
        }

        /** Returns to the previous snapshot without adding a second history entry. */
        private void navigate_back (Variant? parameter) {
            if (history.is_empty ()) {
                return;
            }
            var code = history.pop_tail ();
            var previous = Inspector.from_code (code);
            if (previous != null) {
                show_snapshot ((owned) previous, false);
            }
        }

        /** Publishes one complete snapshot and rebuilds only its bounded metadata rows. */
        private void show_snapshot (owned Inspector.Snapshot next, bool remember) {
            if (remember && snapshot != null) {
                var old_code = snapshot.format (Inspector.Format.CODE);
                if (old_code != null) {
                    if (history.get_length () >= HISTORY_LIMIT) {
                        history.pop_head ();
                    }
                    history.push_tail (old_code);
                }
            }
            snapshot = (owned) next;
            updating = true;
            input_error.visible = false;
            back_button.sensitive = !history.is_empty ();
            var code = snapshot.format (Inspector.Format.CODE) ?? "";
            var name = snapshot.name () ?? (snapshot.count () == 1 ?
                _("No Canonical Name") : _("Character Sequence"));
            var unassigned = false;
            for (size_t index = 0; index < snapshot.count (); index++) {
                if (!Unicode.is_assigned (snapshot.point (index))) {
                    unassigned = true;
                    break;
                }
            }
            if (unassigned && snapshot.count () == 1) {
                name = _(Search.name (snapshot.point (0)) ?? "Unassigned Character");
            }
            identity_name.label = name;
            identity_code.label = code;
            var text = snapshot.text ();
            preview.label = snapshot.display () ?? "";
            copy_character.sensitive = text != null;
            inspect_favorite.sensitive = text != null && library != null;
            inspect_add.sensitive = text != null && composer != null;
            emoji_button.visible = text != null && Emoji.is_candidate (text);
            if (text == null) {
                preview_note.label = _("This sequence cannot be represented as clipboard text. " +
                    "Copy its code points or encodings instead.");
            } else if (unassigned) {
                preview_note.label = _("This sequence contains reserved scalar values or " +
                    "noncharacters. No assigned glyph is implied. Copying preserves the code points.");
            } else if (text != preview.label) {
                preview_note.label = _("Placeholders show nonprinting or combining characters. " +
                    "Copy preserves the original sequence.");
            } else if (preview.get_layout ().get_unknown_glyphs_count () > 0) {
                preview_note.label = _("Some glyphs are unavailable in the current fonts.");
            } else {
                preview_note.label = "";
            }
            preview_note.visible = preview_note.label != "";
            var components = new Gtk.StringList (null);
            var points = new uint32[(int) snapshot.count ()];
            for (size_t index = 0; index < snapshot.count (); index++) {
                var point = snapshot.point (index);
                points[(int) index] = point;
                components.append ("U+%04X — %s".printf (point,
                    Unicode.name (point) ?? _("No Canonical Name")));
            }
            component_filter.model = components;
            component_filter.selected = 0;
            component_filter.visible = snapshot.count () > 1;
            updating = false;
            update_component ();
            var metadata = new StringBuilder ();
            metadata.append_printf ("%s\n%s\n", name, code);
            for (size_t index = 0; index < snapshot.count (); index++) {
                metadata.append_printf ("\nU+%04X\n", snapshot.point (index));
                append_properties (null, metadata, snapshot.point (index));
            }
            populate_encodings (metadata, points);
            populate_normalization (metadata, text);
            populate_related (metadata);
            version_note.label = _("Catalog: Unicode %s · Properties: Unicode %s · " +
                "Aliases and named sequences: Unicode %s. Unavailable values are not inferred.")
                .printf (Unicode.catalog_version (), Unicode.properties_version () ?? _("Unknown"),
                    Unicode.supplement_version ());
            metadata.append_printf ("\n%s\n", version_note.label);
            report = metadata.str;
            details.visible = true;
        }

        /** Replaces only the component rows when the user chooses a different scalar. */
        private void update_component () {
            if (updating || snapshot == null || component_filter.selected >= snapshot.count ()) {
                return;
            }
            clear_rows (property_rows);
            append_properties (property_rows, null, snapshot.point (component_filter.selected));
        }

        /** Adds the scalar's available properties, with the same labels used in copied reports. */
        private void append_properties (Gtk.Box? target, StringBuilder? metadata, uint32 point) {
            add_field (target, metadata, _("Code point"), "U+%04X".printf (point));
            add_field (target, metadata, _("Canonical name"), Unicode.name (point));
            if (!Unicode.is_assigned (point)) {
                add_field (target, metadata, _("Scalar status"), _(Search.name (point) ?? "Unassigned Character"), false);
            }
            add_field (target, metadata, _("Aliases"), Unicode.aliases (point));
            add_field (target, metadata, _("Assigned in catalog"),
                Unicode.is_assigned (point) ? _("Yes") : _("No"));
            add_field (target, metadata, _("Unicode block"), Unicode.find_block (point));
            string[] keys = { "Plane", "Script", "Script_Extensions", "General_Category",
                "Age", "Bidi_Class", "Bidi_Mirrored", "Canonical_Combining_Class",
                "Decomposition_Type", "East_Asian_Width", "Emoji", "Emoji_Presentation",
                "Emoji_Modifier" };
            string[] labels = { _("Plane"), _("Script"), _("Script extensions"),
                _("General category"), _("Unicode version introduced"), _("Bidirectional class"),
                _("Bidirectional mirrored"), _("Canonical combining class"),
                _("Decomposition type"), _("East Asian width"), _("Emoji property"),
                _("Emoji presentation"), _("Emoji modifier") };
            for (var index = 0; index < keys.length; index++) {
                add_field (target, metadata, labels[index],
                    Unicode.get_property_value (point, keys[index]));
            }
            var decomposition = Inspector.decomposition (point);
            var decomposition_type = Unicode.get_property_value (point, "Decomposition_Type");
            add_field (target, metadata, _("Direct decomposition mapping"),
                decomposition ?? (decomposition_type == "None" ? _("None") : null),
                decomposition != null);
            if (target != null && decomposition != null) {
                var button = new Gtk.Button.with_label (_("Inspect Decomposition"));
                button.set_action_target_value (new Variant.string (decomposition));
                button.action_name = "inspect.related";
                target.append (button);
            }
            if (point != 0 && Unicode.is_assigned (point)) {
                add_field (target, metadata, _("Linux Unicode keyboard entry"),
                    _("Ctrl+Shift+U, then %X, then Enter").printf (point));
                add_field (target, metadata, _("Keyboard entry note"),
                    _("Available in compatible GTK or input-method text fields. Other " +
                      "applications may not support this shortcut. For a sequence, enter " +
                      "each scalar separately or copy the complete sequence."), false);
            }
            append_han_properties (target, metadata, point);
        }

        /** Expose official Unihan fields and independently attributed fallback data for Han. */
        private void append_han_properties (Gtk.Box? target, StringBuilder? metadata,
                                            uint32 point) {
            if (Unicode.get_property_value (point, "Script") != "Hani") {
                return;
            }
            var version = Unicode.unihan_version ();
            add_field (target, metadata, _("Official Unihan version"),
                version == "none" ? _("Not bundled") : version, false);
            string[] keys = { "kMandarin", "kCantonese", "kJapaneseOn", "kJapaneseKun",
                "kJapanese", "kKorean", "kHangul", "kDefinition", "kRSUnicode", "kTotalStrokes" };
            string[] labels = { _("Mandarin (Unihan)"), _("Cantonese (Unihan)"),
                _("Japanese on reading (Unihan)"), _("Japanese kun reading (Unihan)"),
                _("Japanese (Unihan)"), _("Korean (Unihan)"), _("Hangul (Unihan)"),
                _("Definition (Unihan)"), _("Radical and residual strokes (Unihan)"),
                _("Total strokes (Unihan)") };
            for (var index = 0; index < keys.length; index++) {
                add_field (target, metadata, labels[index],
                    Unicode.get_property_value (point, keys[index]));
            }
            string[] fallback_keys = { "Mandarin_Fallback", "Hangul_Fallback",
                "Radical_Collation", "Stroke_Collation" };
            string[] fallback_labels = { _("Mandarin (xpinyin, Unicode 14)"),
                _("Hangul reading (XeTeXko 4.4)"), _("Radical group (ICU 76 collation)"),
                _("Stroke group (ICU 76 collation)") };
            for (var index = 0; index < fallback_keys.length; index++) {
                var value = Unicode.get_property_value (point, fallback_keys[index]);
                if (value != null) {
                    add_field (target, metadata, fallback_labels[index], value);
                }
            }
        }

        /** Shows exact encodings and the emoji label independently of the canonical name. */
        private void populate_encodings (StringBuilder metadata, uint32[] points) {
            clear_rows (encoding_rows);
            metadata.append_printf ("\n%s\n", _("Encodings and Representations"));
            add_field (encoding_rows, metadata, _("Scalar count"), points.length.to_string ());
            var emoji_index = Unicode.emoji_index (points);
            if (emoji_index < Unicode.emoji_count ()) {
                add_field (encoding_rows, metadata, _("Emoji name"),
                    Unicode.emoji_label (emoji_index));
            }
            Inspector.Format[] formats = { Inspector.Format.CODE, Inspector.Format.DECIMAL,
                Inspector.Format.UTF8,
                Inspector.Format.UTF16, Inspector.Format.UTF32, Inspector.Format.HTML_DECIMAL,
                Inspector.Format.HTML_HEX, Inspector.Format.ESCAPE, Inspector.Format.CSS };
            string[] labels = { _("Unicode code points"), _("Decimal"), _("UTF-8 bytes"),
                _("UTF-16 code units"),
                _("UTF-32 code units"), _("HTML decimal references"), _("HTML hex references"),
                _("Unicode escape notation"), _("CSS escape notation") };
            for (var index = 0; index < formats.length; index++) {
                add_field (encoding_rows, metadata, labels[index],
                    snapshot.format (formats[index]));
            }
            add_field (encoding_rows, metadata, _("Representation note"),
                _("UTF-16 and UTF-32 show code units, not byte order. HTML, CSS, and " +
                    "programming languages may reject or reinterpret values, including NUL."),
                false);
        }

        /** Shows normalized code points even for NUL; raw copies are offered only when safe. */
        private void populate_normalization (StringBuilder metadata, string? text) {
            clear_rows (normalization_rows);
            metadata.append_printf ("\n%s\n", _("Normalization"));
            string[] forms = { "NFC", "NFD", "NFKC", "NFKD" };
            foreach (var form in forms) {
                var code = snapshot.normalize (form);
                add_field (normalization_rows, metadata, form, code);
                if (code != null && text != null) {
                    var normalized = Unicode.normalize (text, form);
                    if (normalized != null) {
                        var button = new Gtk.Button.with_label (_("Copy %s Text").printf (form));
                        button.set_action_target_value (new Variant.string (normalized));
                        button.action_name = "inspect.copy";
                        normalization_rows.append (button);
                    }
                }
            }
        }

        /** Builds related-character navigation without changing the browser search or filters. */
        private void populate_related (StringBuilder metadata) {
            clear_rows (related_rows);
            metadata.append_printf ("\n%s\n", _("Related Characters"));
            var alternatives = snapshot.related ();
            if (alternatives == null || alternatives.length == 0) {
                var message = alternatives == null ? _("Unavailable") : _("No related characters");
                add_field (related_rows, metadata, _("Related characters"), message, false);
                return;
            }
            foreach (var text in alternatives) {
                var related = Inspector.from_text (text);
                if (related == null) {
                    continue;
                }
                var code = related.format (Inspector.Format.CODE);
                if (code == null) {
                    continue;
                }
                var name = related.name () ?? _("No Canonical Name");
                metadata.append_printf ("%s — %s\n", code, name);
                var button = new Gtk.Button ();
                var label = new Gtk.Label ("%s   %s — %s".printf (
                    related.display () ?? "", code, name));
                label.wrap = true;
                label.wrap_mode = Pango.WrapMode.WORD_CHAR;
                label.xalign = 0;
                button.child = label;
                button.set_action_target_value (new Variant.string (code));
                button.action_name = "inspect.related";
                related_rows.append (button);
            }
        }

        /** Removes dynamically generated children before changing their action targets. */
        private static void clear_rows (Gtk.Box target) {
            unowned Gtk.Widget? child = target.get_first_child ();
            while (child != null) {
                target.remove (child);
                child = target.get_first_child ();
            }
        }

        /** Adds a plain-text, wrapping metadata row and optional exact-value copy action. */
        private void add_field (Gtk.Box? target, StringBuilder? metadata, string title,
                                string? value, bool can_copy = true) {
            var display = value ?? _("Unavailable");
            if (metadata != null) {
                metadata.append_printf ("%s: %s\n", title, display);
            }
            if (target == null) {
                return;
            }
            var row = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 8);
            var labels = new Gtk.Box (Gtk.Orientation.VERTICAL, 4);
            labels.hexpand = true;
            var caption = new Gtk.Label (title);
            caption.xalign = 0;
            caption.wrap = true;
            caption.add_css_class ("dim-label");
            var label = new Gtk.Label (display);
            label.xalign = 0;
            label.wrap = true;
            label.wrap_mode = Pango.WrapMode.WORD_CHAR;
            label.selectable = true;
            label.max_width_chars = 48;
            labels.append (caption);
            labels.append (label);
            row.append (labels);
            if (value != null && can_copy) {
                var button = new Gtk.Button.from_icon_name ("edit-copy-symbolic");
                var description = _("Copy %s").printf (title);
                button.tooltip_text = description;
                button.update_property (Gtk.AccessibleProperty.LABEL, description);
                button.valign = Gtk.Align.CENTER;
                button.set_action_target_value (new Variant.string (value));
                button.action_name = "inspect.copy";
                row.append (button);
            }
            target.append (row);
        }

        /** Compares installed faces for the selected scalar, not an inferred composite glyph. */
        private void show_font_variations (Variant? parameter) {
            if (snapshot == null || component_filter.selected >= snapshot.count ()) {
                return;
            }
            new FontBrowser (false, snapshot.point (component_filter.selected), composer, library).present (this);
        }

        /** Opens an exact-sequence variation collection independently of the snapshot. */
        private void show_emoji_variations (Variant? parameter) {
            var text = snapshot != null ? snapshot.text () : null;
            if (text != null && Emoji.is_candidate (text)) {
                new EmojiVariations (text, composer, library).present (this);
            }
        }

        /** Copies an action's exact string parameter; never copies a preview placeholder. */
        private void copy_value (Variant? parameter) {
            if (parameter != null) {
                copy_text (parameter.get_string ());
            }
        }

        /** Copies the original sequence when it can be carried by GTK's text clipboard. */
        private void copy_original (Variant? parameter) {
            var text = snapshot != null ? snapshot.text () : null;
            if (text != null) {
                copy_text (text, true);
            }
        }

        /** Appends the exact original sequence, keeping metadata and browser state intact. */
        private void add_to_composer () {
            var text = snapshot != null ? snapshot.text () : null;
            if (text != null && composer != null) {
                composer.append_text (text);
                toast_overlay.add_toast (new Adw.Toast (_("Added to Composer")));
            }
        }

        /** Copies a complete plain-text report, not only the currently visible component. */
        private void copy_information (Variant? parameter) {
            if (snapshot != null) {
                copy_text (report);
            }
        }

        /** Publishes text to the display clipboard after an explicit user action. */
        private void copy_text (string text, bool record = false) {
            Application.copy_widget_text (this, text);
            var saved = !record || library == null || library.record_use (text);
            toast_overlay.add_toast (new Adw.Toast (saved ? _("Copied to Clipboard") :
                _("Copied to Clipboard; Usage History Could Not Be Saved")));
        }

        /** Saves the exact selected identity without recording a copy or insertion. */
        private void save_favorite () {
            var text = snapshot != null ? snapshot.text () : null;
            if (text != null && library != null) {
                var message = library.add_favorite (text) ? _("Saved to Favorites") :
                    _("Unable to Save Favorite. Check the Limit or Settings Permissions.");
                toast_overlay.add_toast (new Adw.Toast (message));
            }
        }
    }
}
