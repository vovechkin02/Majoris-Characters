font_include = include_directories('.')
font_catalog = static_library(
  'characters-fonts',
  'font-catalog.c',
  'font-face.c',
  dependencies: [fontconfig_dep, freetype_dep, pangoft2_dep],
)
font_dep = declare_dependency(
  link_with: font_catalog,
  include_directories: font_include,
  dependencies: [fontconfig_dep, freetype_dep, pangoft2_dep],
)
font_vapi = files('../../vapi/characters-fonts.vapi')
