project('jaybird', 'cpp', default_options: ['cpp_std=c++23', 'warning_level=3'])

fmt_sub = subproject('fmt', default_options: ['default_library=static'])
fmt_dep = fmt_sub.get_variable('fmt_dep')
json_dep = dependency('nlohmann-json')
thesauros_dep = dependency('thesauros')

jaybird_dep = declare_dependency(
  include_directories: include_directories('include'),
  dependencies: [fmt_dep, json_dep, thesauros_dep],
)

install_subdir(
  'include',
  install_dir: 'include',
)

pc = import('pkgconfig')
pc.generate(
  name: 'jaybird',
  version: meson.project_version(),
  description: 'JSON support library',
)

if get_option('test')
  subdir('test')
endif
