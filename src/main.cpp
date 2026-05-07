/*
 Driver entry point. Wires the long-lived state - SourceBufferRegistry,
 StringPool, LexerOutput, Diagnostics - and runs the lexer over a
 single input file. Each piece of policy lives in its own module:
 CLI parsing in driver/cli, file I/O in source/file_io, token
 dumping in lexer/dump, diagnostic rendering in diagnostics/renderer.
*/

#include "diagnostics/diagnostics.h"
#include "diagnostics/renderer.h"
#include "driver/cli.h"
#include "lexer/dump.h"
#include "lexer/lexer.h"
#include "lexer/output.h"
#include "source/file_io.h"
#include "source/source_buffer.h"
#include "source/string_pool.h"

#include <cstdio>
#include <utility>

int main(int argc, char *argv[]) {
    using namespace pangea;

    CLIResult cli = parse_cli(argc, argv);
    if (!cli.options) {
        return cli.exit_code;
    }
    CLIOptions &opts = *cli.options;

    std::optional<std::string> source = read_file(opts.input_file);
    if (!source) {
        std::fprintf(stderr, "could not read %s\n", opts.input_file.c_str());
        return 1;
    }

    SourceBufferRegistry registry;
    const SourceBuffer &buf = registry.add(std::move(*source),
                                           std::move(opts.input_file));

    StringPool strings;
    LexerOutput out(buf.size());
    Diagnostics diag;

    Lexer lex(buf, out, strings, &diag);
    lex.tokenise();

    /*
     Render diagnostics first so the user sees them before any token
     dump scrolls past. Hard errors (ERROR, FATAL) abort before the
     dump; warnings let the dump proceed.
    */
    if (!diag.empty()) {
        const Renderer renderer(registry, should_use_colour(opts.colour));
        std::fputs(renderer.render_all(diag).c_str(), stderr);
    }

    if (diag.error_count() > 0) {
        return 1;
    }

    if (opts.tokens_only) {
        dump_tokens(stdout, out, strings);
    }

    return 0;
}
