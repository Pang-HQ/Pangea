#pragma once

/*
 Bundles the token sink, trivia table, source buffer, and reporter so
 scan functions have a single seam through which they emit tokens,
 emit errors, and report diagnostics.

 Every emit through this wrapper triggers trivia attachment. Direct
 calls to TokenSink::emit bypass attachment by design and should only
 be made for synthetic tokens (newline, EOF) where that is the wanted
 behaviour.
*/

#include "../../../diagnostics/diagnostic_codes.h"
#include "../../../source/source_location.h"
#include "../../token.h"
#include "token_sink.h"

#include <string_view>

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
            const SourceBuffer &buf,
            Diagnostics *reporter) noexcept;

    Emitter(const Emitter &) = delete;
    Emitter &operator=(const Emitter &) = delete;
    Emitter(Emitter &&) = delete;
    Emitter &operator=(Emitter &&) = delete;

    // Append a real token; pending leading trivia attaches to it.
    TokenIndex emit(TokenType type,
                    SourceRange range,
                    TokenPayload payload = TokenPayload{});

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
    const SourceBuffer &buf_;
    Diagnostics *reporter_;
};

} // namespace pangea::detail
