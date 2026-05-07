#pragma once

/*
 Token definitions for the lexer.

 Token kinds come from token_kinds.def. The payload field is a single
 uint32_t whose meaning is keyed by Token::type:

   LITERAL_STRING / LITERAL_C_STRING : SymbolID into the StringPool
   SPECIAL_NEWLINE                   : count of collapsed newlines
   LITERAL_INTEGER                   : index into LexerOutput::integer_literals
   LITERAL_FLOAT                     : index into LexerOutput::float_literals
   anything else                     : unused (zero)

 The two literal-by-index cases keep the 8-byte values out of every
 token; the alternative (a tagged union or std::variant) would force
 8-byte alignment on every token and balloon the struct to 24-32
 bytes for a payload that ~95% of tokens do not use.

 Comments and whitespace live out-of-line on LexerOutput; see trivia.h.
*/

#include "../source/string_pool.h"
#include "../source/source_location.h"

#include <cstdint>
#include <string_view>

namespace pangea {

// One enum value per DEFINE_TOKEN entry.
enum class TokenType : std::uint8_t {
#define DEFINE_TOKEN(name, spelling, category) name,
#include "token_kinds.def"
#undef DEFINE_TOKEN
    // Table sentinel. Not a real token kind.
    COUNT
};

// Once initialised, tokens should remain read-only.
struct Token {
    const TokenType type;
    const SourceRange range;
    const std::uint32_t payload = 0;
};

static_assert(sizeof(Token) == 16,
              "Token grew beyond its 16-byte budget "
              "- check field layout and SourceRange size");

// Precondition: type is a real token kind, not COUNT.
std::string_view name_of(TokenType type) noexcept;

// Returns the keyword or type token for id, or IDENTIFIER.
// Precondition: id is not empty.
TokenType lookup_keyword(std::string_view id) noexcept;

/*
 Number of physical newlines collapsed into the given token. Non-zero
 only for SPECIAL_NEWLINE, where the lexer stores the count directly
 in the payload so formatters can distinguish a terminator from a
 paragraph break. Returns 0 for every other token kind.
*/
std::uint32_t newlines_of(const Token &tok) noexcept;

/*
 SymbolID payload for string-literal tokens. Precondition: tok.type
 is LITERAL_STRING or LITERAL_C_STRING; the assert catches misuse on
 any other kind, which would silently misinterpret the payload.
*/
SymbolID symbol_of(const Token &tok) noexcept;

} // namespace pangea
