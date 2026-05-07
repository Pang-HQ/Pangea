project('pangea', 'cpp',
  version: '0.0.1',
  license: 'Apache-2.0',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3',
    'buildtype=debugoptimized',
  ])

cpp = meson.get_compiler('cpp')

# Project-wide warning flags layered on top of meson's warning_level=3.
# Filtered through get_supported_arguments() so the build does not
# break on toolchains that do not recognise a given flag (e.g. some
# of these are GCC-only).
extra_warnings = [
  '-Wconversion',
  '-Wsign-conversion',
  '-Wshadow',
  '-Wformat=2',
  '-Wnull-dereference',
  '-Wduplicated-cond',
  '-Wduplicated-branches',
  '-Wcast-qual',
  '-Wcast-align',
  '-Wold-style-cast',
  '-Wuseless-cast',
  '-Wmissing-declarations',
  '-Wundef',
  '-Wunused-const-variable=2',
]
add_project_arguments(cpp.get_supported_arguments(extra_warnings),
                      language: 'cpp')

# Debug builds layer on sanitisers, libstdc++ debug iterators, and
# FORTIFY_SOURCE. Configure with `meson setup --buildtype=debug build`
# (or `--buildtype=debugoptimized`, the default).
if get_option('buildtype').startswith('debug')
  debug_compile = [
    '-fsanitize=address,undefined,leak,pointer-compare,pointer-subtract',
    '-fno-omit-frame-pointer',
    '-fno-sanitize-recover=all',
    '-D_GLIBCXX_DEBUG',
    '-D_GLIBCXX_DEBUG_PEDANTIC',
    '-D_FORTIFY_SOURCE=3',
  ]
  debug_link = [
    '-fsanitize=address,undefined,leak,pointer-compare,pointer-subtract',
  ]
  add_project_arguments(cpp.get_supported_arguments(debug_compile),
                        language: 'cpp')
  add_project_link_arguments(cpp.get_supported_link_arguments(debug_link),
                             language: 'cpp')
endif

# external/ holds vendored single-header dependencies.
# Marked as a system include so warnings from those headers do not
# clutter the project's own warning output.
external_inc = include_directories('external', is_system: true)

pangea_sources = files(
  'src/main.cpp',

  'src/arena/arena.cpp',
  'src/arena/chunk.cpp',

  'src/source/source_buffer.cpp',
  'src/source/string_pool.cpp',
  'src/source/file_io.cpp',

  'src/diagnostics/diagnostics.cpp',
  'src/diagnostics/renderer.cpp',

  'src/driver/cli.cpp',

  'src/lexer/lexer.cpp',
  'src/lexer/token.cpp',
  'src/lexer/dump.cpp',

  'src/lexer/detail/core/cursor.cpp',
  'src/lexer/detail/core/emitter.cpp',
  'src/lexer/detail/core/token_sink.cpp',
  'src/lexer/detail/core/trivia_table.cpp',

  'src/lexer/detail/scanners/scan_word.cpp',
  'src/lexer/detail/scanners/scan_number.cpp',
  'src/lexer/detail/scanners/scan_string.cpp',
  'src/lexer/detail/scanners/scan_symbol.cpp',

  'src/lexer/detail/numeric/numeric.cpp',
  'src/lexer/detail/numeric/numeric_audit.cpp',

  'src/lexer/detail/unicode/unicode_escape.cpp',
)

executable('pangea',
  pangea_sources,
  include_directories: external_inc,
  install: true)
