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
