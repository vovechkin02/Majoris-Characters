#ifndef CHARACTERS_SEARCH_CONTRACT_TESTS_H
#define CHARACTERS_SEARCH_CONTRACT_TESTS_H

#include "character-search.h"

/** Vala query factory used by the C scanner and catalog contract fixtures. */
CharactersSearch *characters_query_parser_create(CharactersSearchScope scope, gint filter,
                                                 gint category, const gchar *input);

/** Vala query factory for copied repertoires; Vala array length uses gint. */
CharactersSearch *characters_query_parser_create_points(gunichar *points, gint points_length,
                                                        gint category, const gchar *input);

/** Vala language-policy entry point used by the language selection contracts. */
void characters_language_selection_configure(CharactersSearch *job, gint filter,
                                             const gchar *locale, const gchar *sources);

/** Vala relationship policy returns an owned explicit-length string vector. */
gchar **characters_relationships_collect(gunichar *points, gint points_length, gint *result_length);

/** Run one C contract suite from the Vala test executable's entry point. */
gint characters_run_contract_tests(void);

#endif
