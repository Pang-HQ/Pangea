#pragma once

/*
 LexerOutput is the data contract between the lexer and everything
 downstream of it (parser, formatters, diagnostics). The lexer
 writes into a caller-owned LexerOutput; consumers read from the
 same instance.

 Storage is arena-backed:
   - tokens             : TypedArena<Token>
   - trivia + trivia_attachments : pmr::vector backed by trivia_arena
                                   via trivia_resource

 LexerOutput is non-copyable and non-movable on purpose. The pmr
 containers inside hold a raw memory_resource* that must not change
 address; the same constraint that keeps StringPool pinned to its
 owner applies here. The intended ownership model is one
 LexerOutput per source file, owned by the driver and threaded by
 reference through lex and parse.

 LexerOutput is logically write-once: the lexer fills it during
 tokenise(), and consumers should treat it as read-only afterward.
*/

#include "token.h"
#include "trivia.h"

#include "../arena/arena.h"
#include "../arena/arena_resource.h"
#include "../arena/typed_arena.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace pangea {

struct LexerOutput {
    explicit LexerOutput(std::size_t buf_size)
    : tokens(buf_size / 4) {}

    LexerOutput(const LexerOutput &) = delete;
    LexerOutput &operator=(const LexerOutput &) = delete;
    LexerOutput(LexerOutput &&) = delete;
    LexerOutput &operator=(LexerOutput &&) = delete;

    // Forward-iterable, append-only token stream. Always ends with
    // SPECIAL_EOF once tokenise() has finished.
    TypedArena<Token> tokens;

    /*
     Shared arena + pmr resource backing the trivia and attachment
     vectors. One growth chain feeds both, so they pay one set of
     chunk allocations between them.
    */
    Arena trivia_arena;
    ArenaResource trivia_resource{trivia_arena};

    /*
     Trivia entries in source order. trivia_attachments index into
     this vector; consumers walk forward from a run start until the
     next attachment claims a later index, or until end().
    */
    std::pmr::vector<Trivia> trivia{&trivia_resource};

    /*
     One entry per token that has any trivia attached, sorted by
     token_index so callers can binary-search.
    */
    std::pmr::vector<TriviaAttachment> trivia_attachments{&trivia_resource};

    /*
     Return the TokenTrivia for token_index, or nullptr if the
     token has no attached trivia. Binary search; O(log n).
    */
    [[nodiscard]]
    const TokenTrivia *find_trivia(
        std::uint32_t token_index
    ) const noexcept {
        const auto it = std::lower_bound(
            trivia_attachments.begin(),
            trivia_attachments.end(),
            token_index,
            [](const TriviaAttachment &a, std::uint32_t idx) {
                return a.token_index < idx;
            });

        if (it == trivia_attachments.end()
                || it->token_index != token_index) {
            return nullptr;
        }
        return &it->trivia;
    }
};

} // namespace pangea
