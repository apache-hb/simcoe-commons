# SPDX-License-Identifier: Apache-2.0

inc = include_directories('include')

src = files('src/limiting_flag.cpp')

libsimcoe_concurrent = library(
  'simcoe-concurrent',
  src,
  include_directories: inc,
  cpp_args: ['-DSM_CONCURRENT_API_EXPORT=1'],
  version: meson.project_version(),
  install: true,
  gnu_symbol_visibility: 'hidden',
)

simcoe_concurrent_dep = declare_dependency(
  link_with: libsimcoe_concurrent,
  include_directories: include_directories('include'),
)

meson.override_dependency(
  'simcoe-concurrent',
  simcoe_concurrent_dep,
)

pkg.generate(
  libsimcoe_concurrent,
  description: 'simcoe-commons concurrent module',
  url: url,
)

install_headers(
  'include/simcoe/concurrent/exports.hpp',
  'include/simcoe/concurrent/annotations.hpp',
  'include/simcoe/concurrent/mailbox.hpp',
  'include/simcoe/concurrent/limiting_flag.hpp',
  'include/simcoe/concurrent/ring_buffer.hpp',
  subdir: 'simcoe/concurrent',
)

install_headers(
  'include/simcoe/concurrent/detail/ring_buffer_bitset_detail.hpp',
  subdir: 'simcoe/concurrent/detail',
)

if gtest_main.found()
  testcases = {
    'mailbox': {
      'sources': files('test/mailbox_test.cpp'),
    },
    'limiting flag': {
      'sources': files('test/limiting_flag_test.cpp'),
    },
    'ring buffer': {
      'sources': files('test/ring_buffer_test.cpp'),
    },
  }

  foreach testcase_name, testcase_data : testcases
    executable_name = 'simcoe_concurrent_' + testcase_name.replace(' ', '_') + '_test'
    exe = executable(
      executable_name,
      testcase_data['sources'],
      dependencies: [gtest_main, simcoe_concurrent_dep],
    )
    test(
      testcase_name,
      exe,
      suite: 'concurrent',
    )
  endforeach
endif
