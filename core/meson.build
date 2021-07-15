

daisa_inc = include_directories('include')

daisa_lib = static_library('daisa', 
  'daisa.cpp',
  include_directories : daisa_inc,
)

core_test_exe = executable('daisa_test', 'daisa_test.cpp',
  link_with : daisa_lib,
  include_directories : daisa_inc)

test('instruction', core_test_exe, args: ['instruction'])
test('assemble_blocks', core_test_exe, args: ['assemble_blocks'])

# Make this library usable as a Meson subproject.
daisa_dep = declare_dependency(
  include_directories : daisa_inc,
  link_with : daisa_lib)
