#pragma once

/*
 StringPool is the storage backing for decoded string-literal payloads
 and identifier names produced by the lexer. Callers hand it a view
 and get back a SymbolID; view(id) recovers the bytes later.

 The pool deduplicates by content: interning the same bytes twice
 yields the same SymbolID, so id equality is equivalent to string
 equality.

 Views recovered from view() stay valid for the pool's lifetime:
 subsequent intern() calls never invalidate views returned from
 earlier ones. Storage is a chunked Arena so the pool can grow
 without copying interned bytes.

 Every interned string is implicitly NUL-terminated: the byte at
 view(id).data()[view(id).size()] is guaranteed to read '\0'.
 intern_with_nul() documents that contract at the callsite.

 The pool is non-copyable and non-movable on purpose: PMR containers
 inside hold a raw memory_resource* that must not change address.
 The intended ownership model is one StringPool living at the top of
 the compiler driver and threaded by reference through lex, parse,
 and sema.
*/

#include "../arena/arena.h"
#include "../arena/arena_resource.h"

#include <cstdint>
#include <limits>
#include <memory_resource>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pangea {

/*
 Dense identifier for a string interned in a StringPool. Equal IDs
 imply equal bytes (the pool deduplicates by content). The pool that
 minted an id is implicit; ids from different pools are not
 interchangeable.
*/
struct SymbolID {
    static constexpr std::uint32_t INVALID_VALUE =
        std::numeric_limits<std::uint32_t>::max();

    const std::uint32_t value;

    static constexpr SymbolID invalid() noexcept {
        return {INVALID_VALUE};
    }

    constexpr bool is_valid() const noexcept {
        return value != INVALID_VALUE;
    }

    constexpr bool operator==(SymbolID other) const noexcept {
        return value == other.value;
    }

    constexpr bool operator!=(SymbolID other) const noexcept {
        return value != other.value;
    }
};

class StringPool {
public:
    StringPool() = default;

    StringPool(const StringPool &) = delete;
    StringPool &operator=(const StringPool &) = delete;
    StringPool(StringPool &&) = delete;
    StringPool &operator=(StringPool &&) = delete;

    /*
     Look up s by content; if present, return the existing id. Else
     append s and register it. On overflow returns SymbolID::invalid()
     and flips overflowed() to true; prior interns remain intact.
    */
    SymbolID intern(std::string_view s);

    /*
     Same as intern(); kept as a separate name so the C-string
     contract is documented at the callsite. Every interned string
     is NUL-terminated regardless of which entry point was used.
    */
    SymbolID intern_with_nul(std::string_view s);

    // Recover the bytes previously interned under this id.
    std::string_view view(SymbolID id) const noexcept;

    // Sticky overflow flag; frontend surfaces this as a diagnostic.
    bool overflowed() const noexcept;

private:
    /*
     Transparent hash and equality so the dedup map can be probed by
     string_view without materialising a temporary key. The map's
     keys are views into bytes_arena_-owned bytes that stay alive
     for the pool's lifetime.
    */
    struct StringViewHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view s) const noexcept;
    };

    struct StringViewEq {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const noexcept;
    };

    /*
     PERF: std::pmr::unordered_map keeps the bucket-with-chains layout
     of std::unordered_map but routes node allocation through the
     ArenaResource, replacing per-node operator new with arena bumps.
     If profiling promotes this map to a hot spot, swap the type for
     a flat hash map (e.g. ankerl::unordered_dense) - the public
     StringPool API does not depend on the choice.
    */
    using DedupMap = std::pmr::unordered_map<
        std::string_view,
        SymbolID,
        StringViewHash,
        StringViewEq>;

    Arena bytes_arena_;
    ArenaResource map_resource_{bytes_arena_};
    DedupMap dedup_{&map_resource_};

    /*
     Index views_[id.value] -> stable arena bytes for id. Routed
     through map_resource_ so every internal allocation lives in
     bytes_arena_.
    */
    std::pmr::vector<std::string_view> views_{&map_resource_};

    bool overflowed_ = false;
};

} // namespace pangea
