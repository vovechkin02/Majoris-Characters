namespace Characters {
    /** Distinguishes the concrete data source operation, not presentation modes. */
    internal enum FontSourceKind {
        /** Discover or refresh the installed face inventory. */
        INVENTORY,
        /** Extract mapped scalars for one exact installed face. */
        FACE,
        /** Construct the strict byte-to-scalar table for a legacy encoding. */
        ENCODING
    }

    /** Owns one source operation; only the worker writes results before publication. */
    internal sealed class FontSourceRequest : Object {
        /** Operation requested by the main context. */
        public FontSourceKind kind { get; private set; }
        /** Retained input or newly loaded inventory, published after run returns. */
        public FontInventory? inventory { get; private set; }
        /** Immutable face or encoding index, meaningful for those operation kinds. */
        public size_t index { get; private set; }

        /** Explicit refresh bypasses Fontconfig's timestamp check. */
        private bool force_refresh;
        /** Owned scalar repertoire, transferred to the main-context consumer on completion. */
        private uint32[]? points;
        /** Exact face opened on the worker; transferred to the main context after publication. */
        private Fonts.Face? exact_face;
        /** Owned legacy mapping, transferred once alongside its repertoire. */
        private Legacy.Table? table;

        /** Checks a retained inventory or constructs a fresh one off the GTK main context. */
        public FontSourceRequest.for_inventory (FontInventory? current, bool force_refresh) {
            kind = FontSourceKind.INVENTORY;
            inventory = current;
            this.force_refresh = force_refresh;
        }

        /** Retains exact immutable face identity across worker execution. */
        public FontSourceRequest.for_face (FontInventory inventory, size_t index) {
            kind = FontSourceKind.FACE;
            this.inventory = inventory;
            this.index = index;
        }

        /** Captures the selected encoding before any worker starts. */
        public FontSourceRequest.for_encoding (size_t index) {
            kind = FontSourceKind.ENCODING;
            this.index = index;
        }

        /** Executes one source operation on a worker; no GTK state is accessed. */
        public bool run () {
            switch (kind) {
                case FontSourceKind.INVENTORY:
                    if (!force_refresh && inventory != null &&
                        inventory.get_catalog ().is_current ()) {
                        return true;
                    }
                    var catalog = Fonts.load ();
                    if (catalog == null) {
                        return false;
                    }
                    inventory = new FontInventory ((owned) catalog);
                    return true;
                case FontSourceKind.FACE:
                    if (inventory == null) {
                        return false;
                    }
                    exact_face = inventory.get_catalog ().open_face (index);
                    if (exact_face == null) {
                        return false;
                    }
                    points = exact_face.unicode_points ();
                    return points != null;
                case FontSourceKind.ENCODING:
                    table = Legacy.load (index);
                    if (table == null) {
                        return false;
                    }
                    points = table.points ();
                    return points != null;
                default:
                    return false;
            }
        }

        /** Transfers the completed repertoire once; must be called on the main context. */
        public uint32[]? take_points () {
            return (owned) points;
        }

        /** Transfers the exact face once; no further worker operations access it. */
        public Fonts.Face? take_face () {
            return (owned) exact_face;
        }

        /** Transfers the completed encoding table once on the main context. */
        public Legacy.Table? take_table () {
            return (owned) table;
        }
    }
}
