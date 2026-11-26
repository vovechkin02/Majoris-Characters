namespace Characters.Legacy {
    /** Maximum byte count for an explicit conversion. */
    private const int MAX_BYTES = 128;
    /** Maximum decoded or encoded scalar count. */
    private const int MAX_POINTS = 32;

    /** Accepts only ASCII hexadecimal digits, independent of locale. */
    private static int hex_digit (char digit) {
        if (digit >= '0' && digit <= '9') {
            return digit - '0';
        }
        if (digit >= 'A' && digit <= 'F') {
            return digit - 'A' + 10;
        }
        if (digit >= 'a' && digit <= 'f') {
            return digit - 'a' + 10;
        }
        return -1;
    }

    /** Parses adjacent byte pairs or pairs separated by one ASCII space. */
    public static uint8[]? parse_hex (string? hex) {
        if (hex == null || hex.length == 0 || hex.length >= MAX_BYTES * 3) {
            return null;
        }
        var length = hex.length;
        var bytes = new uint8[MAX_BYTES];
        var offset = 0;
        var used = 0;
        while (offset < length) {
            if (used == MAX_BYTES || offset + 1 >= length) {
                return null;
            }
            var high = hex_digit (hex[offset]);
            var low = hex_digit (hex[offset + 1]);
            if (high < 0 || low < 0) {
                return null;
            }
            bytes[used++] = (uint8) (high * 16 + low);
            offset += 2;
            if (offset < length && hex[offset] == ' ') {
                offset++;
                if (offset == length) {
                    return null;
                }
            }
        }
        bytes.resize (used);
        return bytes;
    }

    /** Append exact byte notation to an existing output without intermediate strings. */
    private static void append_byte (StringBuilder output, uint8 value, bool separator) {
        if (separator) {
            output.append_c (' ');
        }
        output.append_printf ("%02X", (uint) value);
    }

    /** Formats exact bytes, including zero, as uppercase space-separated pairs. */
    public static string? format_bytes (uint8[] bytes) {
        if (bytes.length == 0 || bytes.length > MAX_BYTES) {
            return null;
        }
        var output = new StringBuilder.sized ((size_t) bytes.length * 3);
        for (var index = 0; index < bytes.length; index++) {
            append_byte (output, bytes[index], index != 0);
        }
        return output.str;
    }

    /** Decodes an entire byte stream without altering whitespace or scalar identity. */
    public static uint32[]? decode_sequence (size_t encoding, string? hex) {
        var input = parse_hex (hex);
        return input != null ? decode_raw (encoding, input) : null;
    }

    /** Publishes canonical bytes only if strict reverse conversion preserves every scalar. */
    public static string? encode_sequence (size_t encoding, uint32[] points) {
        if (points.length == 0 || points.length > MAX_POINTS) {
            return null;
        }
        foreach (var point in points) {
            if (!Unicode.is_scalar (point)) {
                return null;
            }
        }
        var encoded = encode_raw (encoding, points);
        if (encoded == null) {
            return null;
        }
        var decoded = decode_raw (encoding, encoded);
        if (decoded == null || decoded.length != points.length) {
            return null;
        }
        for (var index = 0; index < points.length; index++) {
            if (points[index] != decoded[index]) {
                return null;
            }
        }
        return format_bytes (encoded);
    }

    /** Restricts explicit decoding to one scalar present in the selected repertoire. */
    public static uint32 decode (Table? table, string? hex) {
        if (table == null) {
            return uint32.MAX;
        }
        var points = decode_sequence (table.encoding_index (), hex);
        if (points == null || points.length != 1 || !table.contains (points[0])) {
            return uint32.MAX;
        }
        return points[0];
    }

    /** Formats all independent alternatives in native length/value order or canonical bytes. */
    public static string? bytes (Table? table, uint32 point) {
        if (table == null || !Unicode.is_scalar (point)) {
            return null;
        }
        var encoding = table.encoding_index ();
        if (is_contextual (encoding)) {
            if (!table.contains (point)) {
                return null;
            }
            return encode_sequence (encoding, new uint32[] { point });
        }
        var keys = table.keys (point);
        if (keys == null || keys.length == 0) {
            return null;
        }
        var output = new StringBuilder.sized ((size_t) keys.length * 14 + 1);
        foreach (var key in keys) {
            var length = (int) (key >> 32);
            if (length < 1 || length > 4) {
                return null;
            }
            if (output.len > 0) {
                output.append (", ");
            }
            for (var index = 0; index < length; index++) {
                var value = (uint8) (key >> (8 * (length - index - 1)));
                append_byte (output, value, index != 0);
            }
        }
        return output.str;
    }
}
