#include "lexer.h"
#include "../utils/unicode/unicode_escape.h"
#include <cctype>
#include <stdexcept>

namespace pangea {

Lexer::Lexer(const std::string& source_code, const std::string& file, ErrorReporter* reporter)
    : source(source_code), filename(file), error_reporter(reporter) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        Token token = nextToken();
        // Skip comment tokens - they're handled internally
        if (token.type != TokenType::COMMENT) {
            tokens.push_back(token);
        }
    }
    
    tokens.emplace_back(TokenType::EOF_TOKEN, "", getLocationFromPosition(current));
    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return makeTokenAtPosition(TokenType::EOF_TOKEN, "", current);
    }
    
    // Store the start position before advancing
    size_t start_pos = current;
    char c = advance();
    
    // Single character tokens
    switch (c) {
        case '(': return makeTokenAtPosition(TokenType::LEFT_PAREN, "(", start_pos);
        case ')': return makeTokenAtPosition(TokenType::RIGHT_PAREN, ")", start_pos);
        case '{': return makeTokenAtPosition(TokenType::LEFT_BRACE, "{", start_pos);
        case '}': return makeTokenAtPosition(TokenType::RIGHT_BRACE, "}", start_pos);
        case '[': return makeTokenAtPosition(TokenType::LEFT_BRACKET, "[", start_pos);
        case ']': return makeTokenAtPosition(TokenType::RIGHT_BRACKET, "]", start_pos);
        case ',': return makeTokenAtPosition(TokenType::COMMA, ",", start_pos);
        case ';': return makeTokenAtPosition(TokenType::SEMICOLON, ";", start_pos);
        case '?': return makeTokenAtPosition(TokenType::QUESTION, "?", start_pos);
        case '~': return makeTokenAtPosition(TokenType::BITWISE_NOT, "~", start_pos);
        case '^': return makeTokenAtPosition(TokenType::BITWISE_XOR, "^", start_pos);
        case '%':
            if (match('=')) return makeTokenAtPosition(TokenType::MODULO_ASSIGN, "%=", start_pos);
            return makeTokenAtPosition(TokenType::MODULO, "%", start_pos);
    }
    
    // Two character tokens and operators
    switch (c) {
        case '+':
            if (match('=')) return makeTokenAtPosition(TokenType::PLUS_ASSIGN, "+=", start_pos);
            if (match('+')) return makeTokenAtPosition(TokenType::INCREMENT, "++", start_pos);
            return makeTokenAtPosition(TokenType::PLUS, "+", start_pos);
        case '-':
            if (match('=')) return makeTokenAtPosition(TokenType::MINUS_ASSIGN, "-=", start_pos);
            if (match('-')) return makeTokenAtPosition(TokenType::DECREMENT, "--", start_pos);
            if (match('>')) return makeTokenAtPosition(TokenType::ARROW, "->", start_pos);
            return makeTokenAtPosition(TokenType::MINUS, "-", start_pos);
        case '*':
            if (match('=')) return makeTokenAtPosition(TokenType::MULTIPLY_ASSIGN, "*=", start_pos);
            if (match('*')) return makeTokenAtPosition(TokenType::POWER, "**", start_pos);
            return makeTokenAtPosition(TokenType::MULTIPLY, "*", start_pos);
        case '/':
            if (match('=')) return makeTokenAtPosition(TokenType::DIVIDE_ASSIGN, "/=", start_pos);
            if (match('/')) {
                return skipLineComment(start_pos);
            }
            if (match('*')) {
                return skipBlockComment(start_pos);
            }
            return makeTokenAtPosition(TokenType::DIVIDE, "/", start_pos);
        case '!':
            if (match('=')) return makeTokenAtPosition(TokenType::NOT_EQUAL, "!=", start_pos);
            return makeTokenAtPosition(TokenType::LOGICAL_NOT, "!", start_pos);
        case '=':
            if (match('=')) return makeTokenAtPosition(TokenType::EQUAL, "==", start_pos);
            return makeTokenAtPosition(TokenType::ASSIGN, "=", start_pos);
        case '<':
            if (match('=')) return makeTokenAtPosition(TokenType::LESS_EQUAL, "<=", start_pos);
            if (match('<')) return makeTokenAtPosition(TokenType::BITWISE_LEFT_SHIFT, "<<", start_pos);
            return makeTokenAtPosition(TokenType::LESS, "<", start_pos);
        case '>':
            if (match('=')) return makeTokenAtPosition(TokenType::GREATER_EQUAL, ">=", start_pos);
            if (match('>')) return makeTokenAtPosition(TokenType::BITWISE_RIGHT_SHIFT, ">>", start_pos);
            return makeTokenAtPosition(TokenType::GREATER, ">", start_pos);
        case '&':
            if (match('&')) return makeTokenAtPosition(TokenType::LOGICAL_AND, "&&", start_pos);
            return makeTokenAtPosition(TokenType::BITWISE_AND, "&", start_pos);
        case '|':
            if (match('|')) return makeTokenAtPosition(TokenType::LOGICAL_OR, "||", start_pos);
            return makeTokenAtPosition(TokenType::BITWISE_OR, "|", start_pos);
        case ':':
            if (match(':')) return makeTokenAtPosition(TokenType::SCOPE_RESOLUTION, "::", start_pos);
            return makeTokenAtPosition(TokenType::COLON, ":", start_pos);
        case '.':
            return makeTokenAtPosition(TokenType::MEMBER_ACCESS, ".", start_pos);
    }
    
    // String literals
    if (c == '"') {
        current--; // Back up to include the quote
        return scanString();
    }
    
    // Number literals
    if (isDigit(c)) {
        current--; // Back up to include the digit
        return scanNumber();
    }
    
    // Identifiers and keywords
    if (isAlpha(c)) {
        current--; // Back up to include the character
        return scanIdentifier();
    }
    
    // Newlines
    if (c == '\n') {
        return makeTokenAtPosition(TokenType::NEWLINE, "\n", start_pos);
    }
    
    // Unknown character - report error and continue
    reportError("Unexpected character: " + std::string(1, c), start_pos, 1, false);
    // Return the unknown character as an identifier to allow error recovery
    return makeTokenAtPosition(TokenType::IDENTIFIER, std::string(1, c), start_pos);
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    return source[current++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t') {
            advance();
        } else {
            break;
        }
    }
}

