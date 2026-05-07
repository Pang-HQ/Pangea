#include "scanners.h"

#include "../core/cursor.h"
#include "../core/emitter.h"
#include "../../token.h"
#include "../unicode/unicode_escape.h"
#include "../../../diagnostics/diagnostic_codes.h"
#include "../../../source/source_buffer.h"
#include "../../../source/string_pool.h"

#include <cassert>
#include <string>
#include <string_view>

namespace pangea::detail {

namespace {

/*
 Recovery sub-loop entered after an embedded NUL is seen inside a
 string literal. Consumes bytes up to either the closing quote, a
 line terminator, or EOF, treating backslash-escapes as opaque pairs
 along the way. Returns true if the closing quote was consumed.
*/
bool recover_after_embedded_nul(SourceCursor &cur) {
    while (true) {
        const char next = cur.peek();
        if (next == '"') {
            cur.advance();
            return true;
        }
        if (next == '\n' || next == '\r' ||
            (next == '\0' && cur.at_end())) {
            return false;
        }
        if (next == '\\') {
            cur.advance();
            if (cur.peek() == '\0' && cur.at_end()) {
                return false;
            }
            cur.advance();
            continue;
        }
        cur.advance();
    }
}

} // namespace

void scan_string(StringKind kind,
                 SourceCursor &cur,
                 Emitter &emit,
                 const SourceBuffer &buf,
                 StringPool &strings,
                 std::string &decode_buf) {
    assert(cur.peek() == '"');

    const SourceLength prefix_len = (kind == StringKind::C) ? 1u : 0u;
    const SourceOffset start = cur.offset() - prefix_len;

    cur.advance();  // opening quote
    const SourceOffset raw_begin = cur.offset();

    while (true) {
        const char c = cur.peek();
        if (c == '"') break;

        if (c == '\n' || c == '\r' || (c == '\0' && cur.at_end())) {
            emit.emit_error({start, cur.offset() - start},
                            DiagnosticCode::LEXER_UNTERMINATED_STRING,
                            "unterminated string literal");
            return;
        }

        if (c == '\0') {
            cur.advance();
            const bool terminated = recover_after_embedded_nul(cur);
            const char *message = terminated
                ? "embedded NUL byte in string literal"
                : "unterminated string literal with embedded NUL byte";
            emit.emit_error({start, cur.offset() - start},
                            DiagnosticCode::LEXER_EMBEDDED_NUL,
                            message);
            return;
        }

        if (c == '\\') {
            cur.advance();
            const char escaped = cur.peek();

            if (escaped == '\0' && cur.at_end()) {
                emit.emit_error(
                    {start, cur.offset() - start},
                    DiagnosticCode::LEXER_UNTERMINATED_STRING,
                    "unterminated string literal (trailing backslash)");
                return;
            }

            /*
             Pangea strings are single-line; bail on '\<newline>' here
             rather than letting the escape pass forward and swallow
             the rest of the source to the next unrelated quote.
            */
            if (escaped == '\n' || escaped == '\r') {
                emit.emit_error(
                    {start, cur.offset() - start},
                    DiagnosticCode::LEXER_UNTERMINATED_STRING,
                    "unterminated string literal "
                    "(line continuations are not supported)");
                return;
            }

            cur.advance();
            continue;
        }

        cur.advance();
    }

    const SourceOffset raw_end = cur.offset();
    cur.advance();  // closing quote

    const std::string_view raw = buf.slice(raw_begin, raw_end - raw_begin);

    if (const auto err = unescape_string(raw, decode_buf)) {
        std::string msg;
        msg.reserve(128);
        msg += "string escape error: ";
        msg += message_for(*err);

        emit.emit_error({start, cur.offset() - start},
                DiagnosticCode::LEXER_STRING_ESCAPE_ERROR,
                std::move(msg));
        return;
    }

    const SymbolID id = (kind == StringKind::C)
        ? strings.intern_with_nul(decode_buf)
        : strings.intern(decode_buf);

    if (!id.is_valid()) {
        assert(strings.overflowed());
        emit.emit_error({start, cur.offset() - start},
                        DiagnosticCode::LEXER_STRING_POOL_EXHAUSTED,
                        "string literal exhausted the pool; "
                        "source is too large to compile");
        return;
    }

    const TokenType type = (kind == StringKind::C)
        ? TokenType::LITERAL_C_STRING
        : TokenType::LITERAL_STRING;
    emit.emit(type, {start, cur.offset() - start}, id);
}

} // namespace pangea::detail
