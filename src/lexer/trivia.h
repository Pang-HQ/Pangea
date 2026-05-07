#pragma once

/*
 Trivia are the comments the lexer keeps attached to tokens.

 Tokens do not carry trivia indices directly. Lexer::Output owns a
 sorted side table of TriviaAttachment; tokens without trivia have
 no entry and pay no per-token overhead.
*/

#include "../source/source_location.h"

#include <cstdint>
#include <limits>

namespace pangea {

inline constexpr std::uint32_t INVALID_TRIVIA_INDEX =
    std::numeric_limits<std::uint32_t>::max();

/*
 A single trivia entry (line or block comment) captured from the
 source. Lexer::Output owns one flat vector of these in source order.

 Tokens point at the FIRST trivia in their run; the run extends up
 to (but not including) the next trivia index claimed by any later
 attachment, or Lexer::Output::trivia.size() if nothing claims a
 later index.
*/
struct Trivia {
    // Keep inline; keeps namespace cleaner.
    enum class Kind : std::uint8_t {
        LINE_COMMENT,
        BLOCK_COMMENT,
    };

    const Kind kind;
    const SourceRange range;
};

/*
 Leading / trailing trivia links for a single token. Either field
 may be INVALID_TRIVIA_INDEX when that side has no trivia attached.
*/
struct TokenTrivia {
    const std::uint32_t leading;
    const std::uint32_t trailing;
};

/*
 One entry per token that has any trivia attached. Stored sorted by
 token_index on Lexer::Output so consumers can binary-search.
*/
struct TriviaAttachment {
    const std::uint32_t token_index;
    const TokenTrivia trivia;
};

} // namespace pangea
