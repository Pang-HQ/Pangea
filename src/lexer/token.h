#pragma once

/*
 Token definitions for the lexer.

 Token kinds come from token_kinds.def. Payloads hold immediate literal
 data only; string literals carry a SymbolID into the StringPool so
 Token stays small. Comments/trivia live out-of-line on Lexer::Output;
 see trivia.h.
*/

#include "../source/string_pool.h"
#include "../source/source_location.h"

#include <cstdint>
#include <string_view>
#include <variant>

namespace pangea {

// One enum value per DEFINE_TOKEN entry.
enum class TokenType : std::uint8_t {
#define DEFINE_TOKEN(name, spelling, category) name,
#include "token_kinds.def"
#undef DEFINE_TOKEN
    // Table sentinel. Not a real token kind.
    COUNT
};

using TokenPayload = std::variant<
    std::monostate,
    std::uint64_t,
    double,
    SymbolID>;

// Once initialised, tokens should remain read-only.
struct Token {
    const TokenType type;
    const SourceRange range;
    const TokenPayload payload = {};
};

// 40 leaves slack for payload growth.
static_assert(sizeof(Token) <= 40,
              "Token grew beyond its size budget "
              "- check TokenPayload alternatives");

// Precondition: type is a real token kind, not COUNT.
std::string_view name_of(TokenType type) noexcept;

// Returns the keyword or type token for id, or IDENTIFIER.
// Precondition: id is not empty.
TokenType lookup_keyword(std::string_view id) noexcept;

/*
 Number of physical newlines collapsed into the given token. Non-zero
 only for SPECIAL_NEWLINE, where the lexer stores the count in the
 payload so formatters can distinguish a terminator from a paragraph
 break. Returns 0 for every other token kind.
*/
std::uint32_t newlines_of(const Token &tok) noexcept;

} // namespace pangea
