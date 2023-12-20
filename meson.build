project('jaybird', 'cpp', default_options: ['cpp_std=c++20', 'warning_level=3'])

json_dep = dependency('nlohmann-json')
thesauros_dep = dependency('thesauros')

jaybird_dep = declare_dependency(
  include_directories: include_directories('include'),
  dependencies: [json_dep, thesauros_dep],
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
