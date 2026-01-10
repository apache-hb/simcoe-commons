# SPDX-License-Identifier: Apache-2.0

inc = include_directories('include')

simcoe_container_dep = declare_dependency(
  include_directories: inc,
)

meson.override_dependency(
  'simcoe-container',
  simcoe_container_dep,
)

pkg.generate(
  name: 'simcoe-container',
  description: 'simcoe-commons container module',
  url: url,
)

install_headers(
  'include/simcoe/container/integer.hpp',
  subdir: 'simcoe/container',
)

if gtest_main.found()
  testcases = {
    'integer': {
      'sources': files('test/integer_test.cpp'),
    },
  }

  foreach testcase_name, testcase_data : testcases
    executable_name = 'simcoe_container_' + testcase_name.replace(' ', '_') + '_test'
    exe = executable(
      executable_name,
      testcase_data['sources'],
      dependencies: [gtest_main, simcoe_container_dep],
    )
    test(
      testcase_name,
      exe,
      suite: 'container',
    )
  endforeach
endif
