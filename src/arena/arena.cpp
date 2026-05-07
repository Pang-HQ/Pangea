#include "arena.h"

#include <cassert>
#include <cstddef>
#include <new>
#include <memory>
#include <utility>

namespace pangea {

Arena::Arena(std::size_t initial_bytes) noexcept {
    /*
     Clamp at SIZE_CAP rather than rejecting oversized requests: callers
     who ask for "as big as possible" up front shouldn't be surprised by
     a silent invalidation if they cross the cap.
    */
    next_chunk_size_ = initial_bytes < Chunk::SIZE_CAP
                       ? initial_bytes
                       : Chunk::SIZE_CAP;
    if (next_chunk_size_ == 0)
        return;

    initial_size_ = next_chunk_size_;
    grow(0);
}

void Arena::reset(double ratio) noexcept {
    /*
     Caller contract is [0, 1]. The dispatch tolerates the boundaries
     explicitly so the asserts are purely defensive against junk input.
    */
    assert(ratio >= 0.0);
    assert(ratio <= 1.0);

    if (ratio <= 0.0) {
        rewind();
        return;
    }
    if (ratio >= 1.0) {
        reinitialise();
        return;
    }

    std::size_t target = static_cast<std::size_t>(
        ratio * static_cast<double>(total_capacity_)
    );
    std::size_t k = chunks_to_free_for(target);
    /*
     Reinitialise already produces a fresh empty chunk; rewinding it
     would touch the same chunk twice for no effect.
    */
    if (!free_top_k(k))
        rewind();
}

std::size_t Arena::used() const noexcept {
    std::size_t total = 0;
    for (std::size_t i = 0; i < chunk_count_; ++i)
        total += chunks_[i]->used();
    return total;
}

std::size_t Arena::remaining() const noexcept {
    if (chunk_count_ == 0)
        return 0;
    return chunks_[chunk_count_ - 1]->remaining();
}

std::byte *Arena::alloc_bytes(
    std::size_t bytes,
    std::size_t align
) noexcept {
    assert(bytes != 0);
    assert(align != 0);

    /*
     A new chunk's begin_ is aligned to default new alignment, but the
     bump cursor inside an existing chunk is not necessarily aligned to
     `align`. Reserving enough headroom for the worst-case padding
     guarantees a fresh chunk can satisfy the request.
    */
    std::size_t needed = bytes + align - 1;

    /*
     Lazy first-chunk allocation supports arenas that lost their initial
     chunk (constructor OOM, or reinitialise with a failing grow).
    */
    if (chunk_count_ == 0) {
        if (!grow(needed))
            return nullptr;
    }

    std::byte *p = chunks_[chunk_count_ - 1]->alloc_bytes(bytes, align);
    if (p != nullptr)
        return p;

    if (!grow(needed))
        return nullptr;

    return chunks_[chunk_count_ - 1]->alloc_bytes(bytes, align);
}

bool Arena::grow(std::size_t min_bytes) noexcept {
    if (chunk_count_ >= MAX_CHUNKS)
        return false;

    /*
     New chunk size is the larger of the geometric target and the
     caller's minimum, clamped at SIZE_CAP. If even the caller's minimum
     alone exceeds the cap, the request cannot be satisfied and we
     surface the OOM up to alloc_bytes.
    */
    std::size_t size = next_chunk_size_;
    if (size < min_bytes)
        size = min_bytes;
    if (size > Chunk::SIZE_CAP) {
        if (min_bytes > Chunk::SIZE_CAP)
            return false;
        size = Chunk::SIZE_CAP;
    }
    if (size == 0)
        return false;

    /*
     Two failure modes: nothrow new returning nullptr, and the chunk's
     own internal buffer alloc failing inside its constructor. Both
     surface as is_valid() == false.
    */
    Chunk *raw = new (std::nothrow) Chunk(size);
    if (raw == nullptr)
        return false;
    if (!raw->is_valid()) {
        delete raw;
        return false;
    }

    std::unique_ptr<Chunk> chunk(raw);
    if (chunk_count_ == 0) {
        head_ = std::move(chunk);
    } else {
        chunks_[chunk_count_ - 1]->next_ = std::move(chunk);
    }
    chunks_[chunk_count_] = raw;
    ++chunk_count_;
    total_capacity_ += size;

    // Record the first capped chunk so chunks_to_free_for can short-circuit.
    if (size == Chunk::SIZE_CAP && first_capped_chunk_ == MAX_CHUNKS)
        first_capped_chunk_ = chunk_count_ - 1;

    next_chunk_size_ = (next_chunk_size_ < Chunk::SIZE_CAP / 2)
                        ? next_chunk_size_ * 2
                        : Chunk::SIZE_CAP;

    return true;
}

void Arena::rewind() noexcept {
    for (std::size_t i = 0; i < chunk_count_; ++i)
        chunks_[i]->reset();
}

void Arena::reinitialise() noexcept {
    head_.reset();
    for (std::size_t i = 0; i < chunk_count_; ++i)
        chunks_[i] = nullptr;
    chunk_count_ = 0;
    total_capacity_ = 0;
    first_capped_chunk_ = MAX_CHUNKS;
    next_chunk_size_ = initial_size_;
    /*
     Best-effort reinit: if grow fails the arena stays invalid until
     the next alloc, which retries growth lazily.
    */
    grow(0);
}

bool Arena::free_top_k(std::size_t k) noexcept {
    if (k == 0)
        return false;
    if (k >= chunk_count_) {
        reinitialise();
        return true;
    }

    for (std::size_t i = chunk_count_ - k; i < chunk_count_; ++i)
        total_capacity_ -= chunks_[i]->capacity();

    /*
     Releasing next_ on the chunk that becomes the new tail RAII-frees
     the entire freed sub-chain in one step.
    */
    chunks_[chunk_count_ - k - 1]->next_.reset();

    for (std::size_t i = chunk_count_ - k; i < chunk_count_; ++i)
        chunks_[i] = nullptr;
    chunk_count_ -= k;

    if (first_capped_chunk_ >= chunk_count_)
        first_capped_chunk_ = MAX_CHUNKS;

    /*
     Restart geometric growth from the surviving tail's size so the
     next chunk is roughly twice it.
    */
    std::size_t tail_size = chunks_[chunk_count_ - 1]->capacity();
    next_chunk_size_ = (tail_size < Chunk::SIZE_CAP / 2)
                        ? tail_size * 2
                        : Chunk::SIZE_CAP;
    return false;
}

std::size_t Arena::chunks_to_free_for(std::size_t target) const noexcept {
    if (target == 0)
        return 0;
    if (target >= total_capacity_)
        return chunk_count_;

    std::size_t k = 0;
    std::size_t freed = 0;

    std::size_t capped_count = (first_capped_chunk_ < chunk_count_)
                                ? chunk_count_ - first_capped_chunk_
                                : 0;

    /*
     Capped chunks are the largest in the arena. Releasing one always
     overshoots a sub-cap target by orders of magnitude, so the entire
     capped section is skipped unless the cap is at most
     MIN_TARGET_TO_CHUNK_RATIO times the requested target.
    */
    if (capped_count > 0 && Chunk::SIZE_CAP <= MIN_TARGET_TO_CHUNK_RATIO * target) {
        std::size_t k_cap = (target + Chunk::SIZE_CAP - 1) / Chunk::SIZE_CAP;

        if (k_cap > capped_count) {
            k_cap = capped_count;
        }

        k += k_cap;
        freed += k_cap * Chunk::SIZE_CAP;
    }

    /*
     Geometric chunks are walked newest-first. Same worth-it test per
     chunk: stop as soon as the next candidate is too oversized
     relative to the remaining target.
    */
    if (freed < target) {
        std::size_t geo_count = first_capped_chunk_ < chunk_count_
                                 ? first_capped_chunk_
                                 : chunk_count_;

        for (std::size_t i = geo_count; i-- > 0 && freed < target;) {
            std::size_t chunk_size = chunks_[i]->capacity();
            std::size_t remaining = target - freed;
            if (chunk_size > MIN_TARGET_TO_CHUNK_RATIO * remaining)
                break;
            freed += chunk_size;
            ++k;
        }
    }

    return k;
}

} // namespace pangea