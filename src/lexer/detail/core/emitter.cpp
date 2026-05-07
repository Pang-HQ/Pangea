#include "emitter.h"

#include "trivia_table.h"
#include "../../../diagnostics/diagnostics.h"
#include "../../../source/source_buffer.h"

#include <utility>

namespace pangea::detail {

Emitter::Emitter(TokenSink &out,
                 TriviaTable &trivia,
                 const SourceBuffer &buf,
                 Diagnostics *reporter) noexcept
    : out_(out),
      trivia_(trivia),
      buf_(buf),
      reporter_(reporter) {}

TokenIndex Emitter::emit(TokenType type,
                         SourceRange range,
                         TokenPayload payload) {
    const TokenIndex index = out_.emit(type, range, std::move(payload));
    trivia_.on_token_emitted(index);
    return index;
}

void Emitter::emit_error(SourceRange range,
                         DiagnosticCode code,
                         std::string_view message) {
    report(code, range, message);
    emit(TokenType::LEX_ERROR, range);
}

void Emitter::report(DiagnosticCode code,
                     SourceRange range,
                     std::string_view message,
                     ErrorLevel level) {
    if (reporter_ == nullptr)
        return;

    const SourceLocation loc = buf_.location_of(range);
    reporter_->report(code, level, loc, message);
}

} // namespace pangea::detail
