#include "source_buffer.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace pangea {

SourceBuffer::SourceBuffer(std::string src, std::string filename)
    : bytes_(std::move(src)),
      real_size_(bytes_.size()),
      filename_(std::move(filename)) {
    /*
     Clip anything past MAX_BYTES and flag it. We do this by
     resizing bytes_ rather than rejecting the input outright so
     that the rest of the compiler still has *something* to look
     at - a 4 GiB source is a user error, not a reason to crash,
     and a clipped buffer lexes cleanly as long as the clip
     point happens to be inside valid UTF-8 (which is best-effort
     - a truncated multi-byte sequence at the edge will surface
     as a normal LEX_ERROR from the garbage-byte path).
    */
    if (real_size_ > MAX_BYTES) {
        bytes_.resize(MAX_BYTES);
        real_size_ = MAX_BYTES;
        oversized_ = true;
    }

    /*
     Append the sentinel unconditionally. We do NOT check whether
     the input already ends in '\0' - the scanner's invariant is
     "bytes_[real_size_] is the sentinel we placed, and it is
     reachable even if real_size_ == 0". Conditional appending
     would muddle that invariant and save nothing meaningful.
    */
    bytes_.push_back('\0');
    build_line_offsets();

    assert(bytes_.size() == real_size_ + 1);
    assert(bytes_[real_size_] == '\0');
    assert(line_offsets_.size() >= 2);
    assert(line_offsets_.front() == 0);
    /*
     The trailing sentinel in line_offsets_ is real_size_ + 1
     rather than real_size_ so that upper_bound always finds a
     strict upper bound for any valid byte offset, including
     offset == real_size_ (which is how an EOF token reports
     its position).
    */
    assert(line_offsets_.back() == static_cast<SourceOffset>(real_size_) + 1);
}

const char *SourceBuffer::data() const noexcept {
    return bytes_.data();
}

size_t SourceBuffer::size() const noexcept {
    return real_size_;
}

const std::string &SourceBuffer::filename() const noexcept {
    return filename_;
}

bool SourceBuffer::oversized() const noexcept {
    return oversized_;
}

std::string_view SourceBuffer::slice(SourceRange range) const noexcept {
    return slice(range.offset, range.length);
}

std::string_view SourceBuffer::slice(SourceOffset off, SourceLength len) const noexcept {
    /*
     Empty slices at EOF are valid, so off == real_size_ is allowed
     when len == 0. Anything past that is a caller bug; abort in both
     debug and release to match location_of. A junk view returned
     silently would be worse than a crash.
    */
    const size_t off_sz = static_cast<size_t>(off);
    const size_t len_sz = static_cast<size_t>(len);
    assert(off_sz <= real_size_);
    assert(len_sz <= real_size_ - off_sz);
    if (off_sz > real_size_ || len_sz > real_size_ - off_sz) {
        std::abort();
    }
    return std::string_view(bytes_.data() + off, len);
}

SourceLocation SourceBuffer::location_of(SourceRange range) const {
    /*
     offset == real_size_ is the EOF position and is valid; anything past
     that is a programming error we refuse to paper over. A silent clamp
     would turn a caller bug into a mislabeled diagnostic, so we abort
     instead. Debug builds trip the assert first for a better trace.
    */
    assert(static_cast<size_t>(range.offset) <= real_size_);
    assert(range.end() <= real_size_);
    if (static_cast<size_t>(range.offset) > real_size_ ||
        range.end() > real_size_) {
        std::abort();
    }

    /*
     `upper_bound` returns the first line start strictly greater than the
     offset. The owning line is the one just before that, so we step back
     by one. The trailing sentinel guarantees upper_bound finds a strictly
     greater element, especially at EOF. The decrement is safe because
     line_offsets_.front() is always 0.
    */
    auto it = std::upper_bound(line_offsets_.begin(), line_offsets_.end(),
                               range.offset);
    assert(it != line_offsets_.begin());
    --it;

    const SourceLine line_index = static_cast<SourceLine>(it - line_offsets_.begin());
    const SourceOffset line_start = *it;

    const SourceLine line = line_index + 1;
    const SourceColumn column = (range.offset - line_start) + 1;

    return SourceLocation(filename_, line, column, range.offset, range.length);
}

