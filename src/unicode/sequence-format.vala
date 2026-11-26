namespace Characters.Inspector {
    /** Exact scalar representations shared by inspection and composition. */
    public enum Format {
        /** Unicode code point notation. */
        CODE,
        /** Decimal scalar values. */
        DECIMAL,
        /** UTF-8 bytes in hexadecimal. */
        UTF8,
        /** UTF-16 code units in hexadecimal. */
        UTF16,
        /** UTF-32 code units in hexadecimal. */
        UTF32,
        /** Decimal numeric character references. */
        HTML_DECIMAL,
        /** Hexadecimal numeric character references. */
        HTML_HEX,
        /** Fixed-width Unicode escapes (C and Python style). */
        ESCAPE,
        /** CSS hexadecimal escapes with terminating spaces. */
        CSS,
    }

    /** Format a nonempty scalar array without losing embedded NUL or selectors. */
    public static string? format_points (uint32[] points, Format format) {
        if (points.length == 0 || points.length > int.MAX / 16) {
            return null;
        }
        var output = new StringBuilder.sized ((size_t) points.length * 16 + 1);
        for (var index = 0; index < points.length; index++) {
            var point = points[index];
            if (!Unicode.is_scalar (point)) {
                return null;
            }
            var separator = index == 0 ? "" : " ";
            switch (format) {
                case Format.CODE:
                    output.append_printf ("%sU+%04X", separator, point);
                    break;
                case Format.DECIMAL:
                    output.append_printf ("%s%u", separator, point);
                    break;
                case Format.UTF8:
                    append_utf8 (output, point, separator);
                    break;
                case Format.UTF16:
                    if (point <= 0xFFFF) {
                        output.append_printf ("%s%04X", separator, point);
                    } else {
                        var offset = point - 0x10000;
                        output.append_printf ("%s%04X %04X", separator,
                            0xD800 + (offset >> 10), 0xDC00 + (offset & 0x3FF));
                    }
                    break;
                case Format.UTF32:
                    output.append_printf ("%s%08X", separator, point);
                    break;
                case Format.HTML_DECIMAL:
                    output.append_printf ("&#%u;", point);
                    break;
                case Format.HTML_HEX:
                    output.append_printf ("&#x%X;", point);
                    break;
                case Format.ESCAPE:
                    output.append_printf (point <= 0xFFFF ? "\\u%04X" : "\\U%08X", point);
                    break;
                case Format.CSS:
                    output.append_printf ("\\%X ", point);
                    break;
                default:
                    return null;
            }
        }
        return output.str;
    }

    /** Append the exact UTF-8 octets; scalar zero is represented as 00. */
    private static void append_utf8 (StringBuilder output, uint32 point, string separator) {
        if (point <= 0x7F) {
            output.append_printf ("%s%02X", separator, point);
        } else if (point <= 0x7FF) {
            output.append_printf ("%s%02X %02X", separator,
                0xC0 | (point >> 6), 0x80 | (point & 0x3F));
        } else if (point <= 0xFFFF) {
            output.append_printf ("%s%02X %02X %02X", separator,
                0xE0 | (point >> 12), 0x80 | ((point >> 6) & 0x3F),
                0x80 | (point & 0x3F));
        } else {
            output.append_printf ("%s%02X %02X %02X %02X", separator,
                0xF0 | (point >> 18), 0x80 | ((point >> 12) & 0x3F),
                0x80 | ((point >> 6) & 0x3F), 0x80 | (point & 0x3F));
        }
    }
}
