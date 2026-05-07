#pragma once

/*
 Arena is a chunked bump allocator. It owns a forward-linked chain of
 Chunks, allocates from the tail, and grows by appending new chunks
 with geometrically increasing capacity (capped at Chunk::SIZE_CAP)
 until the chain reaches MAX_CHUNKS, after which allocation fails.

 Allocated storage is reclaimed only via reset() or destruction; per-
 element destructors are never run, so types passed to alloc<T> and
 make<T> must be trivially destructible.

 Arena allocations belong to an allocation epoch. Pointers returned
 from alloc/make remain valid until reset() or destruction ends that
 epoch.

 reset(0) keeps every chunk and only rewinds the bump cursors, so
 existing storage is reused. reset(ratio > 0) may additionally release
 tail chunks to reduce retained capacity before the next allocation
 epoch.

 All pointers obtained from the arena before reset become invalid
 after reset, regardless of whether the underlying storage is
 physically retained.

 Not thread-safe.
*/

#include "chunk.h"

#include <bit>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace pangea {

class ArenaResource;

class Arena {
public:
    // Default size of the first chunk; subsequent chunks double until SIZE_CAP.
    static constexpr std::size_t INITIAL_SIZE = 16 * 1024;

    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;
    Arena(Arena &&) noexcept = default;
    Arena &operator=(Arena &&) noexcept = default;

    /*
     Allocates the first chunk eagerly. On allocation failure the arena
     is left invalid; subsequent alloc calls retry growth lazily.
     initial_bytes is silently clamped to Chunk::SIZE_CAP.
    */
    explicit Arena(std::size_t initial_bytes = INITIAL_SIZE) noexcept;

    // True iff the arena has at least one valid chunk.
    [[nodiscard]]
    bool is_valid() const noexcept {
        return chunk_count_ > 0 && chunks_[0] != nullptr && chunks_[0]->is_valid();
    }

    /*
     Rewinds bump cursors and optionally releases capacity.

     ratio is a hint in [0, 1] of the approximate fraction of current capacity
     the caller expects not to reuse in the next allocation epoch. 0 means
     free nothing, 1 means free everything. Intermediate values are hints
     to shrink capacity where possible to avoid taking up unnecessary space.

     All pointers obtained from the arena before reset are invalid after
     reset, regardless of whether underlying storage is retained.
    */
    void reset(double ratio = 0.0) noexcept;

    // Bytes currently handed out across all chunks.
    [[nodiscard]] std::size_t used() const noexcept;
    // Bytes still available in the tail chunk only.
    [[nodiscard]] std::size_t remaining() const noexcept;
    // Sum of every chunk's capacity.
    [[nodiscard]] std::size_t total_capacity() const noexcept { return total_capacity_; }
    // Number of chunks currently in the chain.
    [[nodiscard]] std::size_t chunk_count() const noexcept { return chunk_count_; }

    /*
     Allocates uninitialised storage for n contiguous T. Returns nullptr
     if the arena cannot fit the request. T must be trivially destructible
     and have a power-of-two alignment within default new alignment.
    */
    template<typename T>
    [[nodiscard]] T *alloc(std::size_t n = 1) noexcept;

    /*
     Allocates one T and constructs it in place. Returns nullptr on
     allocation failure with no construction performed. T's constructor
     for the given Args must be noexcept.
    */
    template<typename T, typename... Args>
    [[nodiscard]] T *make(Args&&... args) noexcept;

    // Head of the chunk chain, or nullptr if the arena is invalid.
    const Chunk *root_chunk() const noexcept { return head_.get(); }

    /*
     ArenaResource is the std::pmr::memory_resource adapter that lets
     PMR containers allocate from an Arena. It needs runtime alignment
     allocation that the type-keyed alloc<T> cannot express, so it is
     friended in to call alloc_bytes directly.
    */
    friend class ArenaResource;

private:
    /*
     Hard ceiling on chunk count. Combined with geometric growth capped
     at Chunk::SIZE_CAP, bounds total arena memory at roughly
     MAX_CHUNKS * Chunk::SIZE_CAP. Allocations beyond this point fail.
    */
    static constexpr std::size_t MAX_CHUNKS = 24;

    std::byte *alloc_bytes(std::size_t bytes, std::size_t align) noexcept;
    bool grow(std::size_t min_bytes) noexcept;
    void rewind() noexcept;
    void reinitialise() noexcept;

    // Returns true iff the arena was fully reinitialised, false otherwise.
    bool free_top_k(std::size_t k) noexcept;

    std::size_t chunks_to_free_for(std::size_t target_bytes) const noexcept;
    /*
     reset(ratio) refuses to free a chunk whose capacity exceeds this
     multiple of the remaining target. Prevents tiny ratios from
     releasing huge chunks and overshooting the hint by orders of
     magnitude.
    */
    static constexpr std::size_t MIN_TARGET_TO_CHUNK_RATIO = 4;

    /*
     head_ owns the chain via RAII. chunks_[] is a non-owning index of
     each chunk's address, mirroring the linked structure for O(1) tail
     and indexed access. The linked structure is authoritative.
    */
    std::unique_ptr<Chunk> head_;
    Chunk *chunks_[MAX_CHUNKS] = {};

    std::size_t chunk_count_ = 0;
    std::size_t total_capacity_ = 0;
    // Index of the first chunk allocated at SIZE_CAP; MAX_CHUNKS if none.
    std::size_t first_capped_chunk_ = MAX_CHUNKS;
    // Initial chunk size from the constructor; restored on reinitialise.
    std::size_t initial_size_ = 0;
    // Size to use for the next grow(); doubles after each grow until SIZE_CAP.
    std::size_t next_chunk_size_ = 0;
};

template<typename T>
T *Arena::alloc(std::size_t n) noexcept {
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(std::has_single_bit(alignof(T)));
    assert(n != 0);

    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        return nullptr;

    return reinterpret_cast<T *>(
        alloc_bytes(sizeof(T) * n, alignof(T))
    );
}

template<typename T, typename... Args>
T *Arena::make(Args&&... args) noexcept {
    static_assert(std::is_nothrow_constructible_v<T, Args...>);
    T *block = alloc<T>();
    if (block == nullptr)
        return nullptr;

    return new (block) T(std::forward<Args>(args)...);
}

} // namespace pangea
