namespace Characters {
    /** Formats stable Unicode property identifiers for human-readable presentation only. */
    internal sealed class PropertyLabels : Object {
        /** Localizes a snapshot script name, retaining a readable English fallback. */
        public static string script_label (string name) {
            return _(name).replace ("_", " ");
        }

        /** Describes an enumerated property without modifying the native or clipboard value. */
        public static string? describe (string property, string? value) {
            if (value == null) {
                return null;
            }
            if (property == "Script" || property == "Script_Extensions") {
                string[] descriptions = {};
                foreach (var code in value.split (" ")) {
                    unowned string? name = Unicode.script_name (code);
                    descriptions += name != null ? with_code (script_label (name), code) : code;
                }
                return string.joinv (", ", descriptions);
            }
            if (property == "Plane") {
                var name = Search.filter_name (Search.Scope.PLANE,
                    (size_t) uint.parse (value));
                return name != null ? _(name) : value;
            }
            string? description = null;
            switch (property) {
                case "General_Category":
                    description = category (value);
                    break;
                case "Bidi_Class":
                    description = bidi (value);
                    break;
                case "Decomposition_Type":
                    description = decomposition (value);
                    break;
                case "East_Asian_Width":
                    description = width (value);
                    break;
                case "Bidi_Mirrored":
                case "Emoji":
                case "Emoji_Presentation":
                case "Emoji_Modifier":
                    description = flag (value);
                    break;
                default:
                    return value;
            }
            return description != null ? with_code (description, value) : value;
        }

        /** Keeps the normative abbreviated identifier visible beside its translated description. */
        private static string with_code (string description, string code) {
            return "%s (%s)".printf (description, code);
        }

        /** Resolves known general category identifiers; unknown values remain raw. */
        private static string? category (string value) {
            switch (value) {
                case "Cn":
                    return _("Unassigned");
                case "Lu":
                    return _("Uppercase letter");
                case "Ll":
                    return _("Lowercase letter");
                case "Lt":
                    return _("Titlecase letter");
                case "Lm":
                    return _("Modifier letter");
                case "Lo":
                    return _("Other letter");
                case "Mn":
                    return _("Nonspacing mark");
                case "Me":
                    return _("Enclosing mark");
                case "Mc":
                    return _("Spacing combining mark");
                case "Nd":
                    return _("Decimal digit");
                case "Nl":
                    return _("Letter number");
                case "No":
                    return _("Other number");
                case "Zs":
                    return _("Space separator");
                case "Zl":
                    return _("Line separator");
                case "Zp":
                    return _("Paragraph separator");
                case "Cc":
                    return _("Control character");
                case "Cf":
                    return _("Format character");
                case "Co":
                    return _("Private-use character");
                case "Cs":
                    return _("Surrogate");
                case "Pd":
                    return _("Dash punctuation");
                case "Ps":
                    return _("Opening punctuation");
                case "Pe":
                    return _("Closing punctuation");
                case "Pc":
                    return _("Connector punctuation");
                case "Po":
                    return _("Other punctuation");
                case "Sm":
                    return _("Mathematical symbol");
                case "Sc":
                    return _("Currency symbol");
                case "Sk":
                    return _("Modifier symbol");
                case "So":
                    return _("Other symbol");
                case "Pi":
                    return _("Initial quote punctuation");
                case "Pf":
                    return _("Final quote punctuation");
                default:
                    return null;
            }
        }

        /** Resolves known bidi class identifiers; unknown values remain raw. */
        private static string? bidi (string value) {
            switch (value) {
                case "L":
                    return _("Left-to-right");
                case "R":
                    return _("Right-to-left");
                case "AL":
                    return _("Arabic letter");
                case "EN":
                    return _("European number");
                case "ES":
                    return _("European number separator");
                case "ET":
                    return _("European number terminator");
                case "AN":
                    return _("Arabic number");
                case "CS":
                    return _("Common number separator");
                case "NSM":
                    return _("Nonspacing mark");
                case "BN":
                    return _("Boundary neutral");
                case "B":
                    return _("Paragraph separator");
                case "S":
                    return _("Segment separator");
                case "WS":
                    return _("Whitespace");
                case "ON":
                    return _("Other neutral");
                case "LRE":
                    return _("Left-to-right embedding");
                case "LRO":
                    return _("Left-to-right override");
                case "RLE":
                    return _("Right-to-left embedding");
                case "RLO":
                    return _("Right-to-left override");
                case "PDF":
                    return _("Pop directional formatting");
                case "LRI":
                    return _("Left-to-right isolate");
                case "RLI":
                    return _("Right-to-left isolate");
                case "FSI":
                    return _("First strong isolate");
                case "PDI":
                    return _("Pop directional isolate");
                default:
                    return null;
            }
        }

        /** Resolves known decomposition type identifiers; unknown values remain raw. */
        private static string? decomposition (string value) {
            switch (value) {
                case "None":
                    return _("None");
                case "Can":
                    return _("Canonical");
                case "Com":
                    return _("Compatibility");
                case "Enc":
                    return _("Encircled");
                case "Fin":
                    return _("Final form");
                case "Font":
                    return _("Font variant");
                case "Fra":
                    return _("Fraction");
                case "Init":
                    return _("Initial form");
                case "Iso":
                    return _("Isolated form");
                case "Med":
                    return _("Medial form");
                case "Nar":
                    return _("Narrow");
                case "Nb":
                    return _("Nonbreaking");
                case "Sml":
                    return _("Small variant");
                case "Sqr":
                    return _("Squared");
                case "Sub":
                    return _("Subscript");
                case "Sup":
                    return _("Superscript");
                case "Vert":
                    return _("Vertical");
                case "Wide":
                    return _("Wide");
                default:
                    return null;
            }
        }

        /** Resolves known east asian width identifiers; unknown values remain raw. */
        private static string? width (string value) {
            switch (value) {
                case "N":
                    return _("Neutral");
                case "A":
                    return _("Ambiguous width");
                case "H":
                    return _("Halfwidth");
                case "W":
                    return _("Wide");
                case "F":
                    return _("Fullwidth");
                case "Na":
                    return _("Narrow");
                default:
                    return null;
            }
        }

        /** Resolves known flag identifiers; unknown values remain raw. */
        private static string? flag (string value) {
            switch (value) {
                case "Y":
                    return _("Yes");
                case "N":
                    return _("No");
                default:
                    return null;
            }
        }
    }
}
