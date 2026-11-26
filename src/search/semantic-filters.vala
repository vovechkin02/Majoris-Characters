namespace Characters.SemanticFilters {
    /** ICU UCharCategory bit masks, also used by the Unicode 17 scalar snapshot. */
    private const uint32 LETTERS = 0x3Eu;
    /** Nonspacing, enclosing and spacing combining marks. */
    private const uint32 MARKS = 0x1C0u;
    /** Decimal, letter and other numbers. */
    private const uint32 NUMBERS = 0xE00u;
    /** All seven punctuation categories, including initial and final quotes. */
    private const uint32 PUNCTUATION = 0x30F80000u;
    /** All four symbol categories. */
    private const uint32 SYMBOLS = 0x0F000000u;
    /** Separators, controls and formatting characters. */
    private const uint32 SPECIAL = 0x1F000u;
    /** All ICU category values, including unassigned and private use. */
    private const uint32 ANY = 0x3FFFFFFFu;
    /** ICU UDecompositionType bit positions for fraction, subscript and superscript. */
    private const uint32 FRACTION = 1u << 6;
    /** Compatibility subscript mapping. */
    private const uint32 SUBSCRIPT = 1u << 14;
    /** Compatibility superscript mapping. */
    private const uint32 SUPERSCRIPT = 1u << 15;
    /** Stable number of indexed semantic collections. */
    private const int COLLECTION_COUNT = 27;
    /** Upper bound shared with the native scanner, four words per clause. */
    private const int RULE_CAPACITY = 128 * 4;

    /** Return curated inclusive range pairs in stable collection-slot order. */
    private static uint32[] ranges (int filter) {
        switch (filter) {
            case 1: // Arrows
                return new uint32[] {
                    0x2190u, 0x21FFu, 0x27F0u, 0x27FFu, 0x2900u, 0x297Fu,
                    0x2B00u, 0x2B11u, 0x1F800u, 0x1F8FFu,
                };
            case 2: // Bullets
                return new uint32[] {
                    0x2022u, 0x2022u, 0x2023u, 0x2023u, 0x2043u, 0x2043u,
                    0x2219u, 0x2219u, 0x25E6u, 0x25E6u, 0x2605u, 0x2606u,
                    0x2720u, 0x274Bu,
                };
            case 3: // Pictures
                return new uint32[] {
                    0x25A0u, 0x27BFu, 0x1F300u, 0x1F5FFu, 0x1F780u, 0x1F7FFu,
                };
            case 5: // Math
                return new uint32[] {
                    0x2200u, 0x22FFu, 0x27C0u, 0x27EFu, 0x2980u, 0x2AFFu,
                    0x1D400u, 0x1D7FFu,
                };
            case 6: // Letters
                return new uint32[] {
                    0x0000u, 0x024Fu, 0x1E00u, 0x1EFFu, 0x2100u, 0x214Fu,
                };
            case 8: // Technical
                return new uint32[] {
                    0x2300u, 0x23FFu, 0x2400u, 0x245Fu, 0x1FB00u, 0x1FBFFu,
                    0x2699u, 0x2699u, 0x26A1u, 0x26A1u, 0x1F50Cu, 0x1F50Cu,
                    0x1F4BBu, 0x1F4BBu, 0x1F5A5u, 0x1F5A8u,
                };
            case 9: // Music
                return new uint32[] {
                    0x2669u, 0x266Fu, 0x1D000u, 0x1D24Fu,
                };
            case 10: // Games
                return new uint32[] {
                    0x2654u, 0x2667u, 0x2680u, 0x2685u, 0x1F000u, 0x1F0FFu,
                    0x1FA00u, 0x1FA6Fu,
                };
            case 11: // Braille
                return new uint32[] {
                    0x2800u, 0x28FFu,
                };
            case 13: // Box Drawing
                return new uint32[] {
                    0x2500u, 0x259Fu,
                };
            case 14: // Phonetics
                return new uint32[] {
                    0x0250u, 0x02AFu, 0x1D00u, 0x1DBFu, 0x10780u, 0x107BFu,
                };
            case 15: // Enclosed
                return new uint32[] {
                    0x2460u, 0x24FFu, 0x3200u, 0x32FFu, 0x1F100u, 0x1F2FFu,
                };
            case 21: // Quotation
                return new uint32[] {
                    0x0022u, 0x0022u, 0x0027u, 0x0027u, 0x300Cu, 0x300Fu,
                    0x301Du, 0x301Fu,
                };
            case 22: // Accessibility
                return new uint32[] {
                    0x267Fu, 0x267Fu, 0x1F9AFu, 0x1F9AFu, 0x1F9BBu, 0x1F9BBu,
                    0x1F9BCu, 0x1F9BCu, 0x1F9BDu, 0x1F9BDu, 0x1F9BEu, 0x1F9BEu,
                    0x1F9BFu, 0x1F9BFu, 0x1F9CFu, 0x1F9CFu, 0x2800u, 0x28FFu,
                };
            case 23: // Letterlike
                return new uint32[] {
                    0x2100u, 0x214Fu,
                };
            case 24: // Religious
                return new uint32[] {
                    0x2626u, 0x262Au, 0x262Cu, 0x262Fu, 0x2638u, 0x2638u,
                    0x271Du, 0x2721u, 0x1F549u, 0x1F54Eu, 0x1F6D0u, 0x1F6D0u,
                };
            case 25: // Variation
                return new uint32[] {
                    0x180Bu, 0x180Du, 0x180Fu, 0x180Fu, 0xFE00u, 0xFE0Fu,
                    0xE0100u, 0xE01EFu,
                };
            case 26: // Block Elements
                return new uint32[] {
                    0x2580u, 0x259Fu, 0x1FB00u, 0x1FB3Bu, 0x1FB70u, 0x1FB8Bu,
                };
            default:
                return new uint32[0];
        }
    }

    /** Append one conjunction to the bounded, request-local union. */
    private static void append (uint32[] rules, ref int used, uint32 categories,
                                uint32 decomposition = 0, uint32 first = 0,
                                uint32 last = 0x10FFFFu) {
        assert (used + 4 <= rules.length);
        rules[used++] = categories;
        rules[used++] = decomposition;
        rules[used++] = first;
        rules[used++] = last;
    }

    /** Append all curated range clauses, restricted to the requested categories. */
    private static void append_ranges (uint32[] rules, ref int used, int filter,
                                       uint32 categories) {
        var bounds = ranges (filter);
        for (var index = 0; index < bounds.length; index += 2) {
            append (rules, ref used, categories, 0, bounds[index], bounds[index + 1]);
        }
    }

    /** Compile one collection's semantic policy, then copy it into the native request. */
    internal static bool configure (Search.Job job, Search.Scope scope, int filter) {
        if (scope != Search.Scope.SYMBOLS) {
            return true;
        }
        if (filter < -1 || filter >= COLLECTION_COUNT) {
            return false;
        }
        var rules = new uint32[RULE_CAPACITY];
        int used = 0;
        if (filter == -1) {
            append (rules, ref used, MARKS | NUMBERS | PUNCTUATION | SYMBOLS | SPECIAL);
            append (rules, ref used, LETTERS, FRACTION | SUBSCRIPT | SUPERSCRIPT);
            for (var index = 0; index < COLLECTION_COUNT; index++) {
                append_ranges (rules, ref used, index, LETTERS);
            }
        } else {
            switch (filter) {
                case 0: // Punctuation
                    append (rules, ref used, PUNCTUATION);
                    break;
                case 4: // Currencies
                    append (rules, ref used, 1u << 25);
                    break;
                case 7: // Numbers
                    append (rules, ref used, NUMBERS);
                    break;
                case 12: // Marks
                    append (rules, ref used, MARKS);
                    break;
                case 16: // Special and formatting
                    append (rules, ref used, SPECIAL);
                    break;
                case 17: // Superscripts
                    append (rules, ref used, ANY, SUPERSCRIPT);
                    break;
                case 18: // Subscripts
                    append (rules, ref used, ANY, SUBSCRIPT);
                    break;
                case 19: // Fractions
                    append (rules, ref used, ANY, FRACTION);
                    append (rules, ref used, ANY, 0, 0x2044u, 0x2044u);
                    break;
                case 20: // Unicode White_Space (stable in Unicode 17)
                    uint32[] whitespace = {
                        0x0009u, 0x000Du, 0x0020u, 0x0020u, 0x0085u, 0x0085u,
                        0x00A0u, 0x00A0u, 0x1680u, 0x1680u, 0x2000u, 0x200Au,
                        0x2028u, 0x2029u, 0x202Fu, 0x202Fu, 0x205Fu, 0x205Fu,
                        0x3000u, 0x3000u,
                    };
                    for (var index = 0; index < whitespace.length; index += 2) {
                        append (rules, ref used, ANY, 0, whitespace[index], whitespace[index + 1]);
                    }
                    break;
                case 21: // Quotation marks
                    append (rules, ref used, (1u << 28) | (1u << 29));
                    break;
                case 5: // Mathematics
                    append (rules, ref used, 1u << 24);
                    break;
                default:
                    break;
            }
            append_ranges (rules, ref used, filter, filter == 6 ? LETTERS : ANY);
        }
        return job.set_rules (rules[0:used]);
    }
}