std::optional<std::string_view>
SourceBuffer::line_text(SourceLine line) const noexcept {
    if (line == 0) return std::nullopt;

    /*
     line_offsets_ has one entry per line start plus a trailing sentinel
     at real_size_ + 1, so the last valid line index is size() - 2. The
     last entry is the sentinel and does not correspond to any real line.
    */
    const size_t line_idx = static_cast<size_t>(line) - 1;
    if (line_idx + 1 >= line_offsets_.size()) return std::nullopt;

    const size_t line_start = line_offsets_[line_idx];
    size_t line_end = line_offsets_[line_idx + 1];

    /*
     The next entry is either the start of the following line (one past
     a newline) or the trailing sentinel real_size_ + 1. Convert the
     sentinel back to real_size_, then strip a trailing \n, \r\n, or
     \r so callers get just the line text.
    */
    if (line_end == real_size_ + 1) {
        line_end = real_size_;
    }
    assert(line_end <= real_size_);

    if (line_end > line_start && bytes_[line_end - 1] == '\n') --line_end;
    if (line_end > line_start && bytes_[line_end - 1] == '\r') --line_end;

    return std::string_view(bytes_.data() + line_start, line_end - line_start);
}

void SourceBuffer::build_line_offsets() {
    /*
     Single linear pass. We reserve optimistically at one entry
     per ~40 bytes, which is close to the average line length
     for C-like source. Being wrong only costs a handful of
     reallocations during construction and is amortised over
     the lifetime of the buffer.
    */
    line_offsets_.reserve((real_size_ / 40) + 2);

    // Line 1 always starts at offset 0, even for empty files.
    line_offsets_.push_back(0);

    for (size_t i = 0; i < real_size_; ++i) {
        if (bytes_[i] == '\n') {
            line_offsets_.push_back(static_cast<SourceOffset>(i + 1));
            continue;
        }

        if (bytes_[i] == '\r') {
            // \r\n is one line ending, not two.
            if (i + 1 < real_size_ && bytes_[i + 1] == '\n') {
                ++i;
            }
            line_offsets_.push_back(static_cast<SourceOffset>(i + 1));
        }
    }

    /*
     Trailing sentinel. We use (real_size_ + 1) rather than
     real_size_ so that upper_bound called with any valid
     offset (including real_size_ itself) always returns a
     strictly-greater element, keeping the decrement safe
     without a special case. The sentinel is never returned
     as a "real" line start because location_of always steps
     one entry back before reading.
    */
    line_offsets_.push_back(static_cast<SourceOffset>(real_size_) + 1);
}

SourceBufferRegistry::SourceBufferRegistry() {
    /*
     Pre-size for a typical large-project compile. A capacity of 1024
     entries costs ~8 KB of pointer backing and covers every realistic
     case without ever regrowing - vector regrowth would copy the owning
     unique_ptrs but leave the heap-allocated SourceBuffers put, so it
     is correct even without this reserve; the reserve just avoids the
     churn.
    */
    buffers_.reserve(1024);
    by_filename_.reserve(1024);
}

const SourceBuffer &SourceBufferRegistry::add(std::string bytes,
                                              std::string filename) {
    /*
     "Every file gets read exactly once" is a hard invariant of the
     registry - duplicate loads would waste memory and let callers
     diverge on which copy they consult. Enforced by abort in both
     debug and release so the bug surfaces at the first bad add()
     rather than as a confusing diagnostic much later.
    */
    auto existing = by_filename_.find(filename);
    assert(existing == by_filename_.end() &&
           "SourceBufferRegistry: filename added twice");
    if (existing != by_filename_.end()) {
        std::fprintf(stderr, "SourceBufferRegistry: "
            "filename \"%s\" added multiple times, aborting.\n",
            filename.c_str());
        std::abort();
    }

    /*
     Pre-reserve capacity so allocation failure happens before mutating the
     registry. This avoids leaving buffers_ and by_filename_ out of sync.
    */
    buffers_.reserve(buffers_.size() + 1);
    by_filename_.reserve(by_filename_.size() + 1);

    auto owned = std::make_unique<SourceBuffer>(
        std::move(bytes), std::move(filename));

    /*
     Commit ownership before publishing non-owning views/pointers. If vector
     growth throws, no map entry points into the uncommitted buffer.
    */
    buffers_.push_back(std::move(owned));
    const SourceBuffer *raw = buffers_.back().get();

    const auto inserted =
        by_filename_.emplace(std::string_view(raw->filename()), raw);
    assert(inserted.second && "SourceBufferRegistry: filename map insert failed");
    if (!inserted.second) {
        std::fprintf(stderr,
            "SourceBufferRegistry: filename map insert failed, aborting.\n");
        std::abort();
    }

    return *raw;
}

const SourceBuffer *
SourceBufferRegistry::lookup(std::string_view filename) const {
    auto it = by_filename_.find(filename);
    if (it == by_filename_.end()) return nullptr;
    return it->second;
}

size_t SourceBufferRegistry::size() const noexcept {
    return buffers_.size();
}

} // namespace pangea
