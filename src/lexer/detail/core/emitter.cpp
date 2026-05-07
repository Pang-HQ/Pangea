#include "emitter.h"

#include "trivia_table.h"
#include "../../../diagnostics/diagnostics.h"
#include "../../../source/source_buffer.h"

#include <cassert>
#include <limits>

namespace pangea::detail {

Emitter::Emitter(TokenSink &out,
                 TriviaTable &trivia,
                 std::pmr::vector<std::uint64_t> &integer_literals,
                 std::pmr::vector<double> &float_literals,
                 const SourceBuffer &buf,
                 Diagnostics *reporter) noexcept
    : out_(out),
      trivia_(trivia),
      integer_literals_(integer_literals),
      float_literals_(float_literals),
      buf_(buf),
      reporter_(reporter) {}

TokenIndex Emitter::emit(TokenType type, SourceRange range) {
    const TokenIndex index = out_.emit(type, range);
    attach_trivia(index);
    return index;
}

TokenIndex Emitter::emit(TokenType type, SourceRange range, SymbolID id) {
    const TokenIndex index = out_.emit(type, range, id.value);
    attach_trivia(index);
    return index;
}

TokenIndex Emitter::emit(TokenType type,
                         SourceRange range,
                         std::uint64_t value) {
    /*
     Token::payload is uint32_t, so the side-table can hold at most
     ~4 billion integer literals across one compile. That is well past
     anything a real source file will produce, so the assert is fine
     as a hard precondition rather than a soft diagnostic.
    */
    assert(integer_literals_.size()
           < std::numeric_limits<std::uint32_t>::max());

    const auto idx = static_cast<std::uint32_t>(integer_literals_.size());
    integer_literals_.push_back(value);

    const TokenIndex index = out_.emit(type, range, idx);
    attach_trivia(index);
    return index;
}

TokenIndex Emitter::emit(TokenType type, SourceRange range, double value) {
    assert(float_literals_.size()
           < std::numeric_limits<std::uint32_t>::max());

    const auto idx = static_cast<std::uint32_t>(float_literals_.size());
    float_literals_.push_back(value);

    const TokenIndex index = out_.emit(type, range, idx);
    attach_trivia(index);
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

void Emitter::attach_trivia(TokenIndex index) {
    trivia_.on_token_emitted(index);
}

} // namespace pangea::detail
