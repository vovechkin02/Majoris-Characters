#ifndef CHARACTERS_SEARCH_H
#define CHARACTERS_SEARCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** The mutually exclusive primary browsing scopes. */
typedef enum {
    /** All assigned scalars and known sequences. */
    CHARACTERS_SEARCH_ALL,
    /** Fully qualified emoji in semantic group order. */
    CHARACTERS_SEARCH_EMOJI,
    /** Semantic symbol and typography collections. */
    CHARACTERS_SEARCH_SYMBOLS,
    /** Primary Script and explicit Script_Extensions membership. */
    CHARACTERS_SEARCH_SCRIPT,
    /** Catalog Unicode blocks. */
    CHARACTERS_SEARCH_BLOCK,
    /** Standardized named sequences. */
    CHARACTERS_SEARCH_NAMED,
    /** Language-associated writing systems, including current locale and XKB languages. */
    CHARACTERS_SEARCH_LANGUAGE,
    /** Han radical groups, using official data or labeled collation fallback. */
    CHARACTERS_SEARCH_RADICAL,
    /** Han stroke-count groups, using official data or labeled collation fallback. */
    CHARACTERS_SEARCH_STROKES,
    /** Assigned scalars grouped by Unicode plane. */
    CHARACTERS_SEARCH_PLANE,
    /** Reserved and noncharacter scalar values grouped by plane. */
    CHARACTERS_SEARCH_UNASSIGNED
} CharactersSearchScope;

/** Completion status; canceled or failed jobs never expose partial results. */
typedef enum {
    /** Completed with an immutable result vector. */
    CHARACTERS_SEARCH_OK,
    /** Canceled without published results. */
    CHARACTERS_SEARCH_CANCELED,
    /** Invalid query, filter or repeated execution. */
    CHARACTERS_SEARCH_INVALID,
    /** Allocation or catalog failure. */
    CHARACTERS_SEARCH_FAILED
} CharactersSearchStatus;

/** An owned, single-run request with thread-safe cancellation. */
typedef struct CharactersSearch CharactersSearch;

/**
 * characters_search_allocate:
 *
 * Allocate an unprepared single-run request; invalid filters fail at run time.
 */
CharactersSearch *characters_search_allocate(CharactersSearchScope scope, int filter,
                                             int category);

/**
 * characters_search_allocate_points:
 *
 * Copy a bounded explicit scalar repertoire; NULL on invalid data or allocation failure.
 */
CharactersSearch *characters_search_allocate_points(const uint32_t *points, size_t length,
                                                    int category);

/**
 * characters_search_prepare:
 *
 * Copy validated Vala parser output and fold Han terms once before worker scanning.
 * No borrowed arrays survive this call. Failed preparation exposes no results.
 * Call at most once successfully before run; only cancel may run concurrently.
 */
bool characters_search_prepare(CharactersSearch *search, const char *text,
                               const uint32_t *points, size_t point_count,
                               char *const *terms, size_t term_count,
                               bool exact, bool literal_letter);

/**
 * characters_search_set_locale:
 * @search: Request not yet submitted to a worker.
 * @locale: Locale identifier copied before execution; English is always a fallback.
 *
 * Selects emoji annotations. Do not call concurrently with run().
 */
void characters_search_set_locale(CharactersSearch *search, const char *locale);

/**
 * characters_search_set_scripts:
 *
 * Copy the exact snapshot script mask produced by Vala language selection.
 */
void characters_search_set_scripts(CharactersSearch *search, const uint8_t *scripts,
                                   size_t length);

/**
 * characters_search_free:
 * @search: (transfer full): Request no longer used by any thread.
 *
 * Releases all request storage. NULL is accepted.
 */
void characters_search_free(CharactersSearch *search);

/**
 * characters_search_cancel:
 * @search: Live request.
 *
 * Thread-safe, idempotent cancellation. Does not wait for the worker.
 */
void characters_search_cancel(CharactersSearch *search);

/**
 * characters_search_run:
 * @search: Request owned by the caller, run exactly once on a worker.
 *
 * Returns: Completion status. Only cancel() may be called concurrently.
 * Read results after the worker has published completion.
 */
CharactersSearchStatus characters_search_run(CharactersSearch *search);

/**
 * characters_search_count:
 * @search: Completed request.
 *
 * Returns: Number of results, zero unless successfully completed.
 */
size_t characters_search_count(const CharactersSearch *search);

/**
 * characters_search_id:
 * @search: Completed request.
 * @index: Zero-based result position.
 *
 * Returns: Opaque, catalog-local identity or UINT32_MAX for invalid position.
 * Identities are stable in this process, not a persistence format.
 */
uint32_t characters_search_id(const CharactersSearch *search, size_t index);

/**
 * characters_search_filter_count:
 * @scope: Primary collection.
 *
 * Returns: Number of possible filter slots, some of which may have no name.
 */
size_t characters_search_filter_count(CharactersSearchScope scope);

/**
 * characters_search_filter_name:
 * @scope: Primary collection.
 * @index: Filter slot.
 *
 * Returns: (transfer none) (nullable): Immutable English label or NULL for an
 * unavailable slot. Script slots refer to the bundled Unicode snapshot.
 */
const char *characters_search_filter_name(CharactersSearchScope scope, size_t index);

/**
 * characters_search_category_count:
 *
 * Returns: Number of available general-category groups.
 */
size_t characters_search_category_count(void);

/**
 * characters_search_category_name:
 * @index: Category group slot.
 *
 * Returns: (transfer none) (nullable): Immutable English label or NULL.
 */
const char *characters_search_category_name(size_t index);

/**
 * characters_search_text:
 * @id: Result identity.
 *
 * Returns: (transfer full) (nullable): Exact UTF-8 text, or NULL for NUL or
 * invalid identities. Never substitutes a display representation.
 */
char *characters_search_text(uint32_t id);

/**
 * characters_search_display:
 * @id: Result identity.
 *
 * Returns: (transfer full) (nullable): Safe tile representation, including
 * visible placeholders for invisible scalars and dotted circle for marks.
 */
char *characters_search_display(uint32_t id);

/**
 * characters_search_name:
 * @id: Result identity.
 *
 * Returns: (transfer full) (nullable): Catalog name, alias, or descriptive
 * fallback for an assigned scalar; NULL for invalid identity.
 */
char *characters_search_name(uint32_t id);

/**
 * characters_search_code:
 * @id: Result identity.
 *
 * Returns: (transfer full) (nullable): All scalar values in U+ notation.
 */
char *characters_search_code(uint32_t id);

#endif
