#pragma once

/*
 Append-only token accumulator. Owns the in-flight blank-line counter
 used to collapse runs of physical newlines into a single
 SPECIAL_NEWLINE token.

 Tokens are written into a caller-supplied TypedArena<Token>; the
 sink does not own storage. Knows nothing about trivia attachment:
 that policy is bolted on at the orchestration layer via the Emitter
 wrapper.
*/

#include "../../token.h"
#include "../../../arena/typed_arena.h"
#include "../../../source/source_location.h"

#include <cstdint>

namespace pangea::detail {

using TokenIndex = std::uint32_t;

class TokenSink {
public:
    // buf_size is the size of the SourceBuffer that the TokenSink is used for.
    explicit TokenSink(TypedArena<Token> &tokens) noexcept
        : tokens_(tokens) {}

    TokenSink(const TokenSink &) = delete;
    TokenSink &operator=(const TokenSink &) = delete;
    TokenSink(TokenSink &&) = delete;
    TokenSink &operator=(TokenSink &&) = delete;

    // Append a real token and return its index.
    TokenIndex emit(TokenType type,
                    SourceRange range,
                    TokenPayload payload = TokenPayload{});

    /*
     Emit SPECIAL_NEWLINE collecting the buffered blank-line run; the
     count is stored in the token's payload. No-op if no newlines are
     pending.
    */
    void flush_pending_newline(SourceOffset offset);

    // Append SPECIAL_EOF and return its index.
    TokenIndex emit_eof(SourceOffset offset);

    // One physical line ending was observed at top level.
    void add_pending_newline();

    // Discard any pending newlines without emitting a token.
    void drop_pending_newlines();

    [[nodiscard]]
    bool empty() const noexcept;

private:
    TypedArena<Token> &tokens_;

    // Number of tokens emitted; used both to mint indices and to
    // answer empty() in O(1).
    TokenIndex count_ = 0;

    // Buffered blank-line run; payload of the next SPECIAL_NEWLINE.
    std::uint32_t pending_newlines_ = 0;
};

} // namespace pangea::detail
