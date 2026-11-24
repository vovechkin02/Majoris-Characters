# Emoji annotation input

`annotations.json.gz` is a lossless export of the names, keyword lists, and
code-point fields in Debian 13's GTK 3.24.49-3 emoji chooser resources. The
English resource was read from `libgtk-3.so.0` after GTK initialization; the
other 23 resources were read from `/usr/share/gtk-3.0/emoji/*.gresource`.
Each serialized resource is a GVariant of type `a(ausasu)` (code points,
name, keywords, group). The group field is not needed by Characters. Every
locale contained 1,915 source records. The Emoji catalog itself comes from
the separately vendored Unicode 17 GNOME Characters tables, not GTK 3.

GTK's Debian copyright manifest identifies the bundled Emojibase source as
Expat-licensed (Miles Johnson, 2017–2019); the original GTK Debian copyright
manifest is preserved here. Unicode source data attribution and license are
preserved in `../supplement/LICENSE.unicode`. The precise upstream CLDR release
used by these GTK resources was not established; do not label the dataset CLDR
48 or Unicode 17. This is not a GTK3 runtime dependency.

The compressed input is the source for regeneration:

```sh
python3 tools/unicode/generate_emoji_annotations.py
```

To replace it with locally acquired CLDR annotation and derived-annotation XML
files, use the CLDR `common` directory and its actual version:

```sh
python3 tools/unicode/generate_emoji_annotations.py \
  --cldr-common /path/to/cldr/common --cldr-version 48
```

Review changed provenance, locale coverage, and licenses, and rerun native
and GTK tests. The importer accepts explicit or resolved annotations; it does
not implement every CLDR inheritance or alias rule. Resolved CLDR input is
preferred for complete regional data. No network access is used by the generator.
