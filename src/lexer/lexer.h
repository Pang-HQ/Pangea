#pragma once

/*
 One-shot lexer for a single SourceBuffer.

 The lexer reads bytes from a caller-owned SourceBuffer and writes
 tokens, trivia, and trivia attachments into a caller-owned
 LexerOutput. String literals and identifiers are interned into a
 caller-owned StringPool; only the resulting SymbolIDs land in Token
 payloads. Tokens always end with SPECIAL_EOF.

 Trivia live out-of-line. A TriviaAttachment links a token to its
 leading and / or trailing trivia run in LexerOutput::trivia. Tokens
 without any trivia have no attachment entry.

 LexerOutput is logically write-once: the lexer fills it during
 tokenise(), and consumers should treat it as read-only afterward.
 The lexer does not guard against post-return mutation.
*/

#include "output.h"
#include "detail/core/bracket_stack.h"
#include "detail/core/cursor.h"
#include "detail/core/emitter.h"
#include "detail/core/token_sink.h"
#include "detail/core/trivia_table.h"
#include "../source/source_buffer.h"
#include "../source/string_pool.h"

#include <string>

namespace pangea {

// Full definition in diagnostics/diagnostics.h; kept out of this header
// so lexer.h consumers don't pay for its STL dependencies.
class Diagnostics;

class Lexer {
public:
    /*
     buf must outlive out because token ranges point into it. out is
     the LexerOutput the lexer fills; it must outlive any consumer
     reading from it. strings is the pool that string literals and
     identifiers will be interned into; it must outlive any consumer
     that resolves a SymbolID from a Token payload. reporter may be
     nullptr; diagnostics are then dropped, but the lexer still emits
     LEX_ERROR tokens for recovery.
    */
    [[nodiscard]]
    Lexer(const SourceBuffer &buf,
          LexerOutput &out,
          StringPool &strings,
          Diagnostics *reporter);  // TODO: make reporter a reference

    Lexer(const Lexer &) = delete;
    Lexer &operator=(const Lexer &) = delete;
    Lexer(Lexer &&) = delete;
    Lexer &operator=(Lexer &&) = delete;

    void tokenise();

private:
    // External (caller-owned).
    const SourceBuffer &buf_;
    LexerOutput &out_;
    StringPool &strings_;

    // Internal lex state.
    detail::SourceCursor cursor_;
    detail::TokenSink toks_;
    detail::TriviaTable trivia_;
    detail::BracketStack brackets_;

    // Coordinator over the internal state.
    detail::Emitter emitter_;

    /*
     Reusable scratch buffer for unescape decoding of string literals.
     Cleared on every scan; capacity grows as needed and is retained
     across calls so most literals end up allocator-free.
    */
    std::string decode_buf_;

    bool done_ = false;

    void handle_line_ending() noexcept;
    void scan_line_comment();
    void scan_block_comment();
    void scan_one();
    void emit_eof();
};

} // namespace pangea
