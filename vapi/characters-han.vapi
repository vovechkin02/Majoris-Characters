/** Native Han folding and prepared predicate identifiers. */
[CCode (cheader_filename = "unihan.h")]
namespace Characters.Han {
    /** Native predicate kinds, used only in the request-local descriptor. */
    [CCode (cname = "CharactersHanKind", cprefix = "CHARACTERS_HAN_", has_type_id = false)]
    public enum Kind {
        /** Ordinary metadata term. */
        ORDINARY,
        /** Explicit reading or definition. */
        FIELD,
        /** Numeric radical group. */
        RADICAL,
        /** Numeric stroke group. */
        STROKES,
        /** Invalid explicit constraint. */
        REJECT
    }

    /** Return owned folded reading text, or null if the bounded ICU operation fails. */
    [CCode (cname = "characters_unihan_fold_text")]
    public static string? fold_text (string term);

    /** Number of radical or stroke groups. */
    [CCode (cname = "characters_unihan_group_count")]
    public static size_t group_count (bool radical);

    /** Borrow the raw group label at an index. */
    [CCode (cname = "characters_unihan_group_name")]
    public static unowned string? group_name (bool radical, size_t index);

    /** Raw official and fallback metadata fields, in generated record order. */
    [CCode (cname = "CharactersHanValue", cprefix = "CHARACTERS_HAN_VALUE_",
        has_type_id = false)]
    public enum Value {
        /** Mandarin reading. */
        MANDARIN,
        /** Cantonese reading. */
        CANTONESE,
        /** Japanese on reading. */
        JAPANESE_ON,
        /** Japanese kun reading. */
        JAPANESE_KUN,
        /** Japanese reading. */
        JAPANESE,
        /** Korean reading. */
        KOREAN,
        /** Hangul reading. */
        HANGUL,
        /** English definition. */
        DEFINITION,
        /** Official radical and residual strokes. */
        RADICAL,
        /** Official total strokes. */
        STROKES,
        /** Separately attributed Mandarin fallback. */
        MANDARIN_FALLBACK,
        /** Separately attributed Hangul fallback. */
        HANGUL_FALLBACK
    }

    /** Borrow an exact field without changing its provenance. */
    [CCode (cname = "characters_unihan_value")]
    public static unowned string? value (uint32 point, Value field);

    /** Return a raw fallback collation group, zero when absent. */
    [CCode (cname = "characters_unihan_collation")]
    public static uint collation (uint32 point, bool radical);
}
