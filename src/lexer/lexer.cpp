#include "lexer.h"
#include <cctype>
#include <stdexcept>

// NOTE: Character position tracking in the lexer is somewhat buggy,
// this is a known issue and will be fixed in a future release.

namespace pangea {

Lexer::Lexer(const std::string& source_code, const std::string& file, ErrorReporter* reporter)
    : source(source_code), filename(file), error_reporter(reporter) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        tokens.push_back(nextToken());
    }
    
    tokens.emplace_back(TokenType::EOF_TOKEN, "", getCurrentLocation());
    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespace();
    
    if (isAtEnd()) {
        return makeToken(TokenType::EOF_TOKEN);
    }
    
    // Store the start position and location before advancing
    size_t start = current;
    size_t start_column = column;
    char c = advance();
    
    // Single character tokens
    switch (c) {
        case '(': return makeTokenAtStart(TokenType::LEFT_PAREN, "(", start_column);
        case ')': return makeTokenAtStart(TokenType::RIGHT_PAREN, ")", start_column);
        case '{': return makeTokenAtStart(TokenType::LEFT_BRACE, "{", start_column);
        case '}': return makeTokenAtStart(TokenType::RIGHT_BRACE, "}", start_column);
        case '[': return makeTokenAtStart(TokenType::LEFT_BRACKET, "[", start_column);
        case ']': return makeTokenAtStart(TokenType::RIGHT_BRACKET, "]", start_column);
        case ',': return makeTokenAtStart(TokenType::COMMA, ",", start_column);
        case ';': return makeTokenAtStart(TokenType::SEMICOLON, ";", start_column);
        case '?': return makeTokenAtStart(TokenType::QUESTION, "?", start_column);
        case '~': return makeTokenAtStart(TokenType::BITWISE_NOT, "~", start_column);
        case '^': return makeTokenAtStart(TokenType::BITWISE_XOR, "^", start_column);
        case '%':
            if (match('=')) return makeTokenAtStart(TokenType::MODULO_ASSIGN, "%=", start_column);
            return makeTokenAtStart(TokenType::MODULO, "%", start_column);
    }
    
    // Two character tokens and operators
    switch (c) {
        case '+':
            if (match('=')) return makeTokenAtStart(TokenType::PLUS_ASSIGN, "+=", start_column);
            if (match('+')) return makeTokenAtStart(TokenType::INCREMENT, "++", start_column);
            return makeTokenAtStart(TokenType::PLUS, "+", start_column);
        case '-':
            if (match('=')) return makeTokenAtStart(TokenType::MINUS_ASSIGN, "-=", start_column);
            if (match('-')) return makeTokenAtStart(TokenType::DECREMENT, "--", start_column);
            if (match('>')) return makeTokenAtStart(TokenType::ARROW, "->", start_column);
            return makeTokenAtStart(TokenType::MINUS, "-", start_column);
        case '*':
            if (match('=')) return makeTokenAtStart(TokenType::MULTIPLY_ASSIGN, "*=", start_column);
            if (match('*')) return makeTokenAtStart(TokenType::POWER, "**", start_column);
            return makeTokenAtStart(TokenType::MULTIPLY, "*", start_column);
        case '/':
            if (match('=')) return makeTokenAtStart(TokenType::DIVIDE_ASSIGN, "/=", start_column);
            if (match('/')) {
                return skipLineComment();
            }
            if (match('*')) {
                return skipBlockComment();
            }
            return makeTokenAtStart(TokenType::DIVIDE, "/", start_column);
        case '!':
            if (match('=')) return makeTokenAtStart(TokenType::NOT_EQUAL, "!=", start_column);
            return makeTokenAtStart(TokenType::LOGICAL_NOT, "!", start_column);
        case '=':
            if (match('=')) return makeTokenAtStart(TokenType::EQUAL, "==", start_column);
            return makeTokenAtStart(TokenType::ASSIGN, "=", start_column);
        case '<':
            if (match('=')) return makeTokenAtStart(TokenType::LESS_EQUAL, "<=", start_column);
            if (match('<')) return makeTokenAtStart(TokenType::BITWISE_LEFT_SHIFT, "<<", start_column);
            return makeTokenAtStart(TokenType::LESS, "<", start_column);
        case '>':
            if (match('=')) return makeTokenAtStart(TokenType::GREATER_EQUAL, ">=", start_column);
            if (match('>')) return makeTokenAtStart(TokenType::BITWISE_RIGHT_SHIFT, ">>", start_column);
            return makeTokenAtStart(TokenType::GREATER, ">", start_column);
        case '&':
            if (match('&')) return makeTokenAtStart(TokenType::LOGICAL_AND, "&&", start_column);
            return makeTokenAtStart(TokenType::BITWISE_AND, "&", start_column);
        case '|':
            if (match('|')) return makeTokenAtStart(TokenType::LOGICAL_OR, "||", start_column);
            return makeTokenAtStart(TokenType::BITWISE_OR, "|", start_column);
        case ':':
            if (match(':')) return makeTokenAtStart(TokenType::SCOPE_RESOLUTION, "::", start_column);
            return makeTokenAtStart(TokenType::COLON, ":", start_column);
        case '.':
            return makeTokenAtStart(TokenType::MEMBER_ACCESS, ".", start_column);
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
        line++;
        column = 1;
        line_start = current;
        return makeToken(TokenType::NEWLINE, "\\n"); 
    }
    
    // Unknown character
    reportError("Unexpected character: " + std::string(1, c));
    return makeToken(TokenType::EOF_TOKEN);
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    column++;
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
    column++;
    return true;
}

