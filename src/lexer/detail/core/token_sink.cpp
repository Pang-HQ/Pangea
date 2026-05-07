#include "token_sink.h"

#include <cassert>
#include <utility>

namespace pangea::detail {

TokenIndex TokenSink::emit(TokenType type,
                           SourceRange range,
                           TokenPayload payload) {
    const TokenIndex index = count_;
    [[maybe_unused]] const Token *t =
        tokens_.emplace(type, range, std::move(payload));
    assert(t != nullptr && "token arena exhausted");
    ++count_;
    return index;
}

void TokenSink::flush_pending_newline(SourceOffset offset) {
    if (pending_newlines_ == 0)
        return;

    const std::uint64_t count = pending_newlines_;
    pending_newlines_ = 0;

    /*
     SPECIAL_NEWLINE bypasses any trivia attachment by design: trivia
     between two real tokens belongs to the next real token, not to
     the synthetic terminator that sits between them.
    */
    [[maybe_unused]] const Token *t = tokens_.emplace(
        TokenType::SPECIAL_NEWLINE,
        SourceRange{offset, 0},
        count);
    assert(t != nullptr && "token arena exhausted");
    ++count_;
}

TokenIndex TokenSink::emit_eof(SourceOffset offset) {
    const TokenIndex index = count_;
    [[maybe_unused]] const Token *t = tokens_.emplace(
        TokenType::SPECIAL_EOF,
        SourceRange{offset, 0});
    assert(t != nullptr && "token arena exhausted");
    ++count_;
    return index;
}

void TokenSink::add_pending_newline() {
    ++pending_newlines_;
}

void TokenSink::drop_pending_newlines() {
    pending_newlines_ = 0;
}

bool TokenSink::empty() const noexcept {
    return count_ == 0;
}

} // namespace pangea::detail
