#pragma once

/*
 ArenaResource adapts an Arena to std::pmr::memory_resource so PMR
 containers can be backed by an arena. Allocations bump from the
 referenced Arena; deallocate is a no-op since arenas free in bulk
 only on reset() or destruction.

 The adapter holds a non-owning reference to the Arena. The Arena
 must outlive both the adapter and any container using it.

 ArenaResource is non-copyable and non-movable on purpose: PMR
 containers store a raw memory_resource* that survives container
 moves, so the resource's address must stay stable for the lifetime
 of every container that uses it. Callers that need movability
 should hold the resource through a stable indirection
 (e.g. std::unique_ptr).
*/

#include "arena.h"

#include <cstddef>
#include <memory_resource>

namespace pangea {

class ArenaResource final : public std::pmr::memory_resource {
public:
    explicit ArenaResource(Arena &arena) noexcept : arena_(arena) {}

    ArenaResource(const ArenaResource &) = delete;
    ArenaResource &operator=(const ArenaResource &) = delete;
    ArenaResource(ArenaResource &&) = delete;
    ArenaResource &operator=(ArenaResource &&) = delete;

private:
    void *do_allocate(std::size_t bytes, std::size_t align) override {
        return arena_.alloc_bytes(bytes, align);
    }

    /*
     Deallocate is a no-op: memory is not individually reclaimed.
     All allocations are released when the Arena is reset or destroyed.
    */
    void do_deallocate(void *, std::size_t, std::size_t) override {}

    /*
     Resources are equal iff they are the same instance. Memory
     allocated through one resource cannot be freed through another,
     so identity comparison is the safe answer for an adapter that
     defers to a specific arena.
    */
    bool do_is_equal(
        const std::pmr::memory_resource &other
    ) const noexcept override {
        return this == &other;
    }

    Arena &arena_;
};

} // namespace pangea
