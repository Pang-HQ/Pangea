#pragma once

/*
 TypedArena<T> is an Arena restricted to one homogeneous element type.
 emplace constructs a T in place; iteration walks every emplaced T in
 insertion order across the underlying chunk chain.

 T must be trivially destructible and have alignment within default
 new alignment - chunk storage comes from operator new, which makes
 no stronger alignment guarantee.

 Like Arena, no destructor runs on stored values; the storage is
 reclaimed in bulk on reset() or destruction.
*/

#include "arena.h"
#include "chunk.h"

#include <bit>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace pangea {

template<typename T>
class TypedArena {
public:
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(std::has_single_bit(alignof(T)));
    static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

    TypedArena(const TypedArena &) = delete;
    TypedArena &operator=(const TypedArena &) = delete;
    TypedArena(TypedArena &&) noexcept = default;
    TypedArena &operator=(TypedArena &&) noexcept = default;

    explicit TypedArena(std::size_t initial_bytes = Arena::INITIAL_SIZE) noexcept
    : arena_(initial_bytes) {}

    // True iff the underlying arena is valid. Forwards to Arena::is_valid().
    [[nodiscard]]
    bool is_valid() const noexcept { return arena_.is_valid(); }

    /*
     Forwards to Arena::reset. Any iterator obtained before this call is
     invalidated, and pointers into freed chunks become dangling.
    */
    void reset(double ratio = 0.0) noexcept { arena_.reset(ratio); }

    [[nodiscard]] std::size_t used() const noexcept { return arena_.used(); }
    [[nodiscard]] std::size_t remaining() const noexcept { return arena_.remaining(); }
    [[nodiscard]] std::size_t total_capacity() const noexcept { return arena_.total_capacity(); }
    [[nodiscard]] std::size_t chunk_count() const noexcept { return arena_.chunk_count(); }

    /*
     Constructs one T in place and returns a pointer to it. Returns
     nullptr on allocation failure with no construction performed.
    */
    template<typename... Args>
    [[nodiscard]] T *emplace(Args&&... args) noexcept {
        return arena_.make<T>(std::forward<Args>(args)...);
    }

    /*
     Forward iterator over every emplaced T in insertion order. Hops
     across chunk boundaries transparently. The arena must outlive the
     iterator; reset() invalidates iterators.
    */
    class iterator;

    iterator begin() const noexcept;
    iterator end() const noexcept;

private:
    Arena arena_;
};

template<typename T>
class TypedArena<T>::iterator {
public:
    iterator() noexcept = default;

    iterator(const Chunk *chunk, const T *ptr) noexcept
    : chunk_(chunk), ptr_(ptr) {}

    const T &operator*() const noexcept { return *ptr_; }
    const T *operator->() const noexcept { return ptr_; }

    iterator &operator++() noexcept {
        ++ptr_;
        /*
         Stay inside the current chunk until the bump cursor is reached,
         then follow next() to the next non-empty chunk. The empty-chunk
         skip is defensive: a chunk can be empty after grow() succeeded
         but the immediate alloc failed.
        */
        const T *chunk_cur = reinterpret_cast<const T *>(chunk_->cur());
        if (ptr_ < chunk_cur)
            return *this;

        chunk_ = chunk_->next();
        while (chunk_ != nullptr && chunk_->begin() == chunk_->cur())
            chunk_ = chunk_->next();

        ptr_ = (chunk_ != nullptr)
                ? reinterpret_cast<const T *>(chunk_->begin())
                : nullptr;
        return *this;
    }

    iterator operator++(int) noexcept {
        iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool operator==(const iterator &other) const noexcept {
        return chunk_ == other.chunk_ && ptr_ == other.ptr_;
    }

    bool operator!=(const iterator &other) const noexcept {
        return !(*this == other);
    }

private:
    const Chunk *chunk_ = nullptr;
    const T *ptr_ = nullptr;
};

template<typename T>
typename TypedArena<T>::iterator TypedArena<T>::begin() const noexcept {
    const Chunk *first = arena_.root_chunk();
    while (first != nullptr && first->begin() == first->cur())
        first = first->next();

    if (first == nullptr)
        return iterator{};

    return iterator(first, reinterpret_cast<const T *>(first->begin()));
}

template<typename T>
typename TypedArena<T>::iterator TypedArena<T>::end() const noexcept {
    return iterator{};
}

} // namespace pangea
