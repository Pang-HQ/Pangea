#include "chunk.h"

#include <cstdint>
#include <new>

namespace pangea {

Chunk::Chunk(std::size_t bytes) noexcept {
    assert(bytes != 0);
    assert(bytes <= SIZE_CAP);

    /*
     nothrow keeps the constructor noexcept; allocation failure surfaces
     as is_valid() == false rather than as an exception, so the caller
     (Arena::grow) can take a clean OOM path.

     Note we allocate in this round about way to avoid Wduplicated-branches
     from falsely triggering.
    */
    auto *mem = static_cast<std::byte*>(
        ::operator new[](bytes, std::nothrow)
    );
    begin_.reset(mem);
    if (!begin_) return;

    cur_ = begin_.get();
    end_ = begin_.get() + bytes;
}

std::byte *Chunk::alloc_bytes(
    std::size_t bytes,
    std::size_t align
) noexcept {
    assert(bytes != 0);
    assert(align != 0);

    if (!is_valid())
        return nullptr;

    /*
     Padding to bring cur_ up to the requested alignment. Computed via
     uintptr_t since std::byte * has no defined modulo. The two bounds
     checks below are split so neither size_t subtraction can underflow:
     padding fits in [cur_, end_], then bytes fits in [aligned, end_].
    */
    std::size_t padding = (
        align - (reinterpret_cast<std::uintptr_t>(cur_) % align)
    ) % align;

    if (padding > static_cast<std::size_t>(end_ - cur_))
        return nullptr;

    std::byte *aligned = cur_ + padding;

    if (bytes > static_cast<std::size_t>(end_ - aligned))
        return nullptr;

    cur_ = aligned + bytes;
    return aligned;
}

} // namespace pangea
