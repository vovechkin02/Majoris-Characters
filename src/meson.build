app_config = configure_file(
  input: 'config.vala.in',
  output: 'config.vala',
  configuration: configuration,
)

search_sources = files(
  'search/search-controller.vala',
  'search/search-request.vala',
)
font_sources = files(
  'fonts/font-inventory.vala',
  'fonts/font-source-loader.vala',
  'fonts/font-source-request.vala',
)
library_sources = files(
  'library/library-store.vala',
  'library/usage-entry.vala',
)
integration_sources = files(
  'integration/desktop-search.vala',
  'integration/search-provider.vala',
)
application_sources = files('app/application.vala') + integration_sources + font_sources + library_sources + search_sources + files(
  'views/character-inspector.vala',
  'views/character-tile.vala',
  'views/compact-picker.vala',
  'views/composer-view.vala',
  'views/emoji-variations.vala',
  'views/font-browser.vala',
  'views/library-dialog.vala',
  'views/main-window.vala',
  'views/preferences-dialog.vala',
)
app_entry = files('app/main.vala')
app_vala_args = [
  '--gresources=' + resources_manifest,
  '--gresourcesdir=' + (meson.project_source_root() / 'data'),
]
app_c_args = ['-DGETTEXT_PACKAGE="' + gettext_package + '"']
app_dependencies = [adwaita_dep, gio_dep, gtk_dep, font_dep, unicode_dep]

characters_executable = executable(
  meson.project_name(),
  application_sources,
  app_entry,
  app_config,
  unicode_vapi,
  search_vapi,
  inspector_vapi,
  font_vapi,
  legacy_vapi,
  emoji_vapi,
  app_resources,
  dependencies: app_dependencies,
  vala_args: app_vala_args,
  c_args: app_c_args,
  install: true,
)
