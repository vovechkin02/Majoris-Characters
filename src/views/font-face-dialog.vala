namespace Characters {
    /** Inspect raw cmap identities and adjust a private exact face's design coordinates. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/font-face-dialog.ui")]
    internal sealed class FontFaceDialog : Adw.Dialog {
        /** Preview size in points, converted to pixels by FontPreview. */
        private const int PREVIEW_SIZE_PT = 72;

        /** Exact file and face label. */
        [GtkChild]
        private unowned Gtk.Label face_identity;
        /** Native exact-face image; no Pango fallback. */
        [GtkChild]
        private unowned Gtk.Picture face_picture;
        /** Unicode or raw code and actual glyph index. */
        [GtkChild]
        private unowned Gtk.Label glyph_identity;
        /** Explicit preview availability and operation status. */
        [GtkChild]
        private unowned Gtk.Label face_status;
        /** Hidden for faces without adjustable design axes. */
        [GtkChild]
        private unowned Gtk.Box axis_controls;
        /** Selects one design axis while retaining the other coordinates. */
        [GtkChild]
        private unowned Gtk.DropDown axis_selector;
        /** Edits the selected design coordinate. */
        [GtkChild]
        private unowned Gtk.SpinButton axis_value;
        /** Declared minimum, maximum and default for the selected axis. */
        [GtkChild]
        private unowned Gtk.Label axis_range;
        /** Restores the originally selected named instance. */
        [GtkChild]
        private unowned Gtk.Button axis_reset;
        /** Hidden for faces without a Microsoft Symbol cmap. */
        [GtkChild]
        private unowned Gtk.Box symbol_controls;
        /** Switches between the selected Unicode scalar and raw font codes. */
        [GtkChild]
        private unowned Gtk.CheckButton symbol_mode;
        /** Searchable raw cmap entries, not Unicode scalars. */
        [GtkChild]
        private unowned Gtk.DropDown symbol_selector;
        /** Copies only textual hexadecimal and glyph-index metadata. */
        [GtkChild]
        private unowned Gtk.Button copy_symbol_code;

        /** Private mutable face, never shared with the browser grid or worker. */
        private Fonts.Face face;
        /** Scalar selected in the parent browser, or invalid for symbol-only faces. */
        private uint32 point;
        /** Blocks coordinate writes while synchronizing controls. */
        private bool updating = false;
        /** Takes ownership of an independently opened face and initializes its controls. */
        public FontFaceDialog (owned Fonts.Face face, string identity, uint32 point) {
            this.face = (owned) face;
            this.point = point;
            face_identity.label = identity;
            var axes = new Gtk.StringList (null);
            for (size_t index = 0; index < this.face.axis_count (); index++) {
                axes.append (this.face.axis_tag (index) ?? "?");
            }
            axis_selector.model = axes;
            axis_controls.visible = axes.get_n_items () > 0;
            axis_selector.selected = axis_controls.visible ? 0 : Gtk.INVALID_LIST_POSITION;
            var symbols = new Gtk.StringList (null);
            for (size_t index = 0; index < this.face.symbol_count (); index++) {
                symbols.append ("0x%04X".printf (this.face.symbol_code (index)));
            }
            symbol_selector.model = symbols;
            symbol_controls.visible = symbols.get_n_items () > 0;
            symbol_selector.selected = symbol_controls.visible ? 0 : Gtk.INVALID_LIST_POSITION;
            symbol_mode.active = symbol_controls.visible && point == uint32.MAX;
            symbol_mode.sensitive = point != uint32.MAX;
            axis_selector.notify["selected"].connect (sync_axis);
            axis_value.value_changed.connect (change_axis);
            axis_reset.clicked.connect (reset_axes);
            symbol_mode.toggled.connect (update_preview);
            symbol_selector.notify["selected"].connect (update_preview);
            copy_symbol_code.clicked.connect (copy_raw_code);
            map.connect (update_preview);
            sync_axis ();
            update_preview ();
        }

        /** Loads the active coordinate and range without triggering a variation change. */
        private void sync_axis () {
            double minimum, maximum, default_value, current;
            if (!face.axis_range (axis_selector.selected, out minimum, out maximum,
                                  out default_value, out current)) {
                return;
            }
            updating = true;
            var step = (maximum - minimum) / 100.0;
            axis_value.set_range (minimum, maximum);
            axis_value.set_increments (step > 0 ? step : 0.0001, step > 0 ? step * 10 : 0.001);
            axis_value.value = current;
            axis_range.label = _("Minimum: %g · Maximum: %g · Default: %g").printf (
                minimum, maximum, default_value);
            updating = false;
        }

        /** Applies a single coordinate and immediately rerenders the exact selected glyph. */
        private void change_axis () {
            if (updating) {
                return;
            }
            if (!face.set_axis (axis_selector.selected, axis_value.value)) {
                sync_axis ();
                face_status.label = _("Unable to Apply Variation Coordinate");
                return;
            }
            update_preview ();
        }

        /** Resets all coordinates to the originally opened face, including named instances. */
        private void reset_axes () {
            if (!face.reset ()) {
                face_status.label = _("Unable to Reset Variation Coordinates");
                return;
            }
            sync_axis ();
            update_preview ();
        }

        /** Returns the active raw code or Unicode scalar without conflating their namespaces. */
        private uint32 selected_code () {
            return symbol_mode.active ? face.symbol_code (symbol_selector.selected) : point;
        }

        /** Updates preview and metadata without copying or recording a raw symbol identity. */
        private void update_preview () {
            var code = selected_code ();
            var symbol = symbol_mode.active;
            var glyph = face.glyph (code, symbol);
            symbol_selector.sensitive = symbol;
            copy_symbol_code.sensitive = symbol && glyph != 0;
            glyph_identity.label = symbol ? _("Raw code 0x%04X · Glyph %u").printf (code, glyph) :
                _("U+%04X · Glyph %u").printf (code, glyph);
            if (glyph == 0) {
                glyph_identity.label = _("No Mapped Glyph Selected");
                face_picture.paintable = null;
                face_status.label = _("Select a mapped character or raw symbol code.");
                return;
            }
            face_picture.paintable = FontPreview.render (face, code, symbol, PREVIEW_SIZE_PT, this);
            face_status.label = face_picture.paintable != null ?
                _("Exact selected face, without fallback. Adjustments apply to this preview only.") :
                _("Exact glyph rendering unavailable for this format.");
        }

        /** Copies metadata only; does not publish the raw value as a Unicode character. */
        private void copy_raw_code () {
            if (!symbol_mode.active) {
                return;
            }
            var code = selected_code ();
            var glyph = face.glyph (code, true);
            if (glyph != 0) {
                Application.copy_widget_text (this, "0x%04X — %u".printf (code, glyph));
                face_status.label = _("Copied Raw Code and Glyph Index");
            }
        }
    }
}