Token Lexer::skipLineComment(size_t start_pos) {
    // Skip until end of line
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
    
    // Create comment token with the full comment text
    std::string comment_text = source.substr(start_pos, current - start_pos);
    return makeTokenAtPosition(TokenType::COMMENT, comment_text, start_pos);
}

Token Lexer::skipBlockComment(size_t start_pos) {
    size_t nesting_level = 1;
    
    while (!isAtEnd() && nesting_level > 0) {
        if (peek() == '/' && peekNext() == '*') {
            advance(); // consume '/'
            advance(); // consume '*'
            nesting_level++;
        } else if (peek() == '*' && peekNext() == '/') {
            advance(); // consume '*'
            advance(); // consume '/'
            nesting_level--;
        } else {
            advance(); // consume any other character
        }
    }

    if (nesting_level > 0) {
        reportError("Unterminated block comment", start_pos, current - start_pos, false);
    }

    // Create comment token with the full comment text
    std::string comment_text = source.substr(start_pos, current - start_pos);
    return makeTokenAtPosition(TokenType::COMMENT, comment_text, start_pos);
}

Token Lexer::makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos) {
    SourceLocation loc = getLocationFromPosition(start_pos);
    return Token(type, lexeme, loc);
}

Token Lexer::makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, int64_t value) {
    SourceLocation loc = getLocationFromPosition(start_pos);
    return Token(type, lexeme, loc, value);
}

Token Lexer::makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, double value) {
    SourceLocation loc = getLocationFromPosition(start_pos);
    return Token(type, lexeme, loc, value);
}

Token Lexer::makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, bool value) {
    SourceLocation loc = getLocationFromPosition(start_pos);
    return Token(type, lexeme, loc, value);
}

Token Lexer::makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, const std::string& str_value) {
    SourceLocation loc = getLocationFromPosition(start_pos);
    return Token(type, lexeme, loc, str_value);
}

