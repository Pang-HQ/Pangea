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

/*
 Compile-time-only mirror of token_kinds.def. The runtime table
 (LOOKUP_TABLE below) is built from this and never used afterward.
*/
struct KeywordEntry {
    std::string_view spelling = {};
    TokenType type = TokenType::IDENTIFIER;
};

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
 Runtime slot, packed into one uint64. Layout:

   bits 63..8 : upper 56 bits of mix(fnv1a(spelling), HASH_SEED)
   bits  7..0 : TokenType (an 8-bit enum)

 Lookup is a single compare of the upper 56 bits against the id's
 mixed hash. There is no separate "occupied vs empty" check: the
 perfect hash already encodes which slots belong to a keyword by
 construction. A slot the seed search never wrote stays at its
 default-init value, and an arbitrary identifier's mixed hash
 effectively never collides with that value, so a non-keyword id
 simply fails the upper-56-bits compare and falls through.
*/
using LookupSlot = std::uint64_t;

constexpr std::size_t TABLE_SIZE = 256;
constexpr std::uint64_t TABLE_MASK = TABLE_SIZE - 1;
static_assert((TABLE_SIZE & TABLE_MASK) == 0,
              "TABLE_SIZE must be a power of two");
static_assert(TABLE_SIZE >= KEYWORD_COUNT,
              "TABLE_SIZE must hold every keyword");

constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ull;
constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

// Sentinel for find_perfect_seed; the search itself starts at seed = 1.
constexpr std::uint64_t SEED_NOT_FOUND = 0;

constexpr std::uint64_t fnv1a(std::string_view s) noexcept {
    std::uint64_t h = FNV_OFFSET_BASIS;
    for (char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= FNV_PRIME;
    }
    return h;
}

/*
 Murmur3-style 64-bit finalizer, mixed with the seed up front.
 Decorrelates sequential seeds during the compile-time perfect-hash
 search; without the finalizer, plain fnv1a leaves adjacent seeds
 producing correlated outputs over short strings and the search
 fails to converge.
*/
constexpr std::uint64_t mix(std::uint64_t base, std::uint64_t seed) noexcept {
    std::uint64_t h = base ^ seed;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdull;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ull;
    h ^= h >> 33;
    return h;
}

constexpr LookupSlot pack_slot(std::uint64_t mixed_hash,
                               TokenType type) noexcept {
    return (mixed_hash & ~std::uint64_t{0xff})
         | static_cast<std::uint64_t>(static_cast<std::uint8_t>(type));
}

consteval std::array<std::uint64_t, KEYWORD_COUNT> compute_fnv_hashes() {
    std::array<std::uint64_t, KEYWORD_COUNT> hashes{};
    for (std::size_t i = 0; i < KEYWORD_COUNT; ++i) {
        hashes[i] = fnv1a(KEYWORD_ENTRIES[i].spelling);
    }
    return hashes;
}

constexpr std::array<std::uint64_t, KEYWORD_COUNT> FNV_HASHES =
    compute_fnv_hashes();

consteval std::uint64_t find_perfect_seed() {
    constexpr std::uint64_t MAX_TRIES = 20'000;

    for (std::uint64_t seed = 1; seed <= MAX_TRIES; ++seed) {
        std::array<bool, TABLE_SIZE> used{};
        bool collided = false;

        for (std::uint64_t h : FNV_HASHES) {
            const std::size_t idx = mix(h, seed) & TABLE_MASK;
            if (used[idx]) {
                collided = true;
                break;
            }
            used[idx] = true;
        }

        if (!collided)
            return seed;
    }

    return SEED_NOT_FOUND;
}

constexpr std::uint64_t HASH_SEED = find_perfect_seed();
static_assert(HASH_SEED != SEED_NOT_FOUND,
              "no perfect-hash seed found in MAX_TRIES; "
              "bump TABLE_SIZE or MAX_TRIES");

consteval std::array<LookupSlot, TABLE_SIZE> build_lookup_table() {
    std::array<LookupSlot, TABLE_SIZE> table{};
    for (std::size_t i = 0; i < KEYWORD_COUNT; ++i) {
        const std::uint64_t mixed = mix(FNV_HASHES[i], HASH_SEED);
        const std::size_t idx = mixed & TABLE_MASK;
        table[idx] = pack_slot(mixed, KEYWORD_ENTRIES[i].type);
    }
    return table;
}

constexpr std::array<LookupSlot, TABLE_SIZE> LOOKUP_TABLE =
    build_lookup_table();

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
    const std::uint64_t mixed = mix(fnv1a(id), HASH_SEED);
    const LookupSlot slot = LOOKUP_TABLE[mixed & TABLE_MASK];

    if (((slot ^ mixed) >> 8) == 0)
        return static_cast<TokenType>(slot & 0xFF);

    return TokenType::IDENTIFIER;
}

std::uint32_t newlines_of(const Token &tok) noexcept {
    if (tok.type != TokenType::SPECIAL_NEWLINE)
        return 0;

    return tok.payload;
}

SymbolID symbol_of(const Token &tok) noexcept {
    assert(tok.type == TokenType::LITERAL_STRING
        || tok.type == TokenType::LITERAL_C_STRING);
    return SymbolID{tok.payload};
}

} // namespace pangea
