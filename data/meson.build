resources_manifest = meson.current_source_dir() / 'characters.gresource.xml'
app_resources = gnome.compile_resources(
  'characters-resources',
  'characters.gresource.xml',
  source_dir: meson.current_source_dir(),
  c_name: 'characters',
)

schema_file = files('schemas/com.majoris.characters.gschema.xml')
schema_target = custom_target(
  'compiled-settings',
  input: schema_file,
  output: 'gschemas.compiled',
  command: [find_program('glib-compile-schemas'), '--strict',
            '--targetdir', meson.current_build_dir(),
            meson.current_source_dir() / 'schemas'],
  build_by_default: true,
)

install_data(schema_file, install_dir: get_option('datadir') / 'glib-2.0/schemas')
i18n.merge_file(
  input: 'desktop/com.majoris.characters.desktop',
  output: app_id + '.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'applications',
)
install_data('desktop/com.majoris.characters.metainfo.xml',
  install_dir: get_option('datadir') / 'metainfo')
install_data('icons/com.majoris.characters.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps')
configure_file(
  input: 'desktop/com.majoris.characters.service.in',
  output: app_id + '.service',
  configuration: configuration,
  install_dir: get_option('datadir') / 'dbus-1/services',
)

install_data(
  '../THIRD_PARTY_NOTICES.md',
  '../po/LICENSE.cldr',
  '../po/LICENSE.babel',
  '../native/unicode/vendor/gnome/COPYING',
  '../native/unicode/vendor/gnome/COPYINGv2',
  'unicode/supplement/LICENSE.unicode',
  'unicode/emoji/GTK-DEBIAN-COPYRIGHT',
  'unicode/han/ICU-DEBIAN-COPYRIGHT',
  'unicode/han/XKB-DEBIAN-COPYRIGHT',
  'unicode/han/LPPL-1.3c.tex',
  install_dir: get_option('datadir') / 'doc' / meson.project_name(),
)

install_data('desktop/com.majoris.characters.search-provider.ini',
  install_dir: get_option('datadir') / 'gnome-shell/search-providers')
install_data('shell/extension.js', 'shell/metadata.json',
  install_dir: get_option('datadir') / 'gnome-shell/extensions/characters-picker@majoris.com')
configure_file(
  input: 'ibus/majoris-characters.xml.in',
  output: 'majoris-characters.xml',
  configuration: configuration,
  install_dir: get_option('datadir') / 'ibus/component',
)
install_data('../tools/input/characters_input.py',
  install_dir: get_option('libexecdir') / 'majoris-characters',
  install_mode: 'rwxr-xr-x')
install_data('../tools/input/input_protocol.py',
  install_dir: get_option('libexecdir') / 'majoris-characters')
