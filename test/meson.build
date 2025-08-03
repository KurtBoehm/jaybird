options_sub = subproject('options')
args = options_sub.get_variable('all_args')

foreach name, info : {
  'Serialization': [['serialization.cpp'], []],
}
  sources = info[0]
  deps = info[1]
  test(
    name,
    executable(
      'Test' + name,
      sources,
      cpp_args: args,
      dependencies: [jaybird_dep] + deps,
    ),
  )
endforeach
