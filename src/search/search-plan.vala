namespace Characters.SearchPlan {
    /** Last numeric code point, including reserved positions. */
    private const uint32 LAST_POINT = 0x10FFFFu;

    /**
     * Prepare a bounded scan after all query and collection operands have been accepted.
     * A failed preparation leaves the native request uncommitted, so run cannot publish
     * partial or silently degraded results. The native worker owns all copied storage.
     */
    internal static bool prepare (Search.Job job, Search.Scope scope, int filter,
                                  string? query, string locale, string input_sources,
                                  bool explicit_points = false) {
        if (!QueryParser.prepare (job, query, locale) ||
            !SemanticFilters.configure (job, scope, filter)) {
            return false;
        }
        if (scope == Search.Scope.LANGUAGE &&
            !LanguageSelection.configure (job, filter, locale, input_sources)) {
            return false;
        }
        if (explicit_points) {
            return job.set_plan ((uint) Search.Phase.POINTS, 0, 0);
        }
        if (filter < -1 || (filter >= 0 && (size_t) filter >= Search.filter_count (scope))) {
            return false;
        }
        uint phases = (uint) Search.Phase.SCALARS;
        uint32 first = 0;
        uint32 last = LAST_POINT;
        switch (scope) {
            case Search.Scope.ALL:
                phases |= (uint) Search.Phase.EMOJI | (uint) Search.Phase.NAMED;
                break;
            case Search.Scope.EMOJI:
                phases = (uint) Search.Phase.EMOJI;
                first = last = 0;
                break;
            case Search.Scope.NAMED:
                phases = (uint) Search.Phase.NAMED;
                first = last = 0;
                break;
            case Search.Scope.BLOCK:
                if (filter >= 0) {
                    first = Unicode.block_start ((size_t) filter);
                    last = Unicode.block_end ((size_t) filter);
                }
                break;
            case Search.Scope.PLANE:
            case Search.Scope.UNASSIGNED:
                if (filter >= 0) {
                    first = (uint32) filter << 16;
                    last = first + 0xFFFFu;
                }
                break;
            default:
                break;
        }
        return job.set_plan (phases, first, last);
    }
}
