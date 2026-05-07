#pragma once

/*
 Chunk is a single fixed-size buffer of raw bytes that bump-allocates
 within itself. It is the storage primitive used by Arena.

 Chunks are non-copyable and non-movable; once linked into an arena's
 chain they stay at a fixed address, so iterators and other code can
 hold raw Chunk pointers across allocations.

 Chunks do not run destructors on stored values. Anything placed inside
 a chunk via alloc_bytes must be trivially destructible.
*/

#include <cassert>
#include <cstddef>
#include <memory>

namespace pangea {

class Arena;

class Chunk {
public:
    // Hard cap on individual chunk size; bounds geometric growth in Arena.
    static constexpr std::size_t SIZE_CAP = 128ULL * 1024 * 1024;

    Chunk(const Chunk &) = delete;
    Chunk &operator=(const Chunk &) = delete;
    Chunk(Chunk &&) = delete;
    Chunk &operator=(Chunk &&) = delete;

    /*
     Allocates a buffer of bytes via nothrow new. On allocation failure
     the chunk is left invalid (is_valid() returns false) and never
     throws. Preconditions: bytes != 0 and bytes <= SIZE_CAP.
    */
    explicit Chunk(std::size_t bytes) noexcept;

    // True iff the underlying buffer was allocated successfully.
    [[nodiscard]]
    bool is_valid() const noexcept { return begin_ != nullptr; }

    // Rewinds the bump cursor without destroying anything. Pointers
    // previously returned from alloc_bytes are invalidated. Precondition:
    // is_valid().
    void reset() noexcept {
        assert(is_valid());
        cur_ = begin_.get();
    }

    // Bytes already handed out by alloc_bytes. Precondition: is_valid().
    std::size_t used() const noexcept {
        assert(is_valid());
        return static_cast<std::size_t>(cur_ - begin_.get());
    }

    // Bytes still available in this chunk. Precondition: is_valid().
    std::size_t remaining() const noexcept {
        assert(is_valid());
        return static_cast<std::size_t>(end_ - cur_);
    }

    // Total buffer size. Precondition: is_valid().
    std::size_t capacity() const noexcept {
        assert(is_valid());
        return static_cast<std::size_t>(end_ - begin_.get());
    }

    // Buffer endpoints. Stable for the chunk's lifetime.
    const std::byte *begin() const noexcept { return begin_.get(); }
    const std::byte *cur() const noexcept { return cur_; }
    const std::byte *end() const noexcept { return end_; }

    /*
     Bump-allocates bytes with the requested alignment. Returns nullptr
     if the chunk has no room after alignment padding; never throws.
     Preconditions: bytes != 0, align is a non-zero power of two.
    */
    std::byte *alloc_bytes(std::size_t bytes, std::size_t align) noexcept;

    // Forward link in the arena's chain; nullptr at the tail.
    const Chunk *next() const noexcept { return next_.get(); }

    friend class Arena;

private:
    std::unique_ptr<std::byte[]> begin_;
    std::byte *cur_ = nullptr;
    std::byte *end_ = nullptr;
    /*
     Owns the successor chunk via RAII. Destroying head frees the entire
     chain; releasing next_ on a chunk frees the sub-chain past it.
    */
    std::unique_ptr<Chunk> next_;
};

} // namespace pangea
