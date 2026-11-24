namespace Characters {
    /** Owns the session-only text buffer and explicit editing and conversion actions. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/composer-view.ui")]
    internal sealed class ComposerView : Gtk.Box {
        /** Expands without changing the browser's active result or selection. */
        [GtkChild]
        private unowned Gtk.Expander composer_expander;
        /** Native multiline editor with bidirectional text, selection, undo and drag-and-drop. */
        [GtkChild]
        private unowned Gtk.TextView composer_input;
        /** Whole-buffer scalar, byte and extended grapheme counts. */
        [GtkChild]
        private unowned Gtk.Label composer_counts;
        /** Describes action results and bounded technical-operation failures. */
        [GtkChild]
        private unowned Gtk.Label composer_status;
        /** Chooses one existing inspector representation. */
        [GtkChild]
        private unowned Gtk.DropDown composer_format;
        /** Chooses the explicit normalization form. */
        [GtkChild]
        private unowned Gtk.DropDown composer_normalization;
        /** Undo is sensitive only when the native buffer has an undo step. */
        [GtkChild]
        private unowned Gtk.Button composer_undo;
        /** Redo is sensitive only when the native buffer has a redo step. */
        [GtkChild]
        private unowned Gtk.Button composer_redo;
        /** Native cut command requires a nonempty selection. */
        [GtkChild]
        private unowned Gtk.Button composer_cut;
        /** Native copy command requires a nonempty selection. */
        [GtkChild]
        private unowned Gtk.Button composer_copy;
        /** Native paste command accepts text from the system clipboard. */
        [GtkChild]
        private unowned Gtk.Button composer_paste;
        /** Explicitly copies the entire buffer regardless of selection. */
        [GtkChild]
        private unowned Gtk.Button composer_copy_all;
        /** Clears all text as an undoable edit. */
        [GtkChild]
        private unowned Gtk.Button composer_clear;
        /** Copies the selected text or entire buffer in the chosen notation. */
        [GtkChild]
        private unowned Gtk.Button composer_copy_format;
        /** Applies explicit normalization to the selected text or entire buffer. */
        [GtkChild]
        private unowned Gtk.Button composer_normalize;
        /** Inspects a short selection or complete short composition. */
        [GtkChild]
        private unowned Gtk.Button composer_inspect;

        /** Configures the native buffer and connects only widget-lifetime signals. */
        construct {
            var buffer = composer_input.buffer;
            buffer.enable_undo = true;
            buffer.set_max_undo_levels (100);
            composer_format.model = new Gtk.StringList ({ _("Unicode Code Points"), _("Decimal"),
                _("UTF-8 Bytes"), _("UTF-16 Code Units"), _("UTF-32 Code Units"),
                _("HTML Decimal"), _("HTML Hexadecimal"), _("Unicode Escapes"), _("CSS Escapes") });
            composer_format.selected = 0;
            composer_normalization.model = new Gtk.StringList ({ "NFC", "NFD", "NFKC", "NFKD" });
            composer_normalization.selected = 0;
            buffer.changed.connect (update_counts);
            buffer.notify["has-selection"].connect (update_actions);
            buffer.notify["can-undo"].connect (update_actions);
            buffer.notify["can-redo"].connect (update_actions);
            composer_undo.clicked.connect (undo);
            composer_redo.clicked.connect (redo);
            composer_cut.clicked.connect (cut);
            composer_copy.clicked.connect (copy_selection);
            composer_paste.clicked.connect (paste);
            composer_copy_all.clicked.connect (copy_all);
            composer_clear.clicked.connect (clear);
            composer_copy_format.clicked.connect (copy_format);
            composer_normalize.clicked.connect (normalize);
            composer_inspect.clicked.connect (inspect_text);
            update_counts ();
        }

        /** Expands the panel; focus is requested only by the explicit Open Composer command. */
        public void open (bool focus = true) {
            composer_expander.expanded = true;
            if (focus) {
                composer_input.grab_focus ();
            }
        }

        /** Appends exact text at the end, leaving the selected browser identity unchanged. */
        public void append_text (string text) {
            if (text == "") {
                return;
            }
            open (false);
            var buffer = composer_input.buffer;
            Gtk.TextIter end;
            buffer.get_end_iter (out end);
            buffer.begin_user_action ();
            buffer.insert (ref end, text, -1);
            buffer.place_cursor (end);
            buffer.end_user_action ();
            composer_status.label = _("Added to Composer");
        }

        /** Selects the current range, or the whole buffer when there is no selection. */
        private void get_target (out Gtk.TextIter start, out Gtk.TextIter end) {
            var buffer = composer_input.buffer;
            if (!buffer.get_selection_bounds (out start, out end)) {
                buffer.get_bounds (out start, out end);
            }
        }

        /** Reads a bounded target before allocating a copy for a technical operation. */
        private string? read_target (int limit) {
            Gtk.TextIter start;
            Gtk.TextIter end;
            get_target (out start, out end);
            if (end.get_offset () - start.get_offset () > limit) {
                composer_status.label = _("Select at most %d code points for this operation.").printf (
                    limit);
                return null;
            }
            return composer_input.buffer.get_text (start, end, true);
        }

        /** Updates counts after edits, skipping expensive analysis of oversized pasted text. */
        private void update_counts () {
            var buffer = composer_input.buffer;
            var count = buffer.get_char_count ();
            composer_status.label = "";
            if (count > Composition.MAX_POINTS) {
                composer_counts.label = _("%d code points · Detailed counts limited to %d").printf (
                    count, Composition.MAX_POINTS);
            } else {
                Gtk.TextIter start;
                Gtk.TextIter end;
                buffer.get_bounds (out start, out end);
                var text = buffer.get_text (start, end, true);
                var clusters = Composition.graphemes (text);
                composer_counts.label = clusters >= 0 ?
                    _("%d code points · %d UTF-8 bytes · %d graphemes (ICU)").printf (
                        count, text.length, clusters) :
                    _("%d code points · %d UTF-8 bytes").printf (count, text.length);
            }
            update_actions ();
        }

        /** Mirrors native undo/redo and selection state, without duplicating edit history. */
        private void update_actions () {
            var buffer = composer_input.buffer;
            var has_text = buffer.get_char_count () > 0;
            composer_undo.sensitive = buffer.can_undo;
            composer_redo.sensitive = buffer.can_redo;
            composer_cut.sensitive = buffer.has_selection;
            composer_copy.sensitive = buffer.has_selection;
            composer_copy_all.sensitive = has_text;
            composer_clear.sensitive = has_text;
            composer_copy_format.sensitive = has_text;
            composer_normalize.sensitive = has_text;
            composer_inspect.sensitive = has_text;
        }

        /** Restores the previous native edit, including grouped normalization and append. */
        private void undo () {
            composer_input.buffer.undo ();
        }

        /** Reapplies the next native edit if available. */
        private void redo () {
            composer_input.buffer.redo ();
        }

        /** Moves selected text to the native clipboard using GTK's editing contract. */
        private void cut () {
            composer_input.buffer.cut_clipboard (get_clipboard (), true);
        }

        /** Copies just the selected text through GTK. */
        private void copy_selection () {
            composer_input.buffer.copy_clipboard (get_clipboard ());
            composer_status.label = _("Copied Selection to Clipboard");
        }

        /** Requests native asynchronous clipboard paste; GTK owns the pending operation. */
        private void paste () {
            composer_input.buffer.paste_clipboard (get_clipboard (), null, true);
        }

        /** Copies the entire exact UTF-8 buffer, independent of selection or normalization. */
        private void copy_all () {
            var buffer = composer_input.buffer;
            Gtk.TextIter start;
            Gtk.TextIter end;
            buffer.get_bounds (out start, out end);
            get_clipboard ().set_text (buffer.get_text (start, end, true));
            composer_status.label = _("Copied Text to Clipboard");
        }

        /** Removes all text in a single undoable edit. */
        private void clear () {
            var buffer = composer_input.buffer;
            Gtk.TextIter start;
            Gtk.TextIter end;
            buffer.get_bounds (out start, out end);
            buffer.begin_user_action ();
            buffer.delete (ref start, ref end);
            buffer.end_user_action ();
        }

        /** Copies an exact numeric representation without modifying the composition. */
        private void copy_format () {
            var text = read_target (Composition.MAX_POINTS);
            if (text == null) {
                return;
            }
            var value = Composition.format (text, (Inspector.Format) composer_format.selected);
            if (value == null) {
                composer_status.label = _("The selected representation is unavailable.");
                return;
            }
            get_clipboard ().set_text (value);
            composer_status.label = _("Copied Representation to Clipboard");
        }

        /** Normalizes only on explicit request, replacing the exact range in one undo step. */
        private void normalize () {
            var text = read_target (Composition.MAX_POINTS);
            var form = composer_normalization.selected_item as Gtk.StringObject;
            if (text == null || form == null) {
                return;
            }
            var value = Composition.normalize (text, form.get_string ());
            if (value == null) {
                composer_status.label = _("Normalization unavailable for this text or output size.");
                return;
            }
            if (value == text) {
                composer_status.label = _("Text is already normalized.");
                return;
            }
            var buffer = composer_input.buffer;
            Gtk.TextIter start;
            Gtk.TextIter end;
            get_target (out start, out end);
            var offset = start.get_offset ();
            var selected = buffer.has_selection;
            buffer.begin_user_action ();
            buffer.delete (ref start, ref end);
            buffer.insert (ref start, value, -1);
            if (selected) {
                Gtk.TextIter first;
                buffer.get_iter_at_offset (out first, offset);
                buffer.select_range (first, start);
            } else {
                buffer.place_cursor (start);
            }
            buffer.end_user_action ();
            composer_status.label = _("Normalized Text (%s)").printf (form.get_string ());
        }

        /** Opens the existing bounded inspector without changing the text or selection. */
        private void inspect_text () {
            var text = read_target (32);
            if (text == null) {
                return;
            }
            var snapshot = Inspector.from_text (text);
            if (snapshot != null) {
                new CharacterInspector ((owned) snapshot, this).present (this);
            }
        }
    }
}
