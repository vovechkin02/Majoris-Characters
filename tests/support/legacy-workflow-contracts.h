#ifndef CHARACTERS_LEGACY_WORKFLOW_CONTRACTS_H
#define CHARACTERS_LEGACY_WORKFLOW_CONTRACTS_H

#include "legacy-encoding.h"

/** Vala explicit hexadecimal parsing and decode policy; array length is gint. */
guint32 *characters_legacy_decode_sequence(gsize encoding, const gchar *hex, gint *length);

/** Vala strict round-trip encoding and canonical hexadecimal formatting. */
gchar *characters_legacy_encode_sequence(gsize encoding, gunichar *points, gint length);

/** Vala scalar repertoire membership policy for a decoded byte stream. */
guint32 characters_legacy_decode(CharactersLegacy *table, const gchar *hex);

/** Vala formatting of independent or contextual repertoire mappings. */
gchar *characters_legacy_bytes(CharactersLegacy *table, gunichar point);

#endif
