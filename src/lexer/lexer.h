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
    size_t line = 1;
    size_t column = 1;
    size_t line_start = 0;
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
    
    SourceLocation getCurrentLocation() const;
    void skipWhitespace();
    Token skipLineComment();
    Token skipBlockComment();
    
    Token makeToken(TokenType type, const std::string& lexeme = "");
    Token makeToken(TokenType type, const std::string& lexeme, int64_t value);
    Token makeToken(TokenType type, const std::string& lexeme, double value);
    Token makeToken(TokenType type, const std::string& lexeme, bool value);
    Token makeTokenAtStart(TokenType type, const std::string& lexeme, size_t start_column);
    
    Token scanString();
    Token scanNumber();
    Token scanIdentifier();
    
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    void reportError(const std::string& message);
};

} // namespace pangea
