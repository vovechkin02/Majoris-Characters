namespace Characters {
    /** Converts independent byte streams without changing the selected scalar repertoire. */
    [GtkTemplate (ui = "/com/majoris/characters/ui/legacy-sequence-dialog.ui")]
    internal sealed class LegacySequenceDialog : Adw.Dialog {
        /** Identifies the exact converter selected in the browser. */
        [GtkChild]
        private unowned Gtk.Label encoding_name;
        /** Hexadecimal input or canonical output, with no implicit normalization. */
        [GtkChild]
        private unowned Gtk.Entry byte_input;
        /** Literal text or explicit Unicode notation, selected by the action. */
        [GtkChild]
        private unowned Gtk.Entry unicode_input;
        /** Reports conversion success, errors and unavailable clipboard text. */
        [GtkChild]
        private unowned Gtk.Label sequence_status;
        /** Decodes the complete byte input from the converter's initial state. */
        [GtkChild]
        private unowned Gtk.Button decode_bytes;
        /** Encodes explicit numeric Unicode notation. */
        [GtkChild]
        private unowned Gtk.Button encode_codes;
        /** Encodes exact literal text without trimming or normalization. */
        [GtkChild]
        private unowned Gtk.Button encode_text;
        /** Copies the original or canonical hexadecimal bytes after successful conversion. */
        [GtkChild]
        private unowned Gtk.Button copy_sequence_bytes;
        /** Opens the shared inspector for every constituent scalar. */
        [GtkChild]
        private unowned Gtk.Button inspect_sequence;
        /** Immutable converter index; no dependence on later dropdown changes. */
        private size_t encoding;
        /** Non-owning shared composition view. */
        private weak ComposerView? composer;
        /** Shared local favorites and history repository for inspector actions. */
        private LibraryStore? library;
        /** Exact code notation of the last successful conversion; empty after edits. */
        private string result_code = "";
        /** Suppresses invalidation while publishing a consistent conversion result. */
        private bool updating = false;

        /** Opens the bounded converter, optionally decoding a previously entered byte sequence. */
        public LegacySequenceDialog (size_t encoding, string initial = "",
                                     ComposerView? composer = null, LibraryStore? library = null) {
            this.encoding = encoding;
            this.composer = composer;
            this.library = library;
            encoding_name.label = Legacy.name (encoding) ?? _("Unavailable");
            byte_input.changed.connect (invalidate_result);
            unicode_input.changed.connect (invalidate_result);
            byte_input.activate.connect (decode);
            decode_bytes.clicked.connect (decode);
            encode_codes.clicked.connect (encode_code_points);
            encode_text.clicked.connect (encode_literal);
            copy_sequence_bytes.clicked.connect (copy_bytes);
            inspect_sequence.clicked.connect (inspect_result);
            if (initial != "") {
                byte_input.text = initial;
                decode ();
            }
        }

        /** Invalidates successful output when either editable field changes. */
        private void invalidate_result () {
            if (updating) {
                return;
            }
            result_code = "";
            copy_sequence_bytes.sensitive = false;
            inspect_sequence.sensitive = false;
            sequence_status.label = _("Choose a Conversion Action");
        }

        /** Strictly decodes bytes and retains their original hexadecimal identity. */
        private void decode () {
            invalidate_result ();
            var hex = byte_input.text.strip ();
            var points = Legacy.decode_sequence (encoding, hex);
            if (points == null) {
                sequence_status.label = _("Invalid, Empty, Truncated or Oversized Byte Sequence");
                return;
            }
            publish_result (points, hex, false);
        }

        /** Parses explicit numeric input, including nonprinting and NUL scalars. */
        private void encode_code_points () {
            encode (Inspector.from_code (unicode_input.text));
        }

        /** Parses literal UTF-8 input without modifying whitespace or combining marks. */
        private void encode_literal () {
            encode (Inspector.from_text (unicode_input.text));
        }

        /** Encodes only when strict reverse conversion reproduces the entire input. */
        private void encode (owned Inspector.Snapshot? snapshot) {
            invalidate_result ();
            if (snapshot == null) {
                sequence_status.label = _("Invalid, Empty or Oversized Unicode Sequence");
                return;
            }
            var points = new uint32[(int) snapshot.count ()];
            for (int index = 0; index < points.length; index++) {
                points[index] = snapshot.point (index);
            }
            var hex = Legacy.encode_sequence (encoding, points);
            if (hex == null) {
                sequence_status.label = _("No Exact Round-Trip Encoding Within the Byte Limit");
                return;
            }
            publish_result (points, hex, true);
        }

        /** Publishes matching byte and scalar fields atomically and enables explicit actions. */
        private void publish_result (uint32[] points, string hex, bool canonical) {
            var code = new StringBuilder ();
            foreach (var point in points) {
                if (code.len > 0) {
                    code.append_c (' ');
                }
                code.append_printf ("U+%04X", point);
            }
            updating = true;
            result_code = code.str;
            unicode_input.text = result_code;
            byte_input.text = hex;
            updating = false;
            copy_sequence_bytes.sensitive = true;
            inspect_sequence.sensitive = true;
            sequence_status.label = canonical ? _("Exact Round Trip; Canonical Encoded Bytes") :
                _("Exact Decoding; Original Input Bytes Preserved");
        }

        /** Copies only hexadecimal notation, never an opaque byte buffer as GTK text. */
        private void copy_bytes () {
            if (result_code != "") {
                Application.copy_widget_text (this, byte_input.text);
                sequence_status.label = _("Copied to Clipboard");
            }
        }

        /** Delegates Unicode copying, metadata, favorites and composition to the shared inspector. */
        private void inspect_result () {
            if (result_code == "") {
                return;
            }
            var snapshot = Inspector.from_code (result_code);
            if (snapshot != null) {
                new CharacterInspector ((owned) snapshot, composer, library).present (this);
            }
        }
    }
}
