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
        /** Capture the annotation locale before handing the request to its worker. */
        [CCode (cname = "characters_search_set_locale")]
        public void set_locale (string locale);

        /** Copy a query-local script membership mask before the worker starts scanning. */
        [CCode (cname = "characters_search_set_scripts")]
        public void set_scripts ([CCode (array_length_type = "size_t")] uint8[] scripts);

        /** Copy parser output, folding Han terms once; no borrowed data survives. */
        [CCode (cname = "characters_search_prepare")]
        public bool prepare (string text,
            [CCode (array_length_type = "size_t")] uint32[] points,
            [CCode (array_length_type = "size_t")] string[] terms,
            bool exact, bool literal_letter);

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
    [CCode (cname = "characters_search_allocate_points", array_length_type = "size_t")]
    public static Job? allocate_points (uint32[]? points, int category);

    /** Number of collection-specific filter slots, including unavailable slots. */
    [CCode (cname = "characters_search_filter_count")]
    public static size_t filter_count (Scope scope);

    /** Borrowed English label, or null for an unavailable slot. */
    [CCode (cname = "characters_search_filter_name")]
    public static unowned string? filter_name (Scope scope, size_t index);

    /** Number of general-category groups. */
    [CCode (cname = "characters_search_category_count")]
    public static size_t category_count ();

    /** Borrowed English category label, or null for invalid slots. */
    [CCode (cname = "characters_search_category_name")]
    public static unowned string? category_name (size_t index);

    /** Exact UTF-8 sequence, or null for NUL or invalid identities. */
    [CCode (cname = "characters_search_text")]
    public static string? text (uint32 id);

    /** Safe visible tile representation, distinct from the original sequence. */
    [CCode (cname = "characters_search_display")]
    public static string? display (uint32 id);

    /** Catalog or fallback name for a valid identity. */
    [CCode (cname = "characters_search_name")]
    public static string? name (uint32 id);

    /** Complete code point notation for an identity, including composite sequences. */
    [CCode (cname = "characters_search_code")]
    public static string? code (uint32 id);
}
