# Emoji annotation input

`annotations.json.gz` contains the imported CLDR 48 base and derived annotations
with parent-locale fields resolved for 171 locale tables. The original archive
matched its pinned release checksum; the source URL, digest and output hashes
are recorded in `../official-data.json`. CLDR annotations are Unicode-3.0;
see `../supplement/LICENSE.unicode`. The original archive remains an external
maintainer input, while the compressed resolved snapshot is bundled for offline
regeneration. The original GTK/Emojibase copyright notice and eight early CLDR
English additions remain as historical source references, not the current
production annotation source.

The compressed input is the source for regeneration:

```sh
python3 tools/unicode/generate_emoji_annotations.py --coverage data/unicode/emoji/coverage.json
```

To replace it with locally acquired CLDR annotation and derived-annotation XML
files, use the CLDR `common` directory and its actual version:

```sh
python3 tools/unicode/generate_emoji_annotations.py \
  --cldr-common /path/to/cldr/common --cldr-version 48 \
  --coverage data/unicode/emoji/coverage.json
```

Review changed provenance, locale coverage, and licenses, and rerun native
and GTK tests. The importer accepts explicit or resolved annotations; it does
not implement every CLDR inheritance or alias rule. Resolved CLDR input is
preferred for complete regional data. No network access is used by the generator.

Use `--require-complete en` (repeat for each required locale) to reject imports
with missing direct or tone-base annotation keys before replacing output. The
coverage report intentionally excludes fallback to parent locales and English.

## Standardized presentation and qualification data

`presentation-bases.txt` is a compact transcription of the base scalar column
of Unicode 17.0 `emoji-variation-sequences.txt`, preserving all 371 bases with
both FE0E and FE0F. Its source URL, copyright and license are in the file.
`generate_emoji_sequences.py` combines this with the pinned fully qualified
GNOME catalog to produce supplementary exact sequence identities. It can verify
the local snapshot against acquired official files:

```sh
python3 tools/unicode/generate_emoji_sequences.py \
  --official /path/to/emoji-variation-sequences.txt \
  --emoji-test /path/to/emoji-test.txt
```

Without these optional files the tool regenerates the checked-in data. The
published qualification counts match the generated selector omissions, but
full file-level comparison remains outstanding. Unicode data attribution and
license are in `../supplement/LICENSE.unicode`.

For the pinned CLDR 48 release, prefer the paired verified archive workflow
in [official data import](../../../docs/official-data-import.md). It imports
base and derived files, resolves explicit parent-locale fields, and preserves
actual coverage and output digests. The original CLDR archive is an external
maintainer input; the compressed imported snapshot is retained for offline
regeneration. The verified CLDR 48 archive has been imported in 1.0.22.