SourceLocation Lexer::getCurrentLocation() const {
    return SourceLocation(filename, line, column, current);
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

Token Lexer::skipLineComment() {
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
    // Skip the comment and get the next token
    return nextToken();
}

Token Lexer::skipBlockComment() {
    while (!isAtEnd()) {
        if (peek() == '*' && peekNext() == '/') {
            advance(); // consume '*'
            advance(); // consume '/'
            break;
        }
        if (peek() == '\n') {
            line++;
            column = 1;
            line_start = current + 1;
        }
        advance();
    }
    // Skip the comment and get the next token
    return nextToken();
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
    return Token(type, lexeme, getCurrentLocation());
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme, int64_t value) {
    return Token(type, lexeme, getCurrentLocation(), value);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme, double value) {
    return Token(type, lexeme, getCurrentLocation(), value);
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme, bool value) {
    return Token(type, lexeme, getCurrentLocation(), value);
}

Token Lexer::makeTokenAtStart(TokenType type, const std::string& lexeme, size_t start_column) {
    // Calculate the actual start position in the source
    size_t start_pos = current - lexeme.length();
    SourceLocation loc(filename, line, start_column, start_pos);
    return Token(type, lexeme, loc);
}

Token Lexer::scanString() {
    size_t start = current;
    advance(); // consume opening quote
    
    std::string value;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            column = 1;
            line_start = current + 1;
        }
        
        if (peek() == '\\') {
            advance(); // consume backslash
            if (isAtEnd()) break;
            
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '0': value += '\0'; break;
                default:
                    reportError("Unknown escape sequence: \\" + std::string(1, escaped));
                    value += escaped;
                    break;
            }
        } else {
            value += advance();
        }
    }
    
    if (isAtEnd()) {
        reportError("Unterminated string");
        return makeToken(TokenType::EOF_TOKEN);
    }
    
    advance(); // consume closing quote
    
    // Use the processed string value as the lexeme instead of the raw source
    return makeToken(TokenType::STRING_LITERAL, value);
}

Token Lexer::scanNumber() {
    size_t start = current;
    
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
    std::string suffix;
    size_t suffix_start = current;
    if (isAlpha(peek())) {
        while (isAlphaNumeric(peek())) {
            advance();
        }
        suffix = source.substr(suffix_start, current - suffix_start);
    }
    
    std::string number_part = source.substr(start, suffix_start - start);
    std::string full_lexeme = source.substr(start, current - start);
    
    if (is_float) {
        double value = std::stod(number_part);
        return makeToken(TokenType::FLOAT_LITERAL, full_lexeme, value);
    } else {
        int64_t value = std::stoll(number_part);
        return makeToken(TokenType::INTEGER_LITERAL, full_lexeme, value);
    }
}

Token Lexer::scanIdentifier() {
    size_t start = current;
    
    while (isAlphaNumeric(peek())) {
        advance();
    }
    
    std::string lexeme = source.substr(start, current - start);
    TokenType type = TokenUtils::getKeywordType(lexeme);
    
    // Handle boolean literals
    if (type == TokenType::TRUE) {
        return makeToken(TokenType::BOOLEAN_LITERAL, lexeme, true);
    } else if (type == TokenType::FALSE) {
        return makeToken(TokenType::BOOLEAN_LITERAL, lexeme, false);
    } else if (type == TokenType::NULL_KW) {
        return makeToken(TokenType::NULL_LITERAL, lexeme);
    }
    
    return makeToken(type, lexeme);
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

void Lexer::reportError(const std::string& message) {
    if (error_reporter) {
        error_reporter->reportError(getCurrentLocation(), message);
    }
}

} // namespace pangea
