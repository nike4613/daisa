project('daisa', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20'])

includes = include_directories('include')

shlib = library('daisa', 
  'daisa.cpp',
  include_directories : includes,
)

test_exe = executable('daisa_test', 'daisa_test.cpp',
  link_with : shlib,
  include_directories : includes)

test('instruction', test_exe, args: ['instruction'])

# Make this library usable as a Meson subproject.
daisa_dep = declare_dependency(
  include_directories : includes,
  link_with : shlib)
