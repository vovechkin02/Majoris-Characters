namespace Characters {
    /** Owns a sorted, immutable view of one installed Fontconfig snapshot. */
    internal sealed class FontInventory : Object {
        /** Owns native patterns and configuration for all published records. */
        private Fonts.Catalog catalog;
        /** Validated face records in deterministic application order. */
        private FontFaceRecord[] faces;
        /** Family offsets into the sorted faces, with a final sentinel. */
        private uint[] family_starts;
        /** Number of initialized family offsets, excluding the sentinel. */
        public uint family_count { get; private set; }

        /** Takes ownership and builds metadata and family groups on a source worker. */
        public FontInventory (owned Fonts.Catalog catalog) {
            this.catalog = (owned) catalog;
            var ordered = new List<FontFaceRecord> ();
            for (size_t index = 0; index < this.catalog.count (); index++) {
                var record = FontFaceRecord.read (this.catalog, index);
                if (record != null) {
                    ordered.prepend (record);
                }
            }
            ordered.reverse ();
            ordered.sort (FontFaceRecord.compare);
            faces = new FontFaceRecord[(int) ordered.length ()];
            family_starts = new uint[faces.length + 1];
            uint position = 0;
            family_count = 0;
            foreach (unowned FontFaceRecord record in ordered) {
                if (position == 0 || record.family != faces[position - 1].family) {
                    family_starts[family_count++] = position;
                }
                faces[position++] = record;
            }
            family_starts[family_count] = position;
        }

        /** Number of usable faces in application order. */
        public uint count () {
            return (uint) faces.length;
        }

        /** Borrows immutable metadata while this inventory is retained. */
        public unowned FontFaceRecord? record (size_t index) {
            return index < count () ? faces[index] : null;
        }

        /** Returns the first face of a group, or the end sentinel for invalid groups. */
        public uint family_start (uint family) {
            return family <= family_count ? family_starts[family] : count ();
        }

        /** Restores the last matching file and face, otherwise the first available face. */
        public void find_selection (string? file, int face_index, out uint family,
                                    out uint face) {
            family = 0;
            face = 0;
            if (file == null) {
                return;
            }
            for (uint group = 0; group < family_count; group++) {
                for (uint index = family_start (group); index < family_start (group + 1);
                     index++) {
                    if (faces[index].file == file && faces[index].face_index == face_index) {
                        family = group;
                        face = index - family_start (group);
                    }
                }
            }
        }

        /** Returns sorted application identities with exact coverage and a matching label. */
        public uint[] comparisons (uint32 point, string query, string regular) {
            var term = query.strip ().casefold ();
            var result = new uint[faces.length];
            int matched = 0;
            for (uint index = 0; index < count (); index++) {
                if (!catalog.contains (faces[index].native_index, point)) {
                    continue;
                }
                if (term != "" && !faces[index].display_name (regular).casefold ().contains (term)) {
                    continue;
                }
                result[matched++] = index;
            }
            result.resize (matched);
            return result;
        }

        /** Obtains a nominal rendering description for a sorted application identity. */
        public string? description (size_t index) {
            return index < count () ? catalog.description (faces[index].native_index) : null;
        }

        /** Opens an independent exact face, preserving its original native identity. */
        public Fonts.Face? open_face (size_t index) {
            return index < count () ? catalog.open_face (faces[index].native_index) : null;
        }

        /** Checks the retained native configuration for updates on a worker. */
        public bool is_current () {
            return catalog.is_current ();
        }

        /** Applies the retained configuration to a renderer on the GTK main context. */
        public bool apply_config (Pango.FontMap? map) {
            return catalog.apply_config (map);
        }

        /** Checks nominal resolution against the native identity, not the sorted position. */
        public bool matches (size_t index, Pango.Font? font) {
            return index < count () && catalog.matches (faces[index].native_index, font);
        }
    }
}
