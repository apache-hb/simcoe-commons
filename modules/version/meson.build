# SPDX-License-Identifier: Apache-2.0

inc = include_directories('include')

simcoe_version_dep = declare_dependency(
  include_directories: inc,
)

meson.override_dependency(
  'simcoe-version',
  simcoe_version_dep,
)

pkg.generate(
  name: 'simcoe-version',
  description: 'simcoe-commons version module',
  url: url,
)

subdir('include/simcoe/version')
