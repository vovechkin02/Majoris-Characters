library_include = include_directories('.')
library_native = static_library(
  'characters-collections',
  'collection-lock.c',
  dependencies: [glib_dep, gio_dep],
)
library_dep = declare_dependency(
  link_with: library_native,
  include_directories: library_include,
  dependencies: [glib_dep, gio_dep],
)
library_vapi = files('../../vapi/characters-collections.vapi')
