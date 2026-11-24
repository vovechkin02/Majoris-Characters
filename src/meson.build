app_config = configure_file(
  input: 'config.vala.in',
  output: 'config.vala',
  configuration: configuration,
)

app_sources = files(
  'app/application.vala',
  'views/main-window.vala',
  'app/main.vala',
)

characters_executable = executable(
  meson.project_name(),
  app_sources,
  app_config,
  app_resources,
  dependencies: [adwaita_dep, gio_dep, gtk_dep],
  vala_args: ['--gresources=' + resources_manifest,
              '--gresourcesdir=' + (meson.project_source_root() / 'data')],
  c_args: ['-DGETTEXT_PACKAGE="' + gettext_package + '"'],
  install: true,
)
