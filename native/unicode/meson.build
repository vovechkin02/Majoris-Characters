unicode_include = include_directories('.')
unicode_engine = static_library(
  'characters-unicode',
  'character-search.c',
  'icu-adapter.c',
  'emoji-catalog.c',
  'language-collections.c',
  'legacy-encoding.c',
  'normalization.c',
  'scalar-properties.c',
  'unihan.c',
  'unicode-algorithms.c',
  'unicode-engine.c',
  include_directories: unicode_include,
  dependencies: [glib_dep, icu_dep, icu_i18n_dep],
)
# The native target carries the ICU link dependencies. Mixed C/Vala
# consumers need the headers without passing ICU package names to valac.
icu_headers_dep = declare_dependency(
  include_directories: icu_dep.get_variable(pkgconfig: 'includedir'),
)
unicode_dep = declare_dependency(
  link_with: unicode_engine,
  include_directories: unicode_include,
  dependencies: [glib_dep, icu_headers_dep],
)
unicode_vapi = files('../../vapi/characters-unicode.vapi')
search_vapi = files('../../vapi/characters-search.vapi')
icu_vapi = files('../../vapi/characters-icu.vapi')
legacy_vapi = files('../../vapi/characters-legacy.vapi')
emoji_vapi = files('../../vapi/characters-emoji.vapi')
language_vapi = files('../../vapi/characters-language.vapi')
han_vapi = files('../../vapi/characters-han.vapi')
