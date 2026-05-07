#pragma once

/*
 LexerOutput is the data contract between the lexer and everything
 downstream of it (parser, formatters, diagnostics). The lexer
 writes into a caller-owned LexerOutput; consumers read from the
 same instance.

 Storage layout:
   - tokens                          : TypedArena<Token> (16 B per entry)
   - trivia, trivia_attachments,
     integer_literals, float_literals: pmr::vectors backed by
                                       aux_arena via aux_resource

 The aux arena backs every out-of-line side table: trivia ranges,
 trivia-to-token attachments, and the actual values of integer /
 float literals (Token only stores indices into the latter two).
 One growth chain feeds all of them, so they share one set of chunk
 allocations.

 LexerOutput is non-copyable and non-movable on purpose. The pmr
 containers inside hold a raw memory_resource* that must not change
 address; the same constraint that keeps StringPool pinned to its
 owner applies here. The intended ownership model is one
 LexerOutput per source file, owned by the driver and threaded by
 reference through lex and parse.

 LexerOutput is logically write-once: the lexer fills it during
 tokenise(), and consumers should treat it as read-only afterward.
*/

#include "token.h"
#include "trivia.h"

#include "../arena/arena.h"
#include "../arena/arena_resource.h"
#include "../arena/typed_arena.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory_resource>
#include <vector>

namespace pangea {

struct LexerOutput {
    explicit LexerOutput(std::size_t buf_size)
    : tokens(buf_size) {}

    LexerOutput(const LexerOutput &) = delete;
    LexerOutput &operator=(const LexerOutput &) = delete;
    LexerOutput(LexerOutput &&) = delete;
    LexerOutput &operator=(LexerOutput &&) = delete;

    // Forward-iterable, append-only token stream. Always ends with
    // SPECIAL_EOF once tokenise() has finished.
    TypedArena<Token> tokens;

    /*
     Shared arena + pmr resource backing every out-of-line side table:
     trivia, trivia attachments, and the literal value vectors below.
     One growth chain feeds all of them.
    */
    Arena aux_arena;
    ArenaResource aux_resource{aux_arena};

    /*
     Trivia entries in source order. trivia_attachments index into
     this vector; consumers walk forward from a run start until the
     next attachment claims a later index, or until end().
    */
    std::pmr::vector<Trivia> trivia{&aux_resource};

    /*
     One entry per token that has any trivia attached, sorted by
     token_index so callers can binary-search.
    */
    std::pmr::vector<TriviaAttachment> trivia_attachments{&aux_resource};

    /*
     Out-of-line storage for LITERAL_INTEGER and LITERAL_FLOAT values.
     Token::payload holds the uint32_t index into one of these vectors,
     keyed by Token::type. Keeping the values out of Token is what
     lets Token stay 16 bytes regardless of how many numeric literals
     a source has.
    */
    std::pmr::vector<std::uint64_t> integer_literals{&aux_resource};
    std::pmr::vector<double> float_literals{&aux_resource};

    /*
     Return the TokenTrivia for token_index, or nullptr if the
     token has no attached trivia. Binary search; O(log n).
    */
    [[nodiscard]]
    const TokenTrivia *find_trivia(
        std::uint32_t token_index
    ) const noexcept {
        const auto it = std::lower_bound(
            trivia_attachments.begin(),
            trivia_attachments.end(),
            token_index,
            [](const TriviaAttachment &a, std::uint32_t idx) {
                return a.token_index < idx;
            });

        if (it == trivia_attachments.end()
                || it->token_index != token_index) {
            return nullptr;
        }
        return &it->trivia;
    }
};

/*
 Resolve the uint64_t value of a LITERAL_INTEGER token. Precondition:
 tok.type is LITERAL_INTEGER; the payload is its index into
 out.integer_literals.
*/
[[nodiscard]]
inline std::uint64_t int_value_of(const Token &tok,
                                  const LexerOutput &out) noexcept {
    assert(tok.type == TokenType::LITERAL_INTEGER);
    assert(tok.payload < out.integer_literals.size());
    return out.integer_literals[tok.payload];
}

/*
 Resolve the double value of a LITERAL_FLOAT token. Precondition:
 tok.type is LITERAL_FLOAT; the payload is its index into
 out.float_literals.
*/
[[nodiscard]]
inline double float_value_of(const Token &tok,
                              const LexerOutput &out) noexcept {
    assert(tok.type == TokenType::LITERAL_FLOAT);
    assert(tok.payload < out.float_literals.size());
    return out.float_literals[tok.payload];
}

} // namespace pangea
