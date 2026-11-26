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
}
