# Character browsing and search

## Browsing contract

The All Characters scope enumerates every scalar recognized as assigned by the
Stage 2 engine, including controls and private-use scalars, then known composite
emoji and named sequences. Scalars are ordered by code point. Emoji appear in
upstream semantic group order; named sequences retain supplement order. The
same exact sequence is emitted once in global results if both catalogs match;
its catalog-specific identity remains available in its dedicated scope.
Surrogates, unassigned scalars and noncharacters are excluded from All Characters.

The Emoji scope retains exact fully qualified sequences and their upstream
names, including single-scalar emoji. The Symbols scope provides 27 overlapping
semantic collections. The Scripts scope uses the Unicode 17 Script and Script_Extensions properties;
Unicode Blocks uses the independently versioned GNOME block table. Named
Sequences exposes the standardized supplement. The collection dropdown offers
all nine emoji groups, encoded script slots, 346 Unicode blocks, or symbol
collections as appropriate. A general-category dropdown can further constrain
scalar results; it is hidden for emoji, named sequences and unassigned scalars.
When a general-category filter is selected in All Characters, multi-scalar
sequences are excluded because they do not have one scalar category.

Shared script characters match each applicable script. Language and configured
keyboard collections resolve to script unions. The full assigned repertoire uses the
bundled Unicode 17 property snapshot. Unicode block entries that
contain only unassigned or surrogate points return an empty grid. Symbols use
indexed curated ranges, general categories, and Unicode decomposition and
whitespace properties, not an exhaustive semantic ontology; the global scope covers the complete available catalog regardless of
semantic classification. Underlying English catalog data and labels are not
localized at this stage.

### Planes and unassigned values

Unicode Planes groups assigned scalars by their high sixteen bits, from plane 0
through plane 16, including supplementary private-use planes. It does not include
composite sequence identities, since a sequence is not a single plane member.
Unassigned & Noncharacters uses the same filters to browse valid, unassigned
scalar values; the 66 permanent noncharacters have a distinct display name.
Surrogates are excluded from both modes. Reserved values and noncharacters use a
placeholder, never an inferred glyph. Exact numeric values remain inspectable
and copyable, as with legacy repertoire mappings. Both sidebar entries can be
hidden in Preferences. The global assigned-character catalog is unchanged.

### Expanded semantic collections

Ten new overlapping collections retain existing slot numbers: Superscripts,
Subscripts, Fractions, Whitespace, Quotation Marks, Accessibility Symbols,
Letterlike Symbols, Religious Symbols, Variation Selectors, and Block Elements.
The Technical collection also includes selected computing, engineering and
keyboard symbols. Superscript/subscript/fraction membership uses Unicode 17
Decomposition_Type; whitespace uses the stable Unicode White_Space property.
Quotation and other semantic groups use curated ranges and general categories.
These are convenience collections rather than a normative Unicode classification.

## Query syntax

| Input | Meaning |
| --- | --- |
| `euro` | Case-insensitive English metadata substring |
| `greek small letter alpha` | All terms must occur across name, aliases, block, script or category |
| `line feed` | Alias or other metadata match |
| `U+20AC`, `0x20ac`, `20AC` | Exact hexadecimal scalar |
| `41` | Exact hexadecimal scalar, U+0041 |
| `dec:8364` | Exact decimal scalar, U+20AC |
| `€` | Exact literal scalar |
| `👩‍💻` | Exact literal catalog sequence |
| `U+1F469 U+200D U+1F4BB` | Exact catalog sequence, preserving joiners and modifiers |
| `woman technologist` | Emoji catalog label terms |
| `face` | Both U+FACE and metadata containing the word fragment, because all-letter hex is ambiguous |
| Empty or ASCII whitespace | Browse the selected scope without a query |

