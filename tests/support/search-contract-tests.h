#ifndef CHARACTERS_SEARCH_CONTRACT_TESTS_H
#define CHARACTERS_SEARCH_CONTRACT_TESTS_H

#include "character-search.h"

/** Vala query factory used by the C scanner and catalog contract fixtures. */
CharactersSearch *characters_query_parser_create(CharactersSearchScope scope, int filter,
                                                 int category, const char *input);

/** Vala query factory for copied repertoires; Vala array length uses gint. */
CharactersSearch *characters_query_parser_create_points(uint32_t *points, int points_length,
                                                        int category, const char *input);

/** Vala language-policy entry point used by the language selection contracts. */
void characters_language_selection_configure(CharactersSearch *job, int filter,
                                             const char *locale, const char *sources);

/** Vala relationship policy returns an owned explicit-length string vector. */
char **characters_relationships_collect(uint32_t *points, int points_length, int *result_length);

/** Run one C contract suite from the Vala test executable's entry point. */
int characters_run_contract_tests(void);

#endif
