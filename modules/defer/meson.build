# SPDX-License-Identifier: Apache-2.0

inc = include_directories('include')

simcoe_defer_dep = declare_dependency(
  include_directories: inc,
)

meson.override_dependency(
  'simcoe-defer',
  simcoe_defer_dep,
)

install_headers(
  'include/simcoe/defer/defer.hpp',
  subdir: 'simcoe/defer',
)

if gtest_main.found()
  testcases = {
    'defer': {
      'sources': files('test/defer_test.cpp'),
    },
  }

  foreach testcase_name, testcase_data : testcases
    executable_name = 'simcoe_defer_' + testcase_name.replace(' ', '_') + '_test'
    exe = executable(
      executable_name,
      testcase_data['sources'],
      dependencies: [gtest_main, simcoe_defer_dep],
    )
    test(
      testcase_name,
      exe,
      suite: 'defer',
    )
  endforeach
endif
