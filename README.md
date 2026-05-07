# Pangea

Pangea is a statically typed systems programming language with low-level
control and a focus on modern, ergonomic syntax. The compiler is written in
C++23.

The project is in early development. The frontend is being built piece by
piece; nothing here is usable as a language compiler yet.

## Current state

Only the lexer is implemented. The driver can read a source file, tokenise
it, render diagnostics with source-context snippets, and optionally dump the
token stream. Everything past lexing (parsing, semantic analysis, code
generation) is yet to come.

## Building

We use [Meson](https://mesonbuild.com/) with
[Ninja](https://ninja-build.org/). You can find both on any major package manager.

```
meson setup build
meson compile -C build
```

The compiler binary is written to `build/pangea`. The default build type
is `debugoptimized`, which layers on AddressSanitizer, UBSan, libstdc++
debug iterators, and `_FORTIFY_SOURCE`.

For a release build:

```
meson setup build-release --buildtype=release -Db_lto=true -Db_ndebug=true
meson compile -C build-release
```

The release binary is at `build-release/pangea`.

## Running

```
build/pangea path/to/file.pang              # lex; print diagnostics if any
build/pangea --tokens path/to/file.pang     # also dump the token stream
build/pangea --color=never path/to/file.pang
```

The driver exits non-zero if the lexer reported any errors.

## License

Apache 2.0. See [`LICENSE.md`](LICENSE.md).
