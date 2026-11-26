#ifndef CHARACTERS_SEARCH_CONTRACT_TESTS_H
#define CHARACTERS_SEARCH_CONTRACT_TESTS_H

#include "character-search.h"

/** Vala collection count, indexed by the native scope. */
gsize characters_search_filter_count(CharactersSearchScope scope);

/** Owned English collection label, or NULL outside the registry. */
gchar *characters_search_filter_name(CharactersSearchScope scope, gsize index);

/** Owned safe glyph representation, never used for copy. */
gchar *characters_search_display(guint32 id);

/** Owned catalog or fallback name, or NULL for an invalid identity. */
gchar *characters_search_name(guint32 id);

/** Owned exact code-point notation for an identity. */
gchar *characters_search_code(guint32 id);

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
