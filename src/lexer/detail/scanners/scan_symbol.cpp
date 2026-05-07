#include "scanners.h"

#include "../core/bracket_stack.h"
#include "../core/cursor.h"
#include "../core/emitter.h"
#include "../../token.h"
#include "../unicode/unicode_escape.h"
#include "../../../diagnostics/diagnostic_codes.h"

#include <string>
#include <string_view>

namespace pangea::detail {

namespace {

/*
 Garbage-byte classifier used to coalesce runs of non-source bytes
 (high-bit, non-whitespace control) into a single diagnostic so a
 piped binary file does not flood the error stream. \n / \r / \t are
 excluded because they are handled by the trivia loop.
*/
bool is_garbage_byte(char c) noexcept {
    const auto u = static_cast<unsigned char>(c);
    if (u >= 0x80) {
        return true;
    }
    return u < 0x20 && u != '\n' && u != '\r' && u != '\t';
}

} // namespace

void scan_symbol(SourceCursor &cur, Emitter &emit, BracketStack &brackets) {
    const SourceOffset start = cur.offset();
    const char c = cur.peek();
    cur.advance();

    const auto here = [&]() -> SourceRange {
        return {start, cur.offset() - start};
    };

    switch (c) {
        // Single-byte punctuation

        case '(':
            brackets.push(BracketKind::PAREN);
            emit.emit(TokenType::LEFT_PAREN, here());
            return;

        case ')':
            brackets.close(BracketKind::PAREN);
            emit.emit(TokenType::RIGHT_PAREN, here());
            return;

        case '{':
            brackets.push(BracketKind::BRACE);
            emit.emit(TokenType::LEFT_BRACE, here());
            return;

        case '}':
            brackets.close(BracketKind::BRACE);
            emit.emit(TokenType::RIGHT_BRACE, here());
            return;

        case '[':
            brackets.push(BracketKind::BRACKET);
            emit.emit(TokenType::LEFT_BRACKET, here());
            return;

        case ']':
            brackets.close(BracketKind::BRACKET);
            emit.emit(TokenType::RIGHT_BRACKET, here());
            return;

        case ',':
            emit.emit(TokenType::COMMA, here());
            return;

        case ';':
            emit.emit(TokenType::SEMICOLON, here());
            return;

        case '?':
            emit.emit(TokenType::QUESTION, here());
            return;

        case '~':
            emit.emit(TokenType::OP_BITWISE_NOT, here());
            return;

        case '^':
            emit.emit(TokenType::OP_BITWISE_XOR, here());
            return;

        case '.':
            emit.emit(TokenType::OP_MEMBER_ACCESS, here());
            return;

        // Potentially multi-byte operators

        case '%':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_MODULO_ASSIGN, here());
                return;
            }
            emit.emit(TokenType::OP_MODULO, here());
            return;

        case '+':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_PLUS_ASSIGN, here());
                return;
            }
            if (cur.match('+')) {
                emit.emit(TokenType::OP_INCREMENT, here());
                return;
            }
            emit.emit(TokenType::OP_PLUS, here());
            return;

        case '-':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_MINUS_ASSIGN, here());
                return;
            }
            if (cur.match('-')) {
                emit.emit(TokenType::OP_DECREMENT, here());
                return;
            }
            if (cur.match('>')) {
                emit.emit(TokenType::OP_ARROW, here());
                return;
            }
            emit.emit(TokenType::OP_MINUS, here());
            return;

        case '*':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_MULTIPLY_ASSIGN, here());
                return;
            }
            if (cur.match('*')) {
                emit.emit(TokenType::OP_POWER, here());
                return;
            }
            emit.emit(TokenType::OP_MULTIPLY, here());
            return;

        case '/':
            // Comments are already trivia by the time we reach this branch.
            if (cur.match('=')) {
                emit.emit(TokenType::OP_DIVIDE_ASSIGN, here());
                return;
            }
            emit.emit(TokenType::OP_DIVIDE, here());
            return;

        case '!':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_NOT_EQUAL, here());
                return;
            }
            emit.emit(TokenType::OP_LOGICAL_NOT, here());
            return;

        case '=':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_EQUAL, here());
                return;
            }
            emit.emit(TokenType::OP_ASSIGN, here());
            return;

        case '<':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_LESS_EQUAL, here());
                return;
            }
            if (cur.match('<')) {
                emit.emit(TokenType::OP_BITWISE_LEFT_SHIFT, here());
                return;
            }
            emit.emit(TokenType::OP_LESS, here());
            return;

        case '>':
            if (cur.match('=')) {
                emit.emit(TokenType::OP_GREATER_EQUAL, here());
                return;
            }
            if (cur.match('>')) {
                emit.emit(TokenType::OP_BITWISE_RIGHT_SHIFT, here());
                return;
            }
            emit.emit(TokenType::OP_GREATER, here());
            return;

        case '&':
            if (cur.match('&')) {
                emit.emit(TokenType::OP_LOGICAL_AND, here());
                return;
            }
            emit.emit(TokenType::OP_BITWISE_AND, here());
            return;

        case '|':
            if (cur.match('|')) {
                emit.emit(TokenType::OP_LOGICAL_OR, here());
                return;
            }
            emit.emit(TokenType::OP_BITWISE_OR, here());
            return;

        case ':':
            if (cur.match(':')) {
                emit.emit(TokenType::OP_SCOPE_RESOLUTION, here());
                return;
            }
            emit.emit(TokenType::COLON, here());
            return;
    }

    // Coalesced garbage-byte run: one diagnostic for the whole stretch.
    if (is_garbage_byte(c)) {
        while (!cur.at_end() && is_garbage_byte(cur.peek())) {
            cur.advance();
        }
        emit.emit_error(here(),
                        DiagnosticCode::LEXER_UNEXPECTED_CHARACTER,
                        "unexpected non-source bytes in input");
        return;
    }

    /*
     Quote the stray byte with backticks and run it through escape_string
     so quotes, backslashes, and any future non-printable byte that
     escapes the classifier above still render unambiguously.
    */
    const std::string escaped = escape_string(std::string_view(&c, 1));
    emit.emit_error(here(),
                    DiagnosticCode::LEXER_UNEXPECTED_CHARACTER,
                    "unexpected character: `" + escaped + "`");
}

} // namespace pangea::detail
