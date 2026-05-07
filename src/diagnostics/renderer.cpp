#include "renderer.h"

#include "../source/source_buffer.h"
#include "diagnostics.h"
#include <algorithm>
#include <cassert>
#include <utility>

namespace pangea {

Renderer::Style Renderer::style_for_level(ErrorLevel level) noexcept {
    switch (level) {
        case ErrorLevel::INFO: return Style::CYAN;
        case ErrorLevel::WARNING: return Style::YELLOW;
        case ErrorLevel::ERROR: return Style::RED;
        case ErrorLevel::FATAL: return Style::RED;
    }
    std::unreachable();
}

const char *Renderer::label_for_level(ErrorLevel level) noexcept {
    switch (level) {
        case ErrorLevel::INFO: return "info";
        case ErrorLevel::WARNING: return "warning";
        case ErrorLevel::ERROR: return "error";
        case ErrorLevel::FATAL: return "fatal";
    }
    std::unreachable();
}

void Renderer::append_colourised(std::string &out,
                                 std::string_view text,
                                 Style style) const {
    if (!use_colour_ || style == Style::NONE) {
        out.append(text);
        return;
    }

    out.append(ansi_code(style));
    out.append(text);
    out.append("\033[0m");
}

void Renderer::append_source_context(std::string &out,
                                     const DiagnosticMessage &diagnostic,
                                     const SourceBuffer &source_buffer) const {
    // Only called for located diagnostics; render() gates on location.
    assert(diagnostic.location.has_value());
    if (!diagnostic.location)
        return;

    const SourceLocation &loc = diagnostic.location.value();

    // Source context is best-effort. The header line is the important bit.
    if (source_buffer.size() == 0) return;

    const auto line_opt = source_buffer.line_text(loc.line);
    if (!line_opt) return;
    const std::string_view line = *line_opt;

    const std::string line_num = std::to_string(loc.line);
    const size_t width = std::max(line_num.size(), size_t(3));
    const Style caret_style = style_for_level(diagnostic.level);

    out.append(width, ' ');  // padding
    append_colourised(out, " |", Style::BLUE);
    out += '\n';

    out.append(width - line_num.size(), ' ');  // padding
    append_colourised(out, line_num + " | ", Style::BLUE);
    out += line;
    out += '\n';

    out.append(width, ' ');  // padding
    append_colourised(out, " | ", Style::BLUE);

    /*
     Mirror tabs from the source line into the caret's leading padding so
     the ^ lands under the offending character. A tab in the source jumps
     to the next terminal tab stop; echoing '\t' here makes the same jump
     on the caret row. Non-tab positions use spaces. Columns past the end
     of the line pad with spaces.
    */
    const size_t caret_col = loc.column - 1;
    for (size_t i = 0; i < caret_col; ++i) {
        const char pad = (i < line.size() && line[i] == '\t') ? '\t' : ' ';
        out.push_back(pad);
    }

    append_colourised(out, "^", caret_style);

    // TODO: handle multi-byte UTF-8 characters
    for (size_t i = 1; i < loc.length; ++i) {
        append_colourised(out, "~", caret_style);
    }

    out += '\n';
}

std::string Renderer::render(const DiagnosticMessage &diagnostic) const {
    /*
     TODO: if private append_rendered(...) is implemented for render_all,
     use the same method in here.
    */
    std::string out;
    out.reserve(DEFAULT_RENDER_RESERVE);

    append_colourised(out, label_for_level(diagnostic.level),
                      style_for_level(diagnostic.level));
    out += ": ";
    out += diagnostic.message;
    out += '\n';

    if (!diagnostic.location.has_value()) {
        return out;
    }

    const SourceLocation &loc = *diagnostic.location;

    if (!loc.filename.empty()) {
        append_colourised(out, "  --> ", Style::BLUE);
        out += loc.filename;
        out += ':';
        out += std::to_string(loc.line);
        out += ':';
        out += std::to_string(loc.column);
        out += '\n';
    }

    const SourceBuffer *buf = registry_.lookup(loc.filename);
    if (buf != nullptr) {
        append_source_context(out, diagnostic, *buf);
    }

    return out;
}

std::string Renderer::render_all(const Diagnostics &diagnostics) const {
    std::string out;

    // Avoid reserving when there is nothing to render.
    if (diagnostics.empty())
        return out;

    out.reserve(diagnostics.size() * DEFAULT_RENDER_RESERVE);

    for (const DiagnosticMessage &d : diagnostics) {
        /*
         TODO: switch to a private append_rendered(
            std::string &out, const DiagnosticMessage &diagnostic) const;
         in order to prevent heap allocations on each render(d) call.

         Note: this affects render(d)'s implementation if implemented.
        */
        out += render(d);
    }
    return out;
}

const char *Renderer::ansi_code(Style style) noexcept {
    switch (style) {
        case Style::RED: return "\033[1;31m";
        case Style::YELLOW: return "\033[1;33m";
        case Style::CYAN: return "\033[1;36m";
        case Style::BLUE: return "\033[1;34m";
        case Style::BOLD: return "\033[1m";
        case Style::NONE: return "";
    }
    std::unreachable();
}

} // namespace pangea
