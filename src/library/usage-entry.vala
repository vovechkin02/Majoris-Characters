namespace Characters {
    /** Immutable usage snapshot with an exact text identity and saturating count. */
    internal sealed class UsageEntry : Object {
        /** Original, unnormalized character or sequence. */
        public string text { get; private set; }
        /** Count of explicitly recorded uses while the identity remains retained. */
        public uint count { get; private set; }

        /** Creates one independent snapshot for collection display and ranking. */
        public UsageEntry (string text, uint count) {
            this.text = text;
            this.count = count;
        }
    }
}
