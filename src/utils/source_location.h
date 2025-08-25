#pragma once

#include <string>

namespace pangea {

struct SourceLocation {
    std::string filename;
    size_t line;
    size_t column;
    size_t offset;
    size_t length;

    SourceLocation() : line(1), column(1), offset(0), length(0) {}
    SourceLocation(const std::string& file, size_t l, size_t c, size_t o, size_t len = 1)
        : filename(file), line(l), column(c), offset(o), length(len) {}

    std::string toString() const;
};

} // namespace pangea
