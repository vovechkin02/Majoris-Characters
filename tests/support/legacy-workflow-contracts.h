#ifndef CHARACTERS_LEGACY_WORKFLOW_CONTRACTS_H
#define CHARACTERS_LEGACY_WORKFLOW_CONTRACTS_H

#include "legacy-encoding.h"

/** Vala explicit hexadecimal parsing and decode policy; array length is gint. */
uint32_t *characters_legacy_decode_sequence(size_t encoding, const char *hex, int *length);

/** Vala strict round-trip encoding and canonical hexadecimal formatting. */
char *characters_legacy_encode_sequence(size_t encoding, uint32_t *points, int length);

/** Vala scalar repertoire membership policy for a decoded byte stream. */
uint32_t characters_legacy_decode(CharactersLegacy *table, const char *hex);

/** Vala formatting of independent or contextual repertoire mappings. */
char *characters_legacy_bytes(CharactersLegacy *table, uint32_t point);

#endif
