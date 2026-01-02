parts = meson.project_version().split('.')

configure_file(
  input: 'version.hpp.in',
  output: 'version.hpp',
  install_dir: 'include/simcoe/version',
  configuration: {
    'MAJOR': parts[0],
    'MINOR': parts[1],
    'PATCH': parts[2],
  },
)
