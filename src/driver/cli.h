#pragma once

/*
 Command-line interface for the Pangea driver. parse_cli walks
 argv, prints usage / errors directly to stderr or stdout as
 appropriate, and returns either the parsed options or an exit
 code for the caller to honour.

 ColourMode is a CLI policy, not a renderer detail; should_use_colour
 resolves AUTO against the runtime environment so the rest of the
 codebase only ever sees a plain bool.
*/

#include <cstdio>
#include <optional>
#include <string>

namespace pangea {

enum class ColourMode {
    AUTO,
    ALWAYS,
    NEVER,
};

struct CLIOptions {
    std::string input_file;
    bool tokens_only = false;
    ColourMode colour = ColourMode::AUTO;
};

/*
 Result of parsing. When options is engaged, parsing succeeded and
 the driver should proceed; when it is empty, the driver should
 return exit_code (0 on --help, 2 on usage error).
*/
struct CLIResult {
    std::optional<CLIOptions> options;
    int exit_code = 0;
};

[[nodiscard]]
CLIResult parse_cli(int argc, char *argv[]);

void print_usage(std::FILE *out, const char *prog);

/*
 Resolve a ColourMode policy against the runtime environment. AUTO
 returns true iff stderr is a TTY.
*/
[[nodiscard]]
bool should_use_colour(ColourMode mode) noexcept;

} // namespace pangea
