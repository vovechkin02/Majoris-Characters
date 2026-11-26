namespace Characters.Unicode {
    /**
     * Return exact Unicode or Han metadata, keeping formal and fallback values separate.
     * Plane is defined for every valid scalar; other UCD fields require assignment.
     */
    public static string? get_property_value (uint32 point, string? property) {
        if (property == null || !is_scalar (point)) {
            return null;
        }
        if (property == "Plane") {
            return (point >> 16).to_string ();
        }
        var canonical = Scalar.property_alias (property) ?? property;
        if (is_assigned (point)) {
            switch (canonical) {
                case "Age":
                    var age = Scalar.age (point);
                    return age < 0 ? null : "%d.%d".printf (age / 10, age % 10);
                case "Canonical_Combining_Class":
                    return Scalar.combining_class (point).to_string ();
                case "General_Category":
                    return Scalar.text (point, Scalar.Text.CATEGORY);
                case "Script":
                    return Scalar.text (point, Scalar.Text.SCRIPT);
                case "Script_Extensions":
                    return Scalar.text (point, Scalar.Text.EXTENSIONS);
                case "Bidi_Class":
                    return Scalar.text (point, Scalar.Text.BIDI);
                case "Decomposition_Type":
                    return Scalar.text (point, Scalar.Text.DECOMPOSITION);
                case "East_Asian_Width":
                    return Scalar.text (point, Scalar.Text.WIDTH);
                case "Bidi_Mirrored":
                    return boolean_value (point, 0);
                case "Emoji":
                    return boolean_value (point, 1);
                case "Emoji_Presentation":
                    return boolean_value (point, 2);
                case "Emoji_Modifier":
                    return boolean_value (point, 3);
            }
        }
        return han_value (point, property);
    }

    /** Format a standardized Boolean property as the original Y/N value. */
    private static string boolean_value (uint32 point, uint bit) {
        return (Scalar.boolean_flags (point) & (1u << bit)) != 0 ? "Y" : "N";
    }

    /** Select one exact Han source field, never substituting fallback for official metadata. */
    private static string? han_value (uint32 point, string property) {
        switch (property) {
            case "kMandarin":
                return Han.value (point, Han.Value.MANDARIN);
            case "kCantonese":
                return Han.value (point, Han.Value.CANTONESE);
            case "kJapaneseOn":
                return Han.value (point, Han.Value.JAPANESE_ON);
            case "kJapaneseKun":
                return Han.value (point, Han.Value.JAPANESE_KUN);
            case "kJapanese":
                return Han.value (point, Han.Value.JAPANESE);
            case "kKorean":
                return Han.value (point, Han.Value.KOREAN);
            case "kHangul":
                return Han.value (point, Han.Value.HANGUL);
            case "kDefinition":
                return Han.value (point, Han.Value.DEFINITION);
            case "kRSUnicode":
                return Han.value (point, Han.Value.RADICAL);
            case "kTotalStrokes":
                return Han.value (point, Han.Value.STROKES);
            case "Mandarin_Fallback":
                return Han.value (point, Han.Value.MANDARIN_FALLBACK);
            case "Hangul_Fallback":
                return Han.value (point, Han.Value.HANGUL_FALLBACK);
            case "Radical_Collation":
                return collation_value (point, true);
            case "Stroke_Collation":
                return collation_value (point, false);
            default:
                return null;
        }
    }

    /** Format a raw fallback group without making an absent group appear as zero. */
    private static string? collation_value (uint32 point, bool radical) {
        var group = Han.collation (point, radical);
        return group == 0 ? null : group.to_string ();
    }
}
