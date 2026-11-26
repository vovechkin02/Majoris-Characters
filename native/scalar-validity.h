#ifndef CHARACTERS_SCALAR_VALIDITY_H
#define CHARACTERS_SCALAR_VALIDITY_H

#include <glib.h>

/** Test scalar validity independently of assignment, font coverage, and ICU version. */
static inline gboolean characters_scalar_is_valid(gunichar point)
{
    return point <= 0x10FFFFu && !(point >= 0xD800u && point <= 0xDFFFu);
}

#endif
