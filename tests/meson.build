python = import('python').find_installation('python3')
app_test = files('integration/test_application.py')
test_environment = environment()
test_environment.set('GSETTINGS_SCHEMA_DIR', meson.project_build_root() / 'data')

test('Declaration documentation', python,
  args: [meson.project_source_root() / 'tools/check_documentation.py',
         meson.project_source_root()],
  suite: 'source',
)
test('Application command-line options', python,
  args: [app_test, characters_executable, meson.project_version(), 'cli'],
  env: test_environment,
  depends: schema_target,
  suite: 'integration',
  timeout: 45,
)
test('Application window lifecycle', find_program('xvfb-run'),
  args: ['-a', find_program('dbus-run-session').full_path(), '--', python.full_path(),
         app_test, characters_executable, meson.project_version(), 'gui'],
  env: test_environment,
  depends: [characters_executable, schema_target],
  suite: 'integration',
  timeout: 60,
)

unicode_native_test = executable(
  'test-unicode-engine',
  'unit/test-unicode-engine.c',
  dependencies: unicode_dep,
  c_args: ['-UNDEBUG'],
)
test('Unicode engine contracts', unicode_native_test, suite: 'unit', timeout: 30)

unicode_binding_test = executable(
  'test-unicode-bindings',
  'unit/test-unicode-bindings.vala',
  unicode_sources,
  unicode_domain_vapis,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Unicode Vala bindings', unicode_binding_test, suite: 'unit', timeout: 30)

prepared_search_test = executable(
  'test-prepared-search',
  'unit/test-prepared-search.c',
  dependencies: unicode_dep,
  c_args: ['-UNDEBUG'],
)
test('Prepared search boundary contracts', prepared_search_test, suite: 'unit', timeout: 30)

search_contract_driver = files('support/search-contract-driver.vala',
  'support/search-contract-tests.vapi')
search_contract_include = include_directories('support')

character_search_test = executable(
  'test-character-search',
  'unit/test-character-search.c',
  search_contract_driver,
  search_domain_sources,
  search_domain_vapis,
  include_directories: search_contract_include,
  dependencies: [gio_dep, unicode_dep, dependency('threads')],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Character search contracts', character_search_test, suite: 'unit', timeout: 30)

search_controller_test = executable(
  'test-search-controller',
  'unit/test-search-controller.vala',
  search_sources,
  search_domain_sources,
  search_domain_vapis,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Search bindings and request supersession', search_controller_test,
  suite: 'unit', timeout: 30)

input_source_schema = custom_target(
  'input-source-test-schema',
  input: 'fixtures/input-sources/org.gnome.desktop.input-sources.gschema.xml',
  output: 'gschemas.compiled',
  command: [find_program('glib-compile-schemas'), '--strict', '--targetdir', '@OUTDIR@',
            meson.current_source_dir() / 'fixtures/input-sources'],
  build_by_default: true,
)
input_source_environment = environment()
input_source_environment.set('GSETTINGS_SCHEMA_DIR', meson.current_build_dir())
input_source_environment.set('GSETTINGS_BACKEND', 'memory')
input_source_policy_test = executable(
  'test-input-source-policy',
  'unit/test-input-source-policy.vala',
  keyboard_source_adapter,
  search_sources,
  search_domain_sources,
  search_domain_vapis,
  dependencies: [gio_dep, ibus_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Configured input source policy', input_source_policy_test,
  env: input_source_environment, depends: input_source_schema, suite: 'unit', timeout: 30)

character_browser_test = executable(
  'test-character-browser',
  'integration/test-character-browser.vala',
  application_sources,
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
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Character browser interaction', find_program('xvfb-run'),
  args: ['-a', find_program('dbus-run-session').full_path(), '--', python.full_path(),
         app_test, character_browser_test, meson.project_version(), 'browser'],
  env: test_environment,
  depends: [character_browser_test, schema_target],
  suite: 'integration',
  timeout: 90,
)

icu_adapter_test = executable(
  'test-icu-adapter',
  'unit/test-icu-adapter.c',
  dependencies: unicode_dep,
  c_args: ['-UNDEBUG'],
)
test('ICU adapter contracts', icu_adapter_test, suite: 'unit', timeout: 30)

character_inspector_test = executable(
  'test-character-inspector',
  'unit/test-character-inspector.vala',
  unicode_sources,
  unicode_domain_vapis,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Character inspector contracts', character_inspector_test, suite: 'unit', timeout: 30)

inspector_binding_test = executable(
  'test-inspector-bindings',
  'unit/test-inspector-bindings.vala',
  unicode_sources,
  unicode_domain_vapis,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Inspector Vala bindings', inspector_binding_test, suite: 'unit', timeout: 30)

legacy_adapter_test = executable(
  'test-legacy-adapter',
  'unit/test-legacy-adapter.c',
  dependencies: unicode_dep,
  c_args: ['-UNDEBUG'],
)
test('Native legacy adapter contracts', legacy_adapter_test, suite: 'unit', timeout: 30)

legacy_encoding_test = executable(
  'test-legacy-encoding',
  'unit/test-legacy-encoding.c',
  legacy_sources,
  legacy_vapi,
  search_contract_driver,
  search_domain_sources,
  search_domain_vapis,
  include_directories: search_contract_include,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Legacy encoding and repertoire contracts', legacy_encoding_test,
  suite: 'unit', timeout: 30)

font_catalog_test = executable(
  'test-font-catalog',
  'unit/test-font-catalog.c',
  dependencies: font_dep,
  c_args: ['-UNDEBUG'],
)
test('Installed font catalog contracts', font_catalog_test, suite: 'unit', timeout: 30)

font_sources_test = executable(
  'test-font-sources',
  'unit/test-font-sources.vala',
  legacy_sources,
  font_sources,
  search_domain_sources,
  search_domain_vapis,
  font_vapi,
  legacy_vapi,
  dependencies: [gio_dep, gtk_dep, font_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
font_test_environment = environment()
font_test_environment.set('CHARACTERS_FONT_FIXTURES',
  meson.current_source_dir() / 'fixtures/fonts')
test('Font and legacy source bindings and supersession', font_sources_test,
  env: font_test_environment,
  suite: 'unit', timeout: 30)

emoji_catalog_test = executable(
  'test-emoji-catalog',
  'unit/test-emoji-catalog.c',
  search_contract_driver,
  unicode_sources,
  unicode_domain_vapis,
  include_directories: search_contract_include,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Emoji variations and localized annotation search', emoji_catalog_test,
  suite: 'unit', timeout: 30)

emoji_binding_test = executable(
  'test-emoji-bindings',
  'unit/test-emoji-bindings.vala',
  unicode_sources,
  unicode_domain_vapis,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Emoji Vala bindings', emoji_binding_test, suite: 'unit', timeout: 30)

composition_test = executable(
  'test-composition',
  'unit/test-composition.vala',
  unicode_sources,
  unicode_domain_vapis,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args + ['-UNDEBUG'],
)
test('Composition graphemes, representations and normalization', composition_test,
  suite: 'unit', timeout: 30)

library_store_test = executable(
  'test-library-store',
  'unit/test-library-store.vala',
  library_sources,
  app_config,
  dependencies: gio_dep,
  c_args: ['-UNDEBUG'],
)
test('Saved collection ordering and privacy', library_store_test,
  env: test_environment,
  depends: schema_target,
  suite: 'unit',
  timeout: 30,
)
test('Saved collections across process restarts', python,
  args: [files('integration/test_library_persistence.py'), library_store_test],
  env: test_environment,
  depends: [library_store_test, schema_target],
  suite: 'integration',
  timeout: 45,
)

test('Input source response protocol', python,
  args: [files('unit/test_input_protocol.py')],
  suite: 'unit',
  timeout: 10,
)

test('Release source contracts', python,
  args: [files('release/test_source_contracts.py')],
  suite: 'source',
  timeout: 10,
)

test('German translation formats', find_program('msgfmt'),
  args: ['--check-format', '-o', '/dev/null',
         meson.project_source_root() / 'po/de.po'],
  suite: 'source',
  timeout: 10,
)

test('Spanish translation formats', find_program('msgfmt'),
  args: ['--check-format', '-o', '/dev/null',
         meson.project_source_root() / 'po/es.po'],
  suite: 'source',
  timeout: 10,
)

test('French translation formats', find_program('msgfmt'),
  args: ['--check-format', '-o', '/dev/null',
         meson.project_source_root() / 'po/fr.po'],
  suite: 'source',
  timeout: 10,
)

test('Unicode 17 algorithm snapshot contracts', python,
  args: [files('unit/test_algorithm_data.py')],
  suite: 'unit', timeout: 30,
)

test('Unihan offline importer contracts', python,
  args: [files('unit/test_unihan_import.py')],
  suite: 'unit',
  timeout: 10,
)

test('Emoji offline data and coverage contracts', python,
  args: [files('unit/test_emoji_data.py')],
  suite: 'unit',
  timeout: 120,
)

test('Official Unicode data archive import', python,
  args: [files('unit/test_official_data_import.py')],
  suite: 'unit', timeout: 120,
)

font_face_test = executable(
  'test-font-face',
  'unit/test-font-face.c',
  dependencies: font_dep,
  c_args: ['-UNDEBUG'],
)
test('Exact font face and variation contracts', font_face_test,
  args: files('fixtures/fonts/faces.ttc', 'fixtures/fonts/variable.ttf',
              'fixtures/fonts/symbol.ttf'),
  suite: 'unit', timeout: 30,
)

executable(
  'benchmark-search',
  'benchmarks/benchmark-search.vala',
  search_domain_sources,
  search_domain_vapis,
  dependencies: [gio_dep, unicode_dep],
  c_args: app_c_args,
  build_by_default: false,
  install: false,
)

executable(
  'unicode-conformance-probe',
  'extended/unicode-conformance-probe.c',
  dependencies: unicode_dep,
  build_by_default: false,
  install: false,
)
