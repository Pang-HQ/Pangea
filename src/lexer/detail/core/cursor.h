#pragma once

/*
 Byte-level cursor over a SourceBuffer.

 Owns no source storage; the buffer must outlive the cursor. The
 SourceBuffer's trailing '\0' sentinel makes peek(0) at end-of-input
 safe without a per-call bounds check.
*/

#include "../../../source/source_buffer.h"
#include "../../../source/source_location.h"

#include <cstddef>
#include <cstdint>

namespace pangea::detail {

class SourceCursor {
public:
    explicit SourceCursor(const SourceBuffer &buf);

    SourceCursor(const SourceCursor &) = delete;
    SourceCursor &operator=(const SourceCursor &) = delete;
    SourceCursor(SourceCursor &&) = delete;
    SourceCursor &operator=(SourceCursor &&) = delete;

    void advance() noexcept;
    [[nodiscard]] SourceOffset offset() const noexcept;
    [[nodiscard]] bool at_end() const noexcept;

    // May read the current byte or look ahead as far as the sentinel.
    [[nodiscard]] char peek(size_t n = 0) const noexcept;

    // c must not be '\0'.
    [[nodiscard]] bool match(char c) noexcept;

private:
    const char *begin_;
    const char *end_;     // points at sentinel
    const char *cursor_;

    void consume_bom_if_present() noexcept;
};

} // namespace pangea::detail
