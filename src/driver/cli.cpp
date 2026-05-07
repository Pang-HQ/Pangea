#include "cli.h"

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string_view>
#include <utility>

#ifndef _WIN32
# include <unistd.h>
#endif

namespace pangea {

namespace {

constexpr std::string_view COLOR_PREFIX = "--color=";

std::optional<ColourMode> parse_colour_value(std::string_view value) noexcept {
    if (value == "auto") {
        return ColourMode::AUTO;
    }
    if (value == "always") {
        return ColourMode::ALWAYS;
    }
    if (value == "never") {
        return ColourMode::NEVER;
    }
    return std::nullopt;
}

bool env_set_and_nonempty(const char *name) noexcept {
    const char *value = std::getenv(name);
    return value != nullptr && value[0] != '\0';
}

bool stderr_is_tty() noexcept {
#ifdef _WIN32
    return false;
#else
    return ::isatty(fileno(stderr)) != 0;
#endif
}

} // namespace

void print_usage(std::FILE *out, const char *prog) {
    std::fprintf(out,
        "usage: %s [--tokens] [--color=auto|always|never] <file>\n",
        prog);
}

CLIResult parse_cli(int argc, char *argv[]) {
    CLIOptions opts;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(stdout, argv[0]);
            return {std::nullopt, 0};
        }

        if (arg == "--tokens") {
            opts.tokens_only = true;
            continue;
        }

        if (arg.starts_with(COLOR_PREFIX)) {
            const std::string_view value = arg.substr(COLOR_PREFIX.size());
            const std::optional<ColourMode> colour = parse_colour_value(value);
            if (!colour) {
                std::fprintf(stderr,
                    "invalid value for --color: %.*s "
                    "(expected auto, always, or never)\n",
                    static_cast<int>(value.size()), value.data());
                return {std::nullopt, 2};
            }
            opts.colour = *colour;
            continue;
        }

        /*
         Anything else that looks like a flag is a typo or unsupported
         option. Catching both -x and --x here keeps a stray "-foo"
         from being silently treated as the input file.
        */
        if (arg.size() > 1 && arg[0] == '-') {
            std::fprintf(stderr, "unknown option: %.*s\n",
                         static_cast<int>(arg.size()), arg.data());
            print_usage(stderr, argv[0]);
            return {std::nullopt, 2};
        }

        if (!opts.input_file.empty()) {
            std::fprintf(stderr, "more than one input file is not supported\n");
            return {std::nullopt, 2};
        }
        opts.input_file.assign(arg);
    }

    if (opts.input_file.empty()) {
        print_usage(stderr, argv[0]);
        return {std::nullopt, 2};
    }

    return {std::move(opts), 0};
}

bool should_use_colour(ColourMode mode) noexcept {
    /*
     Explicit --color always wins. AUTO defers to the standard
     NO_COLOR / CLICOLOR_FORCE conventions, then falls back to whether
     stderr is a TTY.
    */
    if (mode == ColourMode::ALWAYS) {
        return true;
    }
    if (mode == ColourMode::NEVER) {
        return false;
    }
    if (env_set_and_nonempty("NO_COLOR")) {
        return false;
    }
    if (env_set_and_nonempty("CLICOLOR_FORCE")) {
        return true;
    }
    return stderr_is_tty();
}

} // namespace pangea
