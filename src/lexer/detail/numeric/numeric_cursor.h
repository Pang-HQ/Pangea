#pragma once

#include <cassert>
#include <cstddef>
#include <string_view>

namespace pangea::detail {

struct NumericCursor {
    const std::string_view text;
    std::size_t pos = 0;

    [[nodiscard]]
    char peek(std::size_t n = 0) const noexcept {
        // Avoids potential overflow from pos + n.
        if (pos >= text.size()) return '\0';
        if (n >= text.size() - pos) return '\0';

        return text[pos + n];
    }

    [[nodiscard]]
    bool at_end() const noexcept {
        return pos >= text.size();
    }

    void advance() noexcept {
        assert(!at_end());
        ++pos;
    }
};

} // namespace pangea::detail
