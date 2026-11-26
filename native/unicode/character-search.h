#ifndef CHARACTERS_SEARCH_H
#define CHARACTERS_SEARCH_H

#include <glib.h>

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
CharactersSearch *characters_search_allocate(CharactersSearchScope scope, gint filter,
                                             gint category);

/**
 * characters_search_allocate_points:
 *
 * Copy a bounded explicit scalar repertoire; NULL on invalid data or allocation failure.
 */
CharactersSearch *characters_search_allocate_points(const gunichar *points, gsize length,
                                                    gint category);

/**
 * characters_search_prepare:
 *
 * Copy validated Vala parser output and prepared Han predicates before scanning.
 * No borrowed arrays survive this call. Failed preparation exposes no results.
 * Call at most once successfully before run; only cancel may run concurrently.
 */
gboolean characters_search_prepare(CharactersSearch *search, const gchar *text,
                               const gunichar *points, gsize point_count,
                               gchar *const *terms, gsize term_count,
                               gchar *const *han_terms, gsize han_count,
                               const guint32 *han_rules, gsize han_rule_count,
                               gboolean exact, gboolean literal_letter);

/**
 * characters_search_set_rules:
 *
 * Copy a bounded union of semantic clauses (category mask, decomposition mask,
 * first scalar, last scalar). A zero decomposition mask means any type.
 * Required for symbol scope; false on invalid input or repeated configuration.
 */
gboolean characters_search_set_rules(CharactersSearch *search, const guint32 *rules,
                                 gsize length);

/**
 * characters_search_set_annotation_locales:
 * @search: Unstarted request.
 * @locales: (array length=count): Ordered, resolved annotation table indexes.
 * @count: Number of unique indexes.
 *
 * Returns: Whether the validated locale chain was copied successfully.
 */
gboolean characters_search_set_annotation_locales(CharactersSearch *search,
                                                  const guint32 *locales, gsize count);

/**
 * characters_search_set_annotation_terms:
 * @search: Unstarted request with a prepared query.
 * @terms: (array length=count): Folded UTF-8 operands; empty strings never match.
 * @count: Number of terms, or one for a literal letter.
 *
 * Returns: Whether all operands were copied successfully.
 */
gboolean characters_search_set_annotation_terms(CharactersSearch *search,
                                                gchar *const *terms, gsize count);

/**
 * characters_search_set_scripts:
 *
 * Copy the exact snapshot script mask produced by Vala language selection.
 */
void characters_search_set_scripts(CharactersSearch *search, const guint8 *scripts,
                                   gsize length);

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
gsize characters_search_count(const CharactersSearch *search);

/**
 * characters_search_id:
 * @search: Completed request.
 * @index: Zero-based result position.
 *
 * Returns: Opaque, catalog-local identity or G_MAXUINT32 for invalid position.
 * Identities are stable in this process, not a persistence format.
 */
guint32 characters_search_id(const CharactersSearch *search, gsize index);

/**
 * characters_search_set_metadata:
 *
 * Copy the Vala category masks, English group labels and scalar fallback vocabulary.
 * Reject overlapping masks, invalid lengths and repeated configuration. No borrowed
 * arrays survive this call. Required before run; call only before worker execution.
 */
gboolean characters_search_set_metadata(CharactersSearch *search, const guint32 *masks,
                                          gsize mask_count, gchar *const *labels,
                                          gsize label_count, gchar *const *fallbacks,
                                          gsize fallback_count);

/**
 * characters_search_text:
 * @id: Result identity.
 *
 * Returns: (transfer full) (nullable): Exact UTF-8 text, or NULL for NUL or
 * invalid identities. Never substitutes a display representation.
 */
gchar *characters_search_text(guint32 id);

/**
 * characters_search_sequence_label:
 * @id: Opaque non-scalar catalog identity.
 *
 * Returns: (transfer full) (nullable): Raw catalog sequence label, or NULL for
 * scalar, supplementary alias or invalid identities. Presentation policy lives in Vala.
 */
gchar *characters_search_sequence_label(guint32 id);

#endif
