#include "string_pool.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>

namespace pangea {

std::size_t StringPool::StringViewHash::operator()(
    std::string_view s
) const noexcept {
    return std::hash<std::string_view>{}(s);
}

bool StringPool::StringViewEq::operator()(
    std::string_view a,
    std::string_view b
) const noexcept {
    return a == b;
}

SymbolID StringPool::intern(std::string_view s) {
    // Overflow is sticky: once set, future calls return invalid.
    if (overflowed_) {
        return SymbolID::invalid();
    }

    /*
     Content dedup: identical bytes always map to the same id, so
     id equality downstream is equivalent to string equality. The
     transparent hash lets us probe by string_view without copying.
    */
    if (auto it = dedup_.find(s); it != dedup_.end()) {
        return it->second;
    }

    /*
     SymbolID is a u32; refuse to mint one that collides with the
     invalid sentinel rather than silently aliasing a valid id with
     INVALID_VALUE.
    */
    if (views_.size() >= SymbolID::INVALID_VALUE) {
        overflowed_ = true;
        return SymbolID::invalid();
    }

    /*
     +1 for the trailing NUL: every interned string is implicitly
     NUL-terminated so intern_with_nul has a free contract to honour.
    */
    const std::size_t need = s.size() + 1;

    char *p = bytes_arena_.alloc<char>(need);
    if (p == nullptr) {
        overflowed_ = true;
        return SymbolID::invalid();
    }

    if (!s.empty()) {
        std::memcpy(p, s.data(), s.size());
    }
    p[s.size()] = '\0';

    const SymbolID id{static_cast<std::uint32_t>(views_.size())};

    /*
     The view we record (and use as a dedup key) must point at the
     freshly-allocated arena bytes. The caller's input is allowed to
     dangle the moment intern() returns; the arena copy is what
     stays alive for the pool's lifetime.
    */
    const std::string_view stable(p, s.size());

    views_.push_back(stable);
    dedup_.emplace(stable, id);

    return id;
}

SymbolID StringPool::intern_with_nul(std::string_view s) {
    /*
     intern() always writes a trailing NUL one past the logical end.
     intern_with_nul exists to document the C-string contract at the
     callsite.
    */
    return intern(s);
}

std::string_view StringPool::view(SymbolID id) const noexcept {
    /*
     An invalid id is a legitimate "no string here" answer (e.g. a
     default-constructed Token payload) so yield an empty view and
     move on.
    */
    if (!id.is_valid()) {
        return {};
    }

    /*
     An out-of-range id can only mean the caller is holding an id
     from another pool or has corrupted one - silently clamping
     would mask that bug, so abort.
    */
    assert(id.value < views_.size());
    if (id.value >= views_.size()) {
        std::abort();
    }

    return views_[id.value];
}

bool StringPool::overflowed() const noexcept {
    return overflowed_;
}

} // namespace pangea
