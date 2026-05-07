#include "token.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <utility>

namespace pangea {

namespace {

struct KeywordEntry {
    std::string_view spelling;
    /*
     Default for empty table slots. The slot is only consulted after a
     spelling match, so this default is never observed - it just keeps
     KeywordEntry trivially value-constructible for the table builder.
    */
    TokenType type = TokenType::IDENTIFIER;
};

/*
 Spelled keywords and types both participate in identifier lookup.
 The cascade in token_kinds.def routes every type variant
 (INT/FLOAT/POINTER/plain TYPE) through DEFINE_TYPE and every keyword
 through DEFINE_KEYWORD, so two overrides cover the set without a
 separate category tag or filter pass.
*/
constexpr KeywordEntry KEYWORD_ENTRIES[] = {
#define DEFINE_TOKEN(name, spelling, category)
#define DEFINE_KEYWORD(name, spelling) {spelling, TokenType::name},
#define DEFINE_TYPE(name, spelling) {spelling, TokenType::name},
#include "token_kinds.def"
#undef DEFINE_TOKEN
#undef DEFINE_KEYWORD
#undef DEFINE_TYPE
};

constexpr std::size_t KEYWORD_COUNT = std::size(KEYWORD_ENTRIES);

/*
 Compile-time perfect hash for keyword recognition.

 Each spelling is fnv1a-hashed once into a base hash. At runtime and
 during the compile-time seed search, that base hash is folded with
 a seed through a murmur3-style finalizer to land in a power-of-two
 slot table. The finalizer is what decorrelates sequential seeds:
 plain fnv1a with the seed as the offset basis leaves adjacent seeds
 producing correlated outputs over short strings, so naive search
 does not converge.

 With ~50 keywords and TABLE_SIZE=256, a perfect mapping turns up in
 a few hundred seeds and the static table costs ~6 KiB of .rodata.
 Bump TABLE_SIZE to the next power of two if the static_assert below
 fires after the keyword set grows.
*/
constexpr std::size_t TABLE_SIZE = 256;
constexpr std::uint32_t TABLE_MASK = TABLE_SIZE - 1;
static_assert((TABLE_SIZE & TABLE_MASK) == 0,
              "TABLE_SIZE must be a power of two");
static_assert(TABLE_SIZE >= KEYWORD_COUNT,
              "TABLE_SIZE must hold every keyword");

constexpr std::uint32_t FNV_OFFSET_BASIS = 2166136261u;
constexpr std::uint32_t FNV_PRIME = 16777619u;

// Sentinel for find_perfect_seed; the search itself starts at seed = 1.
constexpr std::uint32_t SEED_NOT_FOUND = 0;

constexpr std::uint32_t fnv1a(std::string_view s) noexcept {
    std::uint32_t h = FNV_OFFSET_BASIS;
    for (char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= FNV_PRIME;
    }
    return h;
}

constexpr std::uint32_t mix(std::uint32_t base,
                            std::uint32_t seed) noexcept {
    std::uint32_t h = base ^ seed;
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

consteval std::array<std::uint32_t, KEYWORD_COUNT> compute_base_hashes() {
    std::array<std::uint32_t, KEYWORD_COUNT> hashes{};
    for (std::size_t i = 0; i < KEYWORD_COUNT; ++i) {
        hashes[i] = fnv1a(KEYWORD_ENTRIES[i].spelling);
    }
    return hashes;
}

constexpr std::array<std::uint32_t, KEYWORD_COUNT> BASE_HASHES =
    compute_base_hashes();

consteval std::uint32_t find_perfect_seed() {
    constexpr std::uint32_t MAX_TRIES = 20'000;

    for (std::uint32_t seed = 1; seed <= MAX_TRIES; ++seed) {
        std::array<bool, TABLE_SIZE> used{};
        bool collided = false;

        for (std::uint32_t base : BASE_HASHES) {
            const std::size_t idx = mix(base, seed) & TABLE_MASK;
            if (used[idx]) {
                collided = true;
                break;
            }
            used[idx] = true;
        }

        if (!collided) {
            return seed;
        }
    }

    return SEED_NOT_FOUND;
}

constexpr std::uint32_t HASH_SEED = find_perfect_seed();
static_assert(HASH_SEED != SEED_NOT_FOUND,
              "no perfect-hash seed found in MAX_TRIES; "
              "bump TABLE_SIZE or MAX_TRIES");

consteval std::array<KeywordEntry, TABLE_SIZE> build_keyword_table() {
    std::array<KeywordEntry, TABLE_SIZE> table{};
    for (const KeywordEntry &entry : KEYWORD_ENTRIES) {
        const std::size_t idx =
            mix(fnv1a(entry.spelling), HASH_SEED) & TABLE_MASK;
        table[idx] = entry;
    }
    return table;
}

constexpr std::array<KeywordEntry, TABLE_SIZE> KEYWORD_TABLE =
    build_keyword_table();

} // namespace

// Precondition: type is a real token kind, not COUNT.
std::string_view name_of(TokenType type) noexcept {
    assert(type != TokenType::COUNT);
    switch (type) {
#define DEFINE_TOKEN(name, spelling, category) \
        case TokenType::name: return #name;
#include "token_kinds.def"
#undef DEFINE_TOKEN
        case TokenType::COUNT:
            break;
    }
    std::unreachable();
}

TokenType lookup_keyword(std::string_view id) noexcept {
    assert(!id.empty());
    const std::size_t idx = mix(fnv1a(id), HASH_SEED) & TABLE_MASK;
    const KeywordEntry &slot = KEYWORD_TABLE[idx];
    if (slot.spelling == id) {
        return slot.type;
    }
    return TokenType::IDENTIFIER;
}

std::uint32_t newlines_of(const Token &tok) noexcept {
    if (tok.type != TokenType::SPECIAL_NEWLINE)
        return 0;

    const std::uint64_t *count = std::get_if<std::uint64_t>(&tok.payload);
    assert(count != nullptr);

    return static_cast<std::uint32_t>(*count);
}

} // namespace pangea
