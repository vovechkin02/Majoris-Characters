#ifndef CHARACTERS_SCALAR_VALIDITY_H
#define CHARACTERS_SCALAR_VALIDITY_H

#include <stdbool.h>
#include <stdint.h>

/** Test scalar validity independently of assignment, font coverage, and ICU version. */
static inline bool characters_scalar_is_valid(uint32_t point) {
    return point <= 0x10FFFFu && !(point >= 0xD800u && point <= 0xDFFFu);
}

#endif
