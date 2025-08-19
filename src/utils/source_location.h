#pragma once

#include <string>

namespace pangea {

struct SourceLocation {
    std::string filename;
    size_t line;
    size_t column;
    size_t offset;

    SourceLocation() : line(1), column(1), offset(0) {}
    SourceLocation(const std::string& file, size_t l, size_t c, size_t o)
        : filename(file), line(l), column(c), offset(o) {}

    std::string toString() const;
};

} // namespace pangea
