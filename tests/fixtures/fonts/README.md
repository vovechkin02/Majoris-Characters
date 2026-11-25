# Original font test fixtures

These three small fonts were generated from original rectangular outlines by
`tests/support/generate_font_fixtures.py`. They are project-owned test data,
licensed under GPL-3.0-or-later, and are not installed by the application.
They contain no third-party glyphs or font files. Regenerate with FontTools
4.63.0; no FontTools build or runtime dependency is required for normal tests.

- `faces.ttc`: two different outlines at the same Unicode code point, sharing
  a family name but with distinct exact collection indexes.
- `variable.ttf`: `wght` axis (100–900), with a named Heavy instance at 900.
- `symbol.ttf`: Microsoft Symbol cmap only, mapping raw codes 0xF041 and 0xF042.
