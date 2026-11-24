unicode_include = include_directories('.')
unicode_engine = static_library(
  'characters-unicode',
  'character-search.c',
  'unicode-engine.c',
  include_directories: unicode_include,
  dependencies: icu_dep,
)
unicode_dep = declare_dependency(
  link_with: unicode_engine,
  include_directories: unicode_include,
  dependencies: icu_dep,
)
unicode_vapi = files('../../vapi/characters-unicode.vapi')
search_vapi = files('../../vapi/characters-search.vapi')
