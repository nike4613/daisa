project('daisa', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20'])

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_DAISA']

shlib = static_library('daisa', 'daisa.cpp',
  install : true,
  cpp_args : lib_args,
  gnu_symbol_visibility : 'hidden',
  pic : true,
)

test_exe = executable('daisa_test', 'daisa_test.cpp',
  link_with : shlib)
test('daisa', test_exe)

# Make this library usable as a Meson subproject.
daisa_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with : shlib)

# Make this library usable from the system's
# package manager.
install_headers('daisa.hpp', subdir : 'daisa')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'daisa',
  filebase : 'daisa',
  description : 'Meson sample project.',
  subdirs : 'daisa',
  libraries : shlib,
  version : '0.1',
)
