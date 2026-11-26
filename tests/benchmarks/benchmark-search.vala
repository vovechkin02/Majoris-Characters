using Characters;

/** Number of measured requests following one warm-up request. */
private const int SAMPLE_COUNT = 7;

/** Compute an ordered FNV-1a-style identity signature outside the timed interval. */
private uint64 identity_signature (Search.Job job) {
    uint64 hash = 0xcbf29ce484222325UL;
    for (size_t index = 0; index < job.count (); index++) {
        hash ^= job.id (index);
        hash *= 0x100000001b3UL;
    }
    return hash;
}

/** Sort the small fixed sample vector and return its middle observation. */
private int64 median (int64[] samples) {
    for (var index = 1; index < samples.length; index++) {
        var value = samples[index];
        var position = index;
        while (position > 0 && samples[position - 1] > value) {
            samples[position] = samples[position - 1];
            position--;
        }
        samples[position] = value;
    }
    return samples[samples.length / 2];
}

/** Measure production query preparation and execution, excluding result signature work. */
private bool measure (string label, Search.Scope scope, int filter, int category,
                      string query, string locale = "en") {
    var samples = new int64[SAMPLE_COUNT];
    size_t expected_count = 0;
    uint64 expected_signature = 0;
    for (var sample = -1; sample < SAMPLE_COUNT; sample++) {
        var start = get_monotonic_time ();
        var job = QueryParser.create_localized (scope, filter, category, query, locale);
        if (job == null || job.run () != Search.Status.OK) {
            stderr.printf ("Search failed: %s\n", label);
            return false;
        }
        var elapsed = get_monotonic_time () - start;
        var signature = identity_signature (job);
        if (sample < 0) {
            expected_count = job.count ();
            expected_signature = signature;
        } else {
            if (job.count () != expected_count || signature != expected_signature) {
                stderr.printf ("Unstable search results: %s\n", label);
                return false;
            }
            samples[sample] = elapsed;
        }
    }
    stdout.printf ("%s\t%s\t%s\t%.3f\n", label, expected_count.to_string (),
                   expected_signature.to_string (), median (samples) / 1000.0);
    return true;
}

/** Run representative headless production searches without imposing timing thresholds. */
int main (string[] args) {
    stdout.printf ("Workload\tCount\tOrdered identity signature\tMedian ms\n");
    bool success = measure ("All", Search.Scope.ALL, -1, -1, "") &&
        measure ("Emoji", Search.Scope.EMOJI, -1, -1, "") &&
        measure ("Named", Search.Scope.NAMED, -1, -1, "") &&
        measure ("French emoji keywords", Search.Scope.EMOJI, -1, -1, "émoticône", "fr") &&
        measure ("Exact scalar", Search.Scope.ALL, -1, -1, "U+20AC") &&
        measure ("Exact emoji alias", Search.Scope.EMOJI, -1, -1, "U+2764 U+FE0E") &&
        measure ("Global metadata", Search.Scope.ALL, -1, -1, "euro") &&
        measure ("Han reading", Search.Scope.ALL, -1, -1, "mandarin:zhong") &&
        measure ("Symbols", Search.Scope.SYMBOLS, -1, -1, "") &&
        measure ("Language", Search.Scope.LANGUAGE, 0, -1, "", "ja") &&
        measure ("Private-use plane", Search.Scope.PLANE, 15, -1, "") &&
        measure ("Unassigned", Search.Scope.UNASSIGNED, -1, -1, "");
    return success ? 0 : 1;
}
