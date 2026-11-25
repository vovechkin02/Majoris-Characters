# Emoji annotation input

`annotations.json.gz` includes a lossless export of the names, keyword lists, and
code-point fields in Debian 13's GTK 3.24.49-3 emoji chooser resources. The
English resource was read from `libgtk-3.so.0` after GTK initialization; the
other 23 resources were read from `/usr/share/gtk-3.0/emoji/*.gresource`.
Each serialized resource is a GVariant of type `a(ausasu)` (code points,
name, keywords, group). The group field is not needed by Characters. Every
locale originally contained 1,915 source records. The eight English Emoji 17
base annotations in `cldr-48-en-additions.json` are incorporated into the
English snapshot (1,923 source records). These records come from
https://github.com/unicode-org/cldr/blob/release-48/common/annotations/en.xml
(Copyright 1991–2025 Unicode, Inc.; Unicode-3.0). The Emoji catalog itself comes from
the separately vendored Unicode 17 GNOME Characters tables, not GTK 3.

GTK's Debian copyright manifest identifies the bundled Emojibase source as
Expat-licensed (Miles Johnson, 2017–2019); the original GTK Debian copyright
manifest is preserved here. Unicode source data attribution and license are
preserved in `../supplement/LICENSE.unicode`. The precise upstream CLDR release
used by these GTK resources was not established; do not label the dataset CLDR
48 or Unicode 17. This is not a GTK3 runtime dependency.

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
