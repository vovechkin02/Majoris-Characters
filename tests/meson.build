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
  unicode_vapi,
  dependencies: [gio_dep, unicode_dep],
  c_args: ['-UNDEBUG'],
)
test('Unicode Vala bindings', unicode_binding_test, suite: 'unit', timeout: 30)

character_search_test = executable(
  'test-character-search',
  'unit/test-character-search.c',
  dependencies: [unicode_dep, dependency('threads')],
  c_args: ['-UNDEBUG'],
)
test('Character search contracts', character_search_test, suite: 'unit', timeout: 30)

search_controller_test = executable(
  'test-search-controller',
  'unit/test-search-controller.vala',
  search_sources,
  search_vapi,
  unicode_vapi,
  dependencies: [gio_dep, unicode_dep],
  c_args: ['-UNDEBUG'],
)
test('Search bindings and request supersession', search_controller_test,
  suite: 'unit', timeout: 30)

character_browser_test = executable(
  'test-character-browser',
  'integration/test-character-browser.vala',
  application_sources,
  app_config,
  unicode_vapi,
  search_vapi,
  inspector_vapi,
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
  timeout: 60,
)

character_inspector_test = executable(
  'test-character-inspector',
  'unit/test-character-inspector.c',
  dependencies: unicode_dep,
  c_args: ['-UNDEBUG'],
)
test('Character inspector contracts', character_inspector_test, suite: 'unit', timeout: 30)

inspector_binding_test = executable(
  'test-inspector-bindings',
  'unit/test-inspector-bindings.vala',
  inspector_vapi,
  dependencies: [gio_dep, unicode_dep],
  c_args: ['-UNDEBUG'],
)
test('Inspector Vala bindings', inspector_binding_test, suite: 'unit', timeout: 30)
