#pragma once

/*
 Tracks open brackets so the orchestration loop can keep newlines
 inside (), [], and {} classified as trivia. Carries no diagnostic
 policy and no positions: balance reporting is the parser's job.
*/

#include <cstdint>
#include <vector>

namespace pangea::detail {

enum class BracketKind : std::uint8_t {
    PAREN,
    BRACE,
    BRACKET,
};

class BracketStack {
public:
    BracketStack() {
        stack_.reserve(64);
    }

    void push(BracketKind kind) {
        stack_.push_back(kind);
    }

    /*
     Pop the matching opener if any. On a kind mismatch, discard
     openers until the matching kind appears or the stack drains;
     this keeps the layout policy stable across malformed input.
    */
    void close(BracketKind kind) noexcept {
        if (stack_.empty())
            return;

        if (stack_.back() == kind) {
            stack_.pop_back();
            return;
        }

        while (!stack_.empty() && stack_.back() != kind) {
            stack_.pop_back();
        }

        if (!stack_.empty())
            stack_.pop_back();
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return stack_.empty();
    }

    void clear() noexcept {
        stack_.clear();
    }

private:
    std::vector<BracketKind> stack_;
};

} // namespace pangea::detail
