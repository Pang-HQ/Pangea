#include "lexer.h"

#include "detail/core/classifiers.h"
#include "detail/scanners/scanners.h"
#include "output.h"

#include <cassert>

namespace pangea {

Lexer::Lexer(const SourceBuffer &buf,
             LexerOutput &out,
             StringPool &strings,
             Diagnostics *reporter)
    : buf_(buf),
      out_(out),
      strings_(strings),
      cursor_(buf_),
      toks_(out_.tokens),
      trivia_(out_.trivia, out_.trivia_attachments),
      emitter_(toks_,
               trivia_,
               out_.integer_literals,
               out_.float_literals,
               buf_,
               reporter) {
    /*
     Cheap pre-reserve so the trivia and attachment pmr::vectors do
     not stack up several geometric-growth buffers in the arena
     during lex. Both heuristics undershoot deliberately - if they
     are off, the vectors still grow; if they are right, we save a
     handful of arena bytes.
    */
    out_.trivia.reserve(buf.size() / 256);
    out_.trivia_attachments.reserve(buf.size() / 512);

    // SourceBuffer already clipped oversized input; report that once here.
    if (buf.oversized()) {
        const SourceRange here(static_cast<std::uint32_t>(buf.size()), 0);
        emitter_.report(DiagnosticCode::LEXER_SOURCE_TOO_LARGE, here,
                        "source file exceeds the 4 GiB lexer limit; "
                        "trailing bytes were dropped before lexing");
    }
}

void Lexer::tokenise() {
    assert(!done_ && "Lexer::tokenise() called more than once");
    done_ = true;

    while (true) {
        // Consume trivia. Line endings and comments are buffered here.
        while (true) {
            const char c = cursor_.peek();

            if (c == ' ' || c == '\t') {
                cursor_.advance();
                continue;
            }

            if (c == '\n' || c == '\r') {
                handle_line_ending();
                continue;
            }

            if (c != '/') {
                break;
            }

            const char n = cursor_.peek(1);

            if (n == '/') {
                scan_line_comment();
                continue;
            }

            if (n == '*') {
                scan_block_comment();
                continue;
            }

            break;
        }

        if (cursor_.at_end()) {
            // Trailing newlines before EOF do not separate statements.
            toks_.drop_pending_newlines();
            emit_eof();

            trivia_.finalise();

#ifndef NDEBUG
            trivia_.audit();
#endif

            assert(!toks_.empty());
            return;
        }

        toks_.flush_pending_newline(cursor_.offset());

        [[maybe_unused]] const SourceOffset before = cursor_.offset();
        scan_one();

        // Invariant: every scan_one() call must make forward progress.
        assert(cursor_.offset() > before &&
               "scan_one() made no forward progress");
    }
}

// Newline policy: see src/lexer/README.md.
void Lexer::handle_line_ending() noexcept {
    // Accept LF, CR, and CRLF. CR alone covers Mac Classic line endings.
    const char c = cursor_.peek();
    assert(c == '\n' || c == '\r');
    cursor_.advance();
    if (c == '\r' && cursor_.peek() == '\n') {
        cursor_.advance();
    }

    trivia_.on_line_ended();

    // A leading newline cannot separate statements.
    if (toks_.empty()) return;
    // Newlines inside (), [], and {} stay trivia.
    if (brackets_.empty()) {
        toks_.add_pending_newline();
    }
}

// Comment / trivia model: see src/lexer/README.md.
void Lexer::scan_line_comment() {
    const SourceOffset start = cursor_.offset();
    assert(cursor_.peek(0) == '/' && cursor_.peek(1) == '/');
    cursor_.advance();
    cursor_.advance();

    // Leave the line ending for handle_line_ending().
    while (true) {
        const char c = cursor_.peek();
        if (c == '\n' || c == '\r' || c == '\0') break;
        cursor_.advance();
    }

    trivia_.push_line_comment({start, cursor_.offset() - start});
}

void Lexer::scan_block_comment() {
    const SourceOffset start = cursor_.offset();
    assert(cursor_.peek(0) == '/' && cursor_.peek(1) == '*');
    cursor_.advance();
    cursor_.advance();

    // Check nested opens before closes so nesting is counted correctly.
    std::uint32_t depth = 1;
    while (depth > 0) {
        if (cursor_.at_end()) {
            emitter_.report(DiagnosticCode::LEXER_UNTERMINATED_BLOCK_COMMENT,
                            {start, cursor_.offset() - start},
                            "unterminated block comment");
            break;
        }

        if (cursor_.peek(0) == '/' && cursor_.peek(1) == '*') {
            cursor_.advance();
            cursor_.advance();
            ++depth;
            continue;
        }

        if (cursor_.peek(0) == '*' && cursor_.peek(1) == '/') {
            cursor_.advance();
            cursor_.advance();
            --depth;
            continue;
        }

        cursor_.advance();
    }

    trivia_.push_block_comment({start, cursor_.offset() - start});
}

void Lexer::scan_one() {
    const char c = cursor_.peek();

    // c"..." must win over the identifier path.
    if (c == 'c' && cursor_.peek(1) == '"') {
        cursor_.advance();
        detail::scan_string(detail::StringKind::C,
                            cursor_, emitter_, buf_, strings_,
                            decode_buf_);
        return;
    }

    if (detail::is_ident_start(c)) {
        detail::scan_word(cursor_, emitter_, buf_);
        return;
    }

    if (detail::is_decimal_digit(c)) {
        detail::scan_number(cursor_, emitter_, buf_);
        return;
    }

    if (c == '"') {
        detail::scan_string(detail::StringKind::PANGEA,
                            cursor_, emitter_, buf_, strings_,
                            decode_buf_);
        return;
    }

    detail::scan_symbol(cursor_, emitter_, brackets_);
}

void Lexer::emit_eof() {
    /*
     Roll any held block trivia onto the previous line's last token,
     then attach any still-pending leading trivia to the EOF sentinel
     so trailing comments before EOF are not dropped.
    */
    trivia_.on_line_ended();
    brackets_.clear();

    const detail::TokenIndex idx = toks_.emit_eof(cursor_.offset());
    trivia_.on_token_emitted(idx);
}

} // namespace pangea
