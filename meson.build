project('daisa_meta', 'cpp',
  default_options : ['warning_level=3', 'cpp_std=c++20'])

daisa_core_proj = subproject('daisa')
daisa_core_dep = daisa_core_proj.get_variable('daisa_dep')

executable('main', 'main.cpp', dependencies : daisa_core_dep)