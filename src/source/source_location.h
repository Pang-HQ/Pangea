#pragma once

#include "../core/warnings.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>

namespace pangea {

// Width aliases for source positions. These stay compact on purpose.
// Changing them updates SourceLocation and SourceRange in one place.
using SourceOffset = std::uint32_t;
using SourceLength = std::uint32_t;
using SourceLine = std::uint32_t;
using SourceColumn = std::uint32_t;

/*
 Half-open [offset, offset + length) byte range into a SourceBuffer.
 Carries no back-pointer; the buffer is passed explicitly when a range
 needs to be resolved against actual bytes.
*/
struct SourceRange {
    const SourceOffset offset;
    const SourceLength length;

    constexpr SourceOffset end() const noexcept {
        return offset + length;
    }

    constexpr bool empty() const noexcept {
        return length == 0;
    }
};

/*
 Diagnostic-facing location in a source file. Owns its filename so diagnostics
 remain valid after the source buffer is destroyed, which makes it heavier
 than SourceRange. Hot paths should use SourceRange and only convert via
 SourceBuffer::location_of when reporting a diagnostic.

 SourceLocation is immutable; all fields are const:
 "The location of a token won't magically change half-way through a compile."
*/
struct SourceLocation {
    // Owned so it outlives the SourceBuffer.
    const std::string filename;

    // 1-based (0 is invalid).
    const SourceLine line;
    const SourceColumn column;

    // 0-based byte offset; may equal source.size() (EOF).
    const SourceOffset offset;

    // Byte length (not display width); may be 0 for point locations.
    // Together with offset, forms a half-open range [offset, offset+length).
    const SourceLength length;

    DISABLE_WSHADOW [[nodiscard]]
    SourceLocation(std::string_view filename,
                   SourceLine line,
                   SourceColumn column,
                   SourceOffset offset,
                   SourceLength length = 1)
        : filename(filename),
          line(line),
          column(column),
          offset(offset),
          length(length) {
        assert(line > 0);
        assert(column > 0);
    }
    ENABLE_WSHADOW
};

} // namespace pangea