Raw hex accepts two to six hexadecimal digits with at least one decimal digit;
four-to-six all-letter hex matches the scalar and metadata. A one-character
query is treated as a literal scalar. Use explicit `U+` or `0x` for unambiguous
numeric queries, including one-digit values, and `dec:` for decimal. Explicit
numeric sequences require a base prefix on each value. Unassigned valid scalars
return no result in ordinary assigned scopes; malformed explicitly numeric input, invalid UTF-8, surrogate
values and out-of-range values report invalid input. Input is bounded to 512
UTF-8 bytes, 64 terms, and 32 scalars for a numeric sequence. Queries are not
normalized: only exact catalog sequences are identified as sequences. Arbitrary
multi-character text inspection and normalization controls belong to Stage 4.

Search is constrained by the current collection and filters. Reset Search and
Filters in the main menu restores global browsing with no query or constraint.
Names, aliases, category and group labels provide keyword matching; localized
CLDR annotations, synonyms derived from CLDR and emoji variant selection remain
Stage 6. Han readings and CJK field queries are supported as described in
[international browsing](international-browsing.md). Font and legacy scopes use
repertoire constraints.

## Presentation and selection

GTK GridView recycles XML-backed tiles. The native result vector uses 32-bit
opaque identities and the GTK model contains at most 512 IDs per page. Page
changes reset selection and scroll position, and do not rerun the query. The
selected character name and complete code-point notation are exposed separately
from the visible glyph. Invisible control and separator scalars render a visible
placeholder; combining marks use a dotted circle. Neither changes the underlying
sequence. NUL has a code point and display representation but cannot be encoded
as a NUL-terminated UTF-8 string; no copy operation is offered by this stage.

The grid uses GTK's native selection and keyboard navigation. Ctrl+F focuses the
search entry. The responsive sidebar collapses at a narrow window width and can
be reopened with the header toggle. The UI inherits system styling without
custom CSS. Actual font coverage, rendering quality, accessibility behavior,
and compact-window allocation remain runtime qualification requirements.

## Native request lifecycle

The Vala `QueryParser` interprets bounded UTF-8 input and shares explicit
numeric decoding with the inspector. `LanguageSelection` resolves locale and
XKB collection policies through the immutable registry and ICU adapter.
`SearchRequest` captures input before worker execution; the worker prepares
and copies scalar candidates, exact-match flags, metadata terms and the script
mask into `CharactersSearch`. `SemanticFilters` builds a copied union of
property/range clauses once per request. `HanQuery` folds text through ICU and
selects field masks or validates numeric group operands before scanning. The
native scanner is run at most once and owns
its result vector; it does not interpret raw query syntax. Its only concurrent operation is atomic cancellation.
The worker checks cancellation every 256 scalar candidates and each sequence.
Cancellation/failure does not publish a partial vector. After completion, the
main-context consumer may read the immutable vector. The request and results are
released when their owning Vala `SearchRequest` is no longer referenced. Scalar
and emoji result IDs are deduplicated using a bounded bitset; IDs are local to the
current catalog and must not be serialized for favorites or history.

The Vala controller permits at most one active worker and one newest pending
request. Every input or filter change immediately clears the old UI, cancels the
active request and replaces pending work. The main context alone publishes the
latest result; a completed-but-superseded worker cannot change the UI. Closing
or disposing the window disconnects its completion receiver and requests
cancellation without waiting on the GTK thread. The worker retains its request
and controller until idle delivery; there are no persistent threads, network
operations, timers, or cross-application dependencies.

### Prepared traversal and performance (1.0.20)

All production requests prepare a single Vala traversal plan after successful
query and collection preparation. A missing or invalid plan cannot publish
results. Explicit repertoires retain caller ordering, including empty input,
and never expand into the global catalog. The native scanner uses immutable
property spans and borrowed sequence scalars, with atomic cancellation checks
before spans and at bounded intervals within included spans. See
[scanner evaluation](native-search-scanner-evaluation.md) for measured local
native compatibility and the optional production-path benchmark.
