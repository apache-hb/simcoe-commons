# SPDX-License-Identifier: Apache-2.0

project(
  'simcoe-commons',
  'cpp',
  version: '0.1.0',
  license: 'Apache-2.0',
  meson_version: '>=0.60.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
  ],
)

pkg = import('pkgconfig')
url = 'https://github.com/apache-hb/simcoe-commons'

tests_opt = get_option('tests').disable_auto_if(meson.is_subproject())
docs_opt = get_option('docs').disable_auto_if(meson.is_subproject())

gtest_main = dependency(
  'gtest_main',
  required: tests_opt,
)

doxygen = find_program(
  'doxygen',
  required: docs_opt,
)

subdir('modules/version')
subdir('modules/defer')
subdir('modules/concurrent')

if doxygen.found()
  doxygen_config = configure_file(
    input: 'resources/Doxyfile.in',
    output: 'Doxyfile',
    configuration: {
      'PROJECT_NAME': meson.project_name(),
      'PROJECT_VERSION': meson.project_version(),
      'OUTPUT_DIRECTORY': meson.current_build_dir() / 'docs',
      'INPUT': meson.project_source_root() / 'modules',
      'CLANG_DATABASE_PATH': meson.global_build_root() / 'compile_commands.json',
      'CITE_BIB_FILES': meson.project_source_root() / 'resources' / 'cite.bib',
    },
  )

  run_target(
    'doxygen',
    command: [doxygen, doxygen_config],
  )
endif
