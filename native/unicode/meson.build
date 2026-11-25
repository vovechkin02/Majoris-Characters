unicode_include = include_directories('.')
unicode_engine = static_library(
  'characters-unicode',
  'character-inspector.c',
  'character-search.c',
  'composition.c',
  'emoji-catalog.c',
  'legacy-encoding.c',
  'normalization.c',
  'scalar-properties.c',
  'unicode-engine.c',
  include_directories: unicode_include,
  dependencies: [icu_dep, icu_i18n_dep],
)
unicode_dep = declare_dependency(
  link_with: unicode_engine,
  include_directories: unicode_include,
  dependencies: [icu_dep, icu_i18n_dep],
)
unicode_vapi = files('../../vapi/characters-unicode.vapi')
search_vapi = files('../../vapi/characters-search.vapi')
inspector_vapi = files('../../vapi/characters-inspector.vapi')
legacy_vapi = files('../../vapi/characters-legacy.vapi')
emoji_vapi = files('../../vapi/characters-emoji.vapi')
