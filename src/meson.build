app_config = configure_file(
  input: 'config.vala.in',
  output: 'config.vala',
  configuration: configuration,
)

search_domain_sources = files(
  'emoji/annotation-policy.vala',
  'emoji/presentation.vala',
  'search/collection-catalog.vala',
  'search/han-query.vala',
  'search/language-selection.vala',
  'search/query-parser.vala',
  'search/search-plan.vala',
  'search/semantic-filters.vala',
  'unicode/scalar-sequence.vala',
  'unicode/character-presentation.vala',
  'unicode/property-metadata.vala',
  'unicode/sequence-format.vala',
)
search_domain_vapis = unicode_vapi + search_vapi + emoji_vapi + language_vapi + han_vapi + icu_vapi

unicode_sources = search_domain_sources + files(
  'emoji/variation-family.vala',
  'emoji/variants.vala',
  'unicode/composition.vala',
  'unicode/inspection-snapshot.vala',
  'unicode/relationships.vala',
)
unicode_domain_vapis = search_domain_vapis

search_sources = files(
  'search/search-controller.vala',
  'search/search-request.vala',
)
legacy_sources = files('encodings/legacy-workflow.vala')
font_sources = files(
  'fonts/font-face-record.vala',
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
application_sources = files('app/application.vala', 'library/library-query.vala') + integration_sources + font_sources + legacy_sources + library_sources + search_sources + unicode_sources + files(
  'views/character-inspector.vala',
  'views/character-tile.vala',
  'views/compact-picker.vala',
  'views/composer-view.vala',
  'views/emoji-variations.vala',
  'views/font-browser.vala',
  'views/font-face-dialog.vala',
  'views/font-preview.vala',
  'views/legacy-sequence-dialog.vala',
  'views/library-dialog.vala',
  'views/main-window.vala',
  'views/preferences-dialog.vala',
  'views/property-labels.vala',
  'views/shortcuts-dialog.vala',
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
  icu_vapi,
  font_vapi,
  legacy_vapi,
  emoji_vapi,
  language_vapi,
  han_vapi,
  app_resources,
  dependencies: app_dependencies,
  vala_args: app_vala_args,
  c_args: app_c_args,
  install: true,
)
