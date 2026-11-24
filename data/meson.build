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
install_data('desktop/com.majoris.characters.desktop',
  install_dir: get_option('datadir') / 'applications')
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
