#pragma once

#include "token.h"
#include "../utils/error_reporter.h"
#include <string>
#include <vector>
#include <memory>

namespace pangea {

class Lexer {
private:
    std::string source;
    std::string filename;
    size_t current = 0;
    ErrorReporter* error_reporter;

public:
    explicit Lexer(const std::string& source_code, const std::string& file = "", ErrorReporter* reporter = nullptr);
    
    std::vector<Token> tokenize();
    Token nextToken();
    
private:
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    void skipWhitespace();
    Token skipLineComment(size_t start_pos);
    Token skipBlockComment(size_t start_pos);
    
    Token makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos);
    Token makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, int64_t value);
    Token makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, double value);
    Token makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, bool value);
    Token makeTokenAtPosition(TokenType type, const std::string& lexeme, size_t start_pos, const std::string& str_value);

    Token scanString();
    Token scanNumber();
    Token scanIdentifier();
    
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    // Position-based helper methods
    size_t getLineFromPosition(size_t pos) const;
    size_t getColumnFromPosition(size_t pos) const;
    SourceLocation getLocationFromPosition(size_t pos) const;
    void reportError(const std::string& message, size_t start_position, size_t length, bool is_warning);
};

} // namespace pangea
