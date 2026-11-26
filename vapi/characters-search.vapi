/** Internal browsing API, with single-run native requests and opaque result identities. */
[CCode (cheader_filename = "character-search.h")]
namespace Characters.Search {
    /** Primary browsing collection; matches the sidebar order. */
    [CCode (cname = "CharactersSearchScope", cprefix = "CHARACTERS_SEARCH_", has_type_id = false)]
    public enum Scope {
        /** All assigned scalars and catalog sequences. */
        ALL,
        /** Fully qualified emoji in semantic group order. */
        EMOJI,
        /** Semantic symbol and typography collections. */
        SYMBOLS,
        /** Unicode Script and Script_Extensions browsing. */
        SCRIPT,
        /** Unicode block browsing. */
        BLOCK,
        /** Standardized named sequences. */
        NAMED,
        /** Language-associated writing systems. */
        LANGUAGE,
        /** Han radical groups. */
        RADICAL,
        /** Han stroke-count groups. */
        STROKES,
        /** Assigned Unicode scalars by plane. */
        PLANE,
        /** Reserved scalars and noncharacters by plane. */
        UNASSIGNED
    }

    /** Native worker completion status; only OK exposes results. */
    [CCode (cname = "CharactersSearchStatus", cprefix = "CHARACTERS_SEARCH_", has_type_id = false)]
    public enum Status {
        /** Search completed successfully. */
        OK,
        /** Search was canceled; results are unavailable. */
        CANCELED,
        /** Invalid query or filter. */
        INVALID,
        /** Worker or allocation failure. */
        FAILED
    }

    /** Owned native request; only cancel may run concurrently with run. */
    [Compact]
    [CCode (cname = "CharactersSearch", free_function = "characters_search_free",
        has_type_id = false)]
    public class Job {
        /** Copy ordered resolved annotation locale indexes. */
        [CCode (cname = "characters_search_set_annotation_locales")]
        public bool set_annotation_locales ([CCode (array_length_type = "gsize")] uint32[] locales);

        /** Copy folded annotation operands parallel to metadata terms or one literal letter. */
        [CCode (cname = "characters_search_set_annotation_terms")]
        public bool set_annotation_terms ([CCode (array_length_type = "gsize")] string[] terms);

        /** Copy a query-local script membership mask before the worker starts scanning. */
        [CCode (cname = "characters_search_set_scripts")]
        public void set_scripts ([CCode (array_length_type = "gsize")] uint8[] scripts);

        /** Copy parser output and prepared Han operands; no borrowed data survives. */
        [CCode (cname = "characters_search_prepare")]
        public bool prepare (string text,
            [CCode (array_length_type = "gsize")] uint32[] points,
            [CCode (array_length_type = "gsize")] string[] terms,
            [CCode (array_length_type = "gsize")] string[] han_terms,
            [CCode (array_length_type = "gsize")] uint32[] han_rules,
            bool exact, bool literal_letter);

        /** Copy a bounded union of four-word semantic clauses before scanning. */
        [CCode (cname = "characters_search_set_rules")]
        public bool set_rules ([CCode (array_length_type = "gsize")] uint32[] rules);

        /** Copy category masks and English search vocabulary before worker execution. */
        [CCode (cname = "characters_search_set_metadata")]
        public bool set_metadata ([CCode (array_length_type = "gsize")] uint32[] masks,
            [CCode (array_length_type = "gsize")] string[] labels,
            [CCode (array_length_type = "gsize")] string[] fallbacks);

        /** Request cancellation without waiting for its worker. */
        [CCode (cname = "characters_search_cancel")]
        public void cancel ();

        /** Execute exactly once on a worker; results become immutable on completion. */
        [CCode (cname = "characters_search_run")]
        public Status run ();

        /** Number of successfully completed results. */
        [CCode (cname = "characters_search_count")]
        public size_t count ();

        /** Opaque identity at a valid result position; UINT32_MAX on invalid positions. */
        [CCode (cname = "characters_search_id")]
        public uint32 id (size_t index);
    }

    /** Allocate an unprepared request, to be populated by the Vala query parser. */
    [CCode (cname = "characters_search_allocate")]
    public static Job? allocate (Scope scope, int filter, int category);

    /** Copy an explicit scalar repertoire; an empty repertoire remains empty. */
    [CCode (cname = "characters_search_allocate_points", array_length_type = "gsize")]
    public static Job? allocate_points (uint32[]? points, int category);

    /** Exact UTF-8 sequence, or null for NUL or invalid identities. */
    [CCode (cname = "characters_search_text")]
    public static string? text (uint32 id);

    /** Owned raw catalog label; null for scalar, supplementary alias or invalid identities. */
    [CCode (cname = "characters_search_sequence_label")]
    public static string? catalog_sequence_label (uint32 id);
}
