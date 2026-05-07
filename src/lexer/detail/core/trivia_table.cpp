#include "trivia_table.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace pangea::detail {

void TriviaTable::push_line_comment(SourceRange range) {
    assert(!finalised_);
    note(push(Trivia::Kind::LINE_COMMENT, range));
}

void TriviaTable::push_block_comment(SourceRange range) {
    assert(!finalised_);
    note(push(Trivia::Kind::BLOCK_COMMENT, range));
}

void TriviaTable::on_token_emitted(TokenIndex token_index) {
    assert(!finalised_);
    assert(token_index != INVALID_TOKEN_INDEX);

    /*
     Same-line follow-up: any in-flight trail must be a block comment
     (// would have ended the line), so it belongs to this new token
     as leading, not to the previous token as trailing.
    */
    TriviaIndex carried = INVALID_TRIVIA_INDEX;
    if (pending_token_ != INVALID_TOKEN_INDEX) {
        carried = pending_trail_;
        pending_trail_ = INVALID_TRIVIA_INDEX;
        seal();
    }

    pending_token_ = token_index;
    if (carried != INVALID_TRIVIA_INDEX) {
        pending_lead_ = carried;
    }
}

void TriviaTable::on_line_ended() {
    assert(!finalised_);
    seal();
}

void TriviaTable::finalise() noexcept {
    assert(!finalised_);
    seal();
    finalised_ = true;
}

TriviaIndex TriviaTable::push(Trivia::Kind kind, SourceRange range) {
    assert(trivia_.size() < static_cast<std::size_t>(INVALID_TRIVIA_INDEX));

    const TriviaIndex index = static_cast<TriviaIndex>(trivia_.size());
    trivia_.push_back({kind, range});
    return index;
}

void TriviaTable::note(TriviaIndex index) {
    // No token bound yet → leading of whatever token comes next.
    // Token already bound → trailing of that token (a block may later
    // be promoted to the next token's leading by on_token_emitted).
    TriviaIndex &slot = (pending_token_ == INVALID_TOKEN_INDEX)
                            ? pending_lead_
                            : pending_trail_;

    // First index of a contiguous run wins; later ones extend it.
    if (slot == INVALID_TRIVIA_INDEX) {
        slot = index;
    }
}

void TriviaTable::seal() {
    if (pending_token_ == INVALID_TOKEN_INDEX) {
        return;
    }

    if (pending_lead_ != INVALID_TRIVIA_INDEX
            || pending_trail_ != INVALID_TRIVIA_INDEX) {
        assert(attachments_.empty()
               || attachments_.back().token_index < pending_token_);

        attachments_.push_back({
            pending_token_,
            TokenTrivia{pending_lead_, pending_trail_},
        });
    }

    pending_token_ = INVALID_TOKEN_INDEX;
    pending_lead_ = INVALID_TRIVIA_INDEX;
    pending_trail_ = INVALID_TRIVIA_INDEX;
}

#ifndef NDEBUG
void TriviaTable::audit() const {
    std::vector<TriviaIndex> starts;
    starts.reserve(attachments_.size() * 2);

    for (const TriviaAttachment &att : attachments_) {
        if (att.trivia.leading != INVALID_TRIVIA_INDEX) {
            starts.push_back(att.trivia.leading);
        }
        if (att.trivia.trailing != INVALID_TRIVIA_INDEX) {
            starts.push_back(att.trivia.trailing);
        }
    }

    std::sort(starts.begin(), starts.end());

    if (!trivia_.empty()) {
        assert(!starts.empty() &&
               "no trivia runs found despite non-empty trivia vector");
        assert(starts.front() == 0 &&
               "trivia[0] is not claimed by any attachment");
    }

    for (std::size_t i = 1; i < starts.size(); ++i) {
        assert(starts[i] != starts[i - 1] &&
               "two attachment runs claim the same trivia index");
    }

    for (std::size_t i = 1; i < attachments_.size(); ++i) {
        assert(attachments_[i - 1].token_index < attachments_[i].token_index);
    }
}
#endif

} // namespace pangea::detail
