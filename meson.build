project('logfile', 'cpp')

add_project_arguments(
  ['-std=gnu++17', '-Wall', '-Wextra', '-pedantic', '-Wfatal-errors', '-O4'],
  language: 'cpp')

pydep = dependency('python3', version : '>=3.5')

r = run_command('python3', '-m', 'pybind11', '--includes')
py_include_flags = r.stdout().strip().split(' ')

r = run_command('python3-config', '--extension-suffix')
py_suffix_split = r.stdout().strip().split('.')
py_suffix = '.'.join([py_suffix_split[1], py_suffix_split[2]])

shared_library('logfile',
               sources : 'py_logfile.cpp',
               dependencies : pydep,
               name_prefix : '',
               name_suffix : py_suffix,
               cpp_args : py_include_flags)

executable('logfilereader',
           sources : 'main.cpp')
