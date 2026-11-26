namespace Characters.Emoji {
    /** Identify a standard skin-tone modifier. */
    private static bool is_tone (uint32 point) {
        return point >= 0x1F3FB && point <= 0x1F3FF;
    }

    /** Identify person components in family and relationship sequences. */
    private static bool is_person (uint32 point) {
        return (point >= 0x1F466 && point <= 0x1F469) ||
            point == 0x1F9D1 || point == 0x1F9D2;
    }

    /** Return a structural variation key, independent of translated catalog names. */
    internal static uint32[] family_key (uint32[] points) {
        var bare = new uint32[points.length];
        int count = 0;
        foreach (var point in points) {
            if (point != 0xFE0E && point != 0xFE0F && !is_tone (point)) {
                bare[count++] = point;
            }
        }
        if (count == 0) {
            return new uint32[0];
        }
        var first = bare[0];
        var family = count >= 3 && count % 2 != 0 && is_person (first);
        for (var index = 0; family && index < count; index++) {
            family = index % 2 != 0 ? bare[index] == 0x200D : is_person (bare[index]);
        }
        if (first == 0x1F46A || family) {
            return new uint32[] { uint32.MAX };
        }
        if (first == 0x1F91D || (count == 3 && first == 0x1FAF1 &&
            bare[1] == 0x200D && bare[2] == 0x1FAF2)) {
            return new uint32[] { 0x1F91D };
        }
        if (first == 0x1F491 || (count == 5 && is_person (first) &&
            bare[1] == 0x200D && bare[2] == 0x2764 && bare[3] == 0x200D &&
            is_person (bare[4]))) {
            return new uint32[] { 0x1F491 };
        }
        if (first == 0x1F48F || (count == 7 && is_person (first) &&
            bare[1] == 0x200D && bare[2] == 0x2764 && bare[3] == 0x200D &&
            bare[4] == 0x1F48B && bare[5] == 0x200D && is_person (bare[6]))) {
            return new uint32[] { 0x1F48F };
        }
        if ((first >= 0x1F46B && first <= 0x1F46D) ||
            (count == 5 && is_person (first) && bare[1] == 0x200D &&
            bare[2] == 0x1F91D && bare[3] == 0x200D && is_person (bare[4]))) {
            return new uint32[] { uint32.MAX - 2 };
        }
        if (count == 2 && bare[1] == 0x20E3) {
            return new uint32[] { 0x20E3 };
        }
        if ((count == 2 && first >= 0x1F1E6 && first <= 0x1F1FF &&
            bare[1] >= 0x1F1E6 && bare[1] <= 0x1F1FF) ||
            (count > 2 && first == 0x1F3F4 && bare[count - 1] == 0xE007F)) {
            return new uint32[] { uint32.MAX - 1 };
        }
        var key = new uint32[points.length];
        int length = 0;
        for (var index = 0; index < points.length; index++) {
            var point = points[index];
            if (point == 0xFE0F || point == 0xFE0E || is_tone (point)) {
                continue;
            }
            if (point == 0x200D && index + 1 < points.length) {
                var next = points[index + 1];
                if (next == 0x2640 || next == 0x2642 || next == 0x27A1 ||
                    (next >= 0x1F9B0 && next <= 0x1F9B3)) {
                    index++;
                    continue;
                }
            }
            if (point == 0x1F468 || point == 0x1F469) {
                point = 0x1F9D1;
            } else if (point == 0x1F466 || point == 0x1F467) {
                point = 0x1F9D2;
            } else if (point == 0x1F474 || point == 0x1F475) {
                point = 0x1F9D3;
            }
            key[length++] = point;
        }
        key.resize (length);
        return key;
    }

    /** Compare keys by contents and length, not array identity. */
    internal static bool same_family (uint32[] left, uint32[] right) {
        if (left.length != right.length) {
            return false;
        }
        for (var index = 0; index < left.length; index++) {
            if (left[index] != right[index]) {
                return false;
            }
        }
        return true;
    }
}
