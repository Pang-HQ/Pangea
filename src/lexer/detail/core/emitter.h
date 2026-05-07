#pragma once

/*
 Bundles the token sink, trivia table, source buffer, and reporter so
 scan functions have a single seam through which they emit tokens,
 emit errors, and report diagnostics.

 Every emit through this wrapper triggers trivia attachment. Direct
 calls to TokenSink::emit bypass attachment by design and should only
 be made for synthetic tokens (newline, EOF) where that is the wanted
 behaviour.

 Payload-bearing emits dispatch by the value's type:
   - SymbolID       : stored inline in Token::payload
   - std::uint64_t  : pushed into LexerOutput::integer_literals;
                      the index lands in Token::payload
   - double         : pushed into LexerOutput::float_literals;
                      the index lands in Token::payload

 Side-table pushes happen here so scan functions stay unaware of the
 storage split.
*/

#include "../../../diagnostics/diagnostic_codes.h"
#include "../../../source/source_location.h"
#include "../../../source/string_pool.h"
#include "../../token.h"
#include "token_sink.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace pangea {
class Diagnostics;
class SourceBuffer;
} // namespace pangea

namespace pangea::detail {

class TriviaTable;

class Emitter {
public:
    Emitter(TokenSink &out,
            TriviaTable &trivia,
            std::pmr::vector<std::uint64_t> &integer_literals,
            std::pmr::vector<double> &float_literals,
            const SourceBuffer &buf,
            Diagnostics *reporter) noexcept;

    Emitter(const Emitter &) = delete;
    Emitter &operator=(const Emitter &) = delete;
    Emitter(Emitter &&) = delete;
    Emitter &operator=(Emitter &&) = delete;

    // Append a real token; pending leading trivia attaches to it.
    TokenIndex emit(TokenType type, SourceRange range);
    TokenIndex emit(TokenType type, SourceRange range, SymbolID id);
    TokenIndex emit(TokenType type, SourceRange range, std::uint64_t value);
    TokenIndex emit(TokenType type, SourceRange range, double value);

    // Report a diagnostic and emit a LEX_ERROR token over the range.
    void emit_error(SourceRange range,
                    DiagnosticCode code,
                    std::string_view message);

    // Report a diagnostic without emitting any token.
    void report(DiagnosticCode code,
                SourceRange range,
                std::string_view message,
                ErrorLevel level = ErrorLevel::ERROR);

private:
    TokenSink &out_;
    TriviaTable &trivia_;
    std::pmr::vector<std::uint64_t> &integer_literals_;
    std::pmr::vector<double> &float_literals_;
    const SourceBuffer &buf_;
    Diagnostics *reporter_;

    // Common tail: stamp the trivia attachment for the just-emitted token.
    void attach_trivia(TokenIndex index);
};

} // namespace pangea::detail
