#include "cursor.h"

#include <cassert>
#include <cstddef>
#include <limits>

namespace pangea::detail {

SourceCursor::SourceCursor(const SourceBuffer &buf)
    : begin_(buf.data()),
      end_(buf.data() + buf.size()),
      cursor_(buf.data()) {
    /*
     peek() reads cursor_[n] without a bounds check, so the buffer's
     trailing '\0' sentinel is load-bearing. Catch a missing sentinel
     here rather than at the first peek that walks off the end.
    */
    assert(end_[0] == '\0' &&
           "SourceBuffer sentinel missing - peek() is unsafe");

    consume_bom_if_present();
}

char SourceCursor::peek(std::size_t n) const noexcept {
    assert(n <= static_cast<std::size_t>(end_ - cursor_));
    return cursor_[n];
}

void SourceCursor::advance() noexcept {
    assert(cursor_ < end_ && "advance() past end of source");
    ++cursor_;
}

bool SourceCursor::match(char c) noexcept {
    /*
     Matching '\0' would silently succeed at EOF (the sentinel) and
     push cursor_ past end_, breaking every later assertion.
    */
    assert(c != '\0' && "match('\\0') is never valid");
    if (*cursor_ != c) {
        return false;
    }
    ++cursor_;
    return true;
}

SourceOffset SourceCursor::offset() const noexcept {
    const std::ptrdiff_t diff = cursor_ - begin_;

    assert(diff >= 0);
    assert(diff <= end_ - begin_);
    assert(diff <= static_cast<std::ptrdiff_t>(
        std::numeric_limits<SourceOffset>::max()));

    return static_cast<SourceOffset>(diff);
}

bool SourceCursor::at_end() const noexcept {
    return cursor_ >= end_;
}

void SourceCursor::consume_bom_if_present() noexcept {
    if (static_cast<std::size_t>(end_ - cursor_) < 3) {
        return;
    }

    if (static_cast<unsigned char>(cursor_[0]) == 0xEF &&
        static_cast<unsigned char>(cursor_[1]) == 0xBB &&
        static_cast<unsigned char>(cursor_[2]) == 0xBF) {
        cursor_ += 3;
    }
}

} // namespace pangea::detail
