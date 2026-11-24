project(
  'majoris-characters',
  ['c', 'vala'],
  version: '0.2.0',
  meson_version: '>=1.3.0',
  default_options: ['c_std=gnu11', 'warning_level=2'],
  license: 'GPL-3.0-or-later',
)

gnome = import('gnome')
i18n = import('i18n')
app_id = 'com.majoris.characters'
gettext_package = meson.project_name()

adwaita_dep = dependency('libadwaita-1', version: '>=1.7')
gio_dep = dependency('gio-2.0', version: '>=2.80')
gtk_dep = dependency('gtk4', version: '>=4.14')
icu_dep = dependency('icu-uc', version: '>=76')

configuration = configuration_data()
configuration.set('VERSION', meson.project_version())
configuration.set('APP_ID', app_id)
configuration.set('GETTEXT_PACKAGE', gettext_package)
configuration.set('LOCALEDIR', get_option('prefix') / get_option('localedir'))
configuration.set('EXECUTABLE', get_option('prefix') / get_option('bindir') / meson.project_name())

subdir('data')
subdir('native/unicode')
subdir('src')
subdir('po')
subdir('tests')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)
