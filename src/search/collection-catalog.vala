namespace Characters.Search {
    /** Return the stable English symbol name for a valid slot. */
    private static string? symbol_name (size_t index) {
        switch (index) {
            case 0:
                return N_("Punctuation");
            case 1:
                return N_("Arrows");
            case 2:
                return N_("Bullets & Stars");
            case 3:
                return N_("Pictures & Shapes");
            case 4:
                return N_("Currencies");
            case 5:
                return N_("Mathematics");
            case 6:
                return N_("Letters");
            case 7:
                return N_("Numbers & Fractions");
            case 8:
                return N_("Technical");
            case 9:
                return N_("Music");
            case 10:
                return N_("Games");
            case 11:
                return N_("Braille");
            case 12:
                return N_("Combining Marks");
            case 13:
                return N_("Box Drawing");
            case 14:
                return N_("Phonetics");
            case 15:
                return N_("Enclosed");
            case 16:
                return N_("Special & Formatting");
            case 17:
                return N_("Superscripts");
            case 18:
                return N_("Subscripts");
            case 19:
                return N_("Fractions");
            case 20:
                return N_("Whitespace");
            case 21:
                return N_("Quotation Marks");
            case 22:
                return N_("Accessibility Symbols");
            case 23:
                return N_("Letterlike Symbols");
            case 24:
                return N_("Religious Symbols");
            case 25:
                return N_("Variation Selectors");
            case 26:
                return N_("Block Elements");
            default:
                return null;
        }
    }

    /** Return the stable English plane name for a valid slot. */
    private static string? plane_name (size_t index) {
        switch (index) {
            case 0:
                return N_("0: Basic Multilingual Plane");
            case 1:
                return N_("1: Supplementary Multilingual Plane");
            case 2:
                return N_("2: Supplementary Ideographic Plane");
            case 3:
                return N_("3: Tertiary Ideographic Plane");
            case 4:
                return N_("4: Plane 4 (Reserved)");
            case 5:
                return N_("5: Plane 5 (Reserved)");
            case 6:
                return N_("6: Plane 6 (Reserved)");
            case 7:
                return N_("7: Plane 7 (Reserved)");
            case 8:
                return N_("8: Plane 8 (Reserved)");
            case 9:
                return N_("9: Plane 9 (Reserved)");
            case 10:
                return N_("10: Plane 10 (Reserved)");
            case 11:
                return N_("11: Plane 11 (Reserved)");
            case 12:
                return N_("12: Plane 12 (Reserved)");
            case 13:
                return N_("13: Plane 13 (Reserved)");
            case 14:
                return N_("14: Supplementary Special-purpose Plane");
            case 15:
                return N_("15: Supplementary Private Use Area-A");
            case 16:
                return N_("16: Supplementary Private Use Area-B");
            default:
                return null;
        }
    }

    /** Return the stable English category name for a valid slot. */
    private static string? category_label (size_t index) {
        switch (index) {
            case 0:
                return N_("Letters");
            case 1:
                return N_("Marks");
            case 2:
                return N_("Numbers");
            case 3:
                return N_("Punctuation");
            case 4:
                return N_("Symbols");
            case 5:
                return N_("Separators");
            case 6:
                return N_("Controls & Formatting");
            case 7:
                return N_("Private Use");
            default:
                return null;
        }
    }

    /** Return the stable English fallback name for a valid slot. */
    private static string? fallback_label (size_t index) {
        switch (index) {
            case 0:
                return N_("Noncharacter");
            case 1:
                return N_("Unassigned Character");
            case 2:
                return N_("Private Use Character");
            case 3:
                return N_("Unnamed Character");
            default:
                return null;
        }
    }

    /** Number of general-category groups, independent of translated labels. */
    public static size_t category_count () {
        return 8;
    }

    /** Return the untranslated category label for a valid group slot. */
    public static string? category_name (size_t index) {
        return index < 8 ? category_label (index) : null;
    }

    /** Copy category masks and English metadata vocabulary into a request before scanning. */
    internal static bool configure_metadata (Job job) {
        // ICU UCharCategory bits in category-label order, not GLib UnicodeType order.
        uint32[] masks = {
            0x3Eu, 0x1C0u, 0xE00u, 0x30F80000u,
            0x0F000000u, 0x7000u, 0x18000u, 0x20000u,
        };
        var labels = new string[(int) category_count ()];
        var fallbacks = new string[4];
        for (var index = 0; index < labels.length; index++) {
            labels[index] = category_label ((size_t) index) ?? "";
        }
        for (var index = 0; index < fallbacks.length; index++) {
            fallbacks[index] = fallback_label ((size_t) index) ?? "";
        }
        return job.set_metadata (masks, labels, fallbacks);
    }

    /** Select the fallback vocabulary for a valid scalar without a canonical name or alias. */
    internal static string fallback_name (uint32 point) {
        if (!Unicode.is_assigned (point)) {
            var noncharacter = (point >= 0xFDD0u && point <= 0xFDEFu) ||
                (point & 0xFFFFu) >= 0xFFFEu;
            return fallback_label (noncharacter ? 0 : 1) ?? "";
        }
        var type = Scalar.category (point);
        return fallback_label (type == Scalar.Category.PRIVATE_USE_CHAR ? 2 : 3) ?? "";
    }

    /** Number of collection-specific slots, including any unavailable registry entries. */
    public static size_t filter_count (Scope scope) {
        switch (scope) {
            case Scope.EMOJI:
                return Unicode.emoji_group_count ();
            case Scope.SYMBOLS:
                return SemanticFilters.COLLECTION_COUNT;
            case Scope.SCRIPT:
                return Scalar.script_count ();
            case Scope.BLOCK:
                return Unicode.block_count ();
            case Scope.LANGUAGE:
                return LanguageData.count ();
            case Scope.RADICAL:
                return Han.group_count (true);
            case Scope.STROKES:
                return Han.group_count (false);
            case Scope.PLANE:
            case Scope.UNASSIGNED:
                return 17;
            default:
                return 0;
        }
    }

    /** Return an untranslated label or null for an invalid or unavailable slot. */
    public static string? filter_name (Scope scope, size_t index) {
        if (index >= filter_count (scope)) {
            return null;
        }
        switch (scope) {
            case Scope.EMOJI:
                return Unicode.emoji_group_name (index);
            case Scope.SYMBOLS:
                return symbol_name (index);
            case Scope.SCRIPT:
                return Scalar.script_name (index);
            case Scope.BLOCK:
                return Unicode.block_name (index);
            case Scope.LANGUAGE:
                return LanguageData.name (index);
            case Scope.RADICAL:
                return Han.group_name (true, index);
            case Scope.STROKES:
                return Han.group_name (false, index);
            case Scope.PLANE:
            case Scope.UNASSIGNED:
                return plane_name (index);
            default:
                return null;
        }
    }
}