Token Lexer::scanString() {
    size_t start_pos = current;
    
    advance(); // consume opening quote
    
    // Collect raw string content (without quotes) to be processed by unicode_escape
    std::string raw_content;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            // Allow multi-line strings
            raw_content += advance();
        } else {
            // Just collect the raw characters, including escape sequences
            // The unicode_escape utility will handle all escaping
            raw_content += advance();
        }
    }
    
    if (isAtEnd()) {
        reportError("Unterminated string", start_pos, current - start_pos, false);
        // Return what we have so far for error recovery
        std::string partial_lexeme = source.substr(start_pos, current - start_pos);
        return makeTokenAtPosition(TokenType::STRING_LITERAL, partial_lexeme, start_pos, raw_content);
    }
    
    advance(); // consume closing quote
    
    // Create lexeme with the original source text including quotes
    std::string lexeme = source.substr(start_pos, current - start_pos);
    
    // Process the raw content using unicode_escape
    std::string processed_value;
    try {
        processed_value = escape_string(raw_content);
    } catch (const StringEscapeError& e) {
        reportError("String escape error: " + std::string(e.what()), start_pos, current - start_pos, false);

        // Use raw content as fallback
        processed_value = raw_content;
    }
    
    return makeTokenAtPosition(TokenType::STRING_LITERAL, lexeme, start_pos, processed_value);
}

Token Lexer::scanNumber() {
    size_t start_pos = current;
    
    while (isDigit(peek())) {
        advance();
    }
    
    bool is_float = false;
    if (peek() == '.' && isDigit(peekNext())) {
        is_float = true;
        advance(); // consume '.'
        while (isDigit(peek())) {
            advance();
        }
    }
    
    // Check for type suffix
    size_t number_end = current;
    if (isAlpha(peek())) {
        while (isAlphaNumeric(peek())) {
            advance();
        }
    }
    
    std::string lexeme = source.substr(start_pos, current - start_pos);
    std::string number_part = source.substr(start_pos, number_end - start_pos);
    
    try {
        if (is_float) {
            double value = std::stod(number_part);
            return makeTokenAtPosition(TokenType::FLOAT_LITERAL, lexeme, start_pos, value);
        } else {
            int64_t value = std::stoll(number_part);
            return makeTokenAtPosition(TokenType::INTEGER_LITERAL, lexeme, start_pos, value);
        }
    } catch (const std::exception& e) {
        reportError("Invalid number format: " + lexeme, start_pos, current - start_pos, false);
        return makeTokenAtPosition(TokenType::INTEGER_LITERAL, lexeme, start_pos, static_cast<int64_t>(0));
    }
}

Token Lexer::scanIdentifier() {
    size_t start_pos = current;
    
    while (isAlphaNumeric(peek())) {
        advance();
    }
    
    std::string lexeme = source.substr(start_pos, current - start_pos);
    TokenType type = TokenUtils::getKeywordType(lexeme);
    
    // Handle boolean literals with proper values
    if (type == TokenType::TRUE) {
        return makeTokenAtPosition(TokenType::BOOLEAN_LITERAL, lexeme, start_pos, true);
    } else if (type == TokenType::FALSE) {
        return makeTokenAtPosition(TokenType::BOOLEAN_LITERAL, lexeme, start_pos, false);
    } else if (type == TokenType::NULL_KW) {
        return makeTokenAtPosition(TokenType::NULL_LITERAL, lexeme, start_pos);
    }

    return makeTokenAtPosition(type, lexeme, start_pos);
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

size_t Lexer::getLineFromPosition(size_t pos) const {
    size_t line = 1;
    for (size_t i = 0; i < pos && i < source.length(); i++) {
        if (source[i] == '\n') {
            line++;
        }
    }
    return line;
}

size_t Lexer::getColumnFromPosition(size_t pos) const {
    // Find the start of the line containing this position
    size_t line_start_pos = 0;
    for (size_t i = 0; i < pos && i < source.length(); i++) {
        if (source[i] == '\n') {
            line_start_pos = i + 1;
        }
    }
    
    // Column is 1-based, so add 1 to the distance from line start
    return (pos - line_start_pos) + 1;
}

SourceLocation Lexer::getLocationFromPosition(size_t pos) const {
    size_t line = getLineFromPosition(pos);
    size_t column = getColumnFromPosition(pos);
    size_t length = 1;
    
    if (current > pos) {
        length = current - pos;
    }

    return SourceLocation(filename, line, column, pos, length);
}

void Lexer::reportError(const std::string& message, size_t start_position, size_t length = 1, bool is_warning = false) {
    if (error_reporter) {
        SourceLocation loc = getLocationFromPosition(start_position);
        loc.length = length;
        error_reporter->reportError(loc, message, is_warning);
    }
}

} // namespace pangea
