#pragma once

/*
 SourceBuffer owns the bytes of one source file and a precomputed
 map from byte offset to (line, column).

 It is the shared source of truth for resolving offsets in user-facing
 diagnostics and is kept separate from the lexer so the parser and any
 future incremental machinery can reuse it.

 Design notes:

 - bytes_ always has one trailing '\0' sentinel beyond the real source
   bytes. Scanner hot loops may read *cursor unconditionally without a
   per-byte bounds check. size() excludes the sentinel.

 - line_offsets_ is built once in a single linear pass. Resolving an
   offset to a line and column is O(log L) via binary search and only
   happens on diagnostic paths.

 - SourceBuffer is non-copyable and non-movable because it hands out
   std::string_view objects and raw pointers into bytes_. Moving may
   invalidate those views silently.
*/

#include "source_location.h"

#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pangea {

class SourceBuffer {
public:
    /*
     One value is reserved so the line_offsets_ sentinel at
     real_size_ + 1 still fits in SourceOffset.
    */
    static constexpr std::size_t MAX_BYTES =
        static_cast<std::size_t>(std::numeric_limits<SourceOffset>::max()) - 1;

    /*
     Takes ownership of src.

     A trailing '\0' sentinel is appended beyond the real bytes so the
     scanner can read through end-of-input without a separate bounds
     check.

     If src exceeds MAX_BYTES, it is truncated and oversized() reports
     that truncation to the caller.
    */
    SourceBuffer(std::string src, std::string filename);

    SourceBuffer(const SourceBuffer &) = delete;
    SourceBuffer &operator=(const SourceBuffer &) = delete;
    SourceBuffer(SourceBuffer &&) = delete;
    SourceBuffer &operator=(SourceBuffer &&) = delete;

    /*
     Pointer to the first real byte. Storage is contiguous for exactly
     size() + 1 bytes, with the final byte being the sentinel.
    */
    const char *data() const noexcept;

    // Number of real source bytes, excluding the sentinel.
    std::size_t size() const noexcept;

    const std::string &filename() const noexcept;

    /*
     Returns a read-only view of [offset, offset + length). The range must
     lie entirely within the real bytes of the buffer.
    */
    std::string_view slice(SourceRange range) const noexcept;
    std::string_view slice(SourceOffset off, SourceLength len) const noexcept;

    /*
     Resolves a byte range to a SourceLocation for diagnostics.
     The range must lie entirely within the real bytes of the buffer;
     violating that precondition aborts the process.
    */
    [[nodiscard]]
    SourceLocation location_of(SourceRange range) const;

    /*
     Returns the text of the given 1-based line, with line terminators
     (\n, \r\n, or \r) stripped. An empty view inside the optional is a
     valid blank line; std::nullopt means the line is 0 or past the last
     line of the source.
    */
    std::optional<std::string_view> line_text(SourceLine line) const noexcept;

    // True if the constructor had to truncate an oversized source file.
    bool oversized() const noexcept;

private:
    /*
     Owned source bytes plus one trailing '\0' sentinel. real_size_ stores
     the length of the real source bytes only.
    */
    std::string bytes_;
    std::size_t real_size_;
    std::string filename_;
    bool oversized_ = false;

    /*
     line_offsets_[i] is the byte offset of the first character of source
     line (i + 1). line_offsets_[0] is always 0.

     A trailing sentinel equal to real_size_ + 1 ensures upper_bound on
     any offset in [0, real_size_] always finds a strictly greater entry,
     including EOF.
    */
    std::vector<SourceOffset> line_offsets_;

    void build_line_offsets();
};

/*
 Owns every SourceBuffer created during one compilation. The driver
 loads files through add(), other stages read them through lookup().

 Each filename may be added exactly once; a duplicate add() aborts.
 Non-owning pointers returned from add()/lookup() stay valid for the
 registry's lifetime.
*/
class SourceBufferRegistry {
public:
    SourceBufferRegistry();

    SourceBufferRegistry(const SourceBufferRegistry &) = delete;
    SourceBufferRegistry &operator=(const SourceBufferRegistry &) = delete;
    SourceBufferRegistry(SourceBufferRegistry &&) = delete;
    SourceBufferRegistry &operator=(SourceBufferRegistry &&) = delete;

    /*
     Constructs and stores a SourceBuffer, returning a stable reference
     to it. Aborts if filename has already been registered.
    */
    const SourceBuffer &add(std::string bytes, std::string filename);

    /*
     Non-owning pointer to the buffer registered under filename, or
     nullptr if none exists.
    */
    const SourceBuffer *lookup(std::string_view filename) const;

    std::size_t size() const noexcept;

private:
    std::vector<std::unique_ptr<SourceBuffer>> buffers_;
    std::unordered_map<std::string_view, const SourceBuffer *> by_filename_;
};

} // namespace pangea