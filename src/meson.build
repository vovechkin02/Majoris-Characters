app_config = configure_file(
  input: 'config.vala.in',
  output: 'config.vala',
  configuration: configuration,
)

search_sources = files(
  'search/search-controller.vala',
  'search/search-request.vala',
)
application_sources = files('app/application.vala') + search_sources + files(
  'views/character-tile.vala',
  'views/main-window.vala',
)
app_entry = files('app/main.vala')
app_vala_args = [
  '--gresources=' + resources_manifest,
  '--gresourcesdir=' + (meson.project_source_root() / 'data'),
]
app_c_args = ['-DGETTEXT_PACKAGE="' + gettext_package + '"']
app_dependencies = [adwaita_dep, gio_dep, gtk_dep, unicode_dep]

characters_executable = executable(
  meson.project_name(),
  application_sources,
  app_entry,
  app_config,
  unicode_vapi,
  search_vapi,
  app_resources,
  dependencies: app_dependencies,
  vala_args: app_vala_args,
  c_args: app_c_args,
  install: true,
)
