#pragma once

#include "diagnostics.h"
#include "../source/source_buffer.h"

#include <cstddef>
#include <string>

namespace pangea {

/*
 Renderer turns diagnostics into human-readable strings. It looks up
 source context through the attached registry and formats the header,
 location line, source snippet, and caret.

 Pure: each call returns a freshly-built std::string. The driver decides
 where the string goes (stderr, log file, test snapshot, LSP field).

 Colour mode is set at construction - it is a static policy for the
 process, not something callers need to toggle per-diagnostic.
*/
class Renderer {
public:
    Renderer(const SourceBufferRegistry &registry, bool use_colour)
        : registry_(registry),
          use_colour_(use_colour) {}

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;

    std::string render(const DiagnosticMessage &diagnostic) const;
    std::string render_all(const Diagnostics &diagnostics) const;

private:
    static constexpr size_t DEFAULT_RENDER_RESERVE = 256;

    const SourceBufferRegistry &registry_;
    const bool use_colour_;

    enum class Style {
        NONE,
        RED,
        YELLOW,
        CYAN,
        BLUE,
        BOLD
    };

    static Style style_for_level(ErrorLevel level) noexcept;
    static const char *label_for_level(ErrorLevel level) noexcept;
    static const char *ansi_code(Style style) noexcept;

    void append_colourised(std::string &out,
                           std::string_view text,
                           Style style) const;
    void append_source_context(std::string &out,
                               const DiagnosticMessage &diagnostic,
                               const SourceBuffer &source_buffer) const;
};

} // namespace pangea
