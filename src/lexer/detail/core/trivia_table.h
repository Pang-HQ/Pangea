#pragma once

/*
 Owns the in-flight state that classifies comments as leading or
 trailing trivia of a token, and writes finalised entries through
 caller-supplied vectors. The table does not own its storage.
*/

#include "../../trivia.h"
#include "../../../source/source_location.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace pangea::detail {

using TriviaIndex = std::uint32_t;
using TokenIndex = std::uint32_t;

class TriviaTable {
public:
    TriviaTable(std::pmr::vector<Trivia> &trivia,
                std::pmr::vector<TriviaAttachment> &attachments) noexcept
        : trivia_(trivia), attachments_(attachments) {}

    TriviaTable(const TriviaTable &) = delete;
    TriviaTable &operator=(const TriviaTable &) = delete;
    TriviaTable(TriviaTable &&) = delete;
    TriviaTable &operator=(TriviaTable &&) = delete;

    void push_line_comment(SourceRange range);
    void push_block_comment(SourceRange range);

    // Updates pending trivia when a token boundary is reached.
    void on_token_emitted(TokenIndex token_index);
    void on_line_ended();

    /*
     Seal any in-flight attachment and lock the table. Must be called
     exactly once, after the lexer has emitted its final token.
    */
    void finalise() noexcept;

#ifndef NDEBUG
    void audit() const;
#endif

private:
    static constexpr TriviaIndex INVALID_TRIVIA_INDEX =
        pangea::INVALID_TRIVIA_INDEX;

    static constexpr TokenIndex INVALID_TOKEN_INDEX =
        std::numeric_limits<TokenIndex>::max();

    TriviaIndex push(Trivia::Kind kind, SourceRange range);
    void note(TriviaIndex index);
    void seal();

    std::pmr::vector<Trivia> &trivia_;
    std::pmr::vector<TriviaAttachment> &attachments_;

    /*
     In-flight attachment. pending_token_ == INVALID_TOKEN_INDEX means
     trivia is queued as leading for whichever token comes next; once a
     token is bound, pending_lead_ / pending_trail_ are its lead and
     trail runs, mutable until the next on_token_emitted or
     on_line_ended seals them onto attachments_.
    */
    TokenIndex pending_token_ = INVALID_TOKEN_INDEX;
    TriviaIndex pending_lead_ = INVALID_TRIVIA_INDEX;
    TriviaIndex pending_trail_ = INVALID_TRIVIA_INDEX;

    bool finalised_ = false;
};

} // namespace pangea::detail
