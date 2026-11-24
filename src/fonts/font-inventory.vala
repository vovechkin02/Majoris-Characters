namespace Characters {
    /** Shares one immutable native inventory across source workers and main-context consumers. */
    internal sealed class FontInventory : Object {
        /** Owns the native configuration and exact installed-face patterns. */
        private Fonts.Catalog catalog;

        /** Takes ownership of a successfully loaded native snapshot. */
        public FontInventory (owned Fonts.Catalog catalog) {
            this.catalog = (owned) catalog;
        }

        /** Borrows the immutable catalog for the lifetime of this reference-counted inventory. */
        public unowned Fonts.Catalog get_catalog () {
            return catalog;
        }
    }
}
