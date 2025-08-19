#pragma once

#include "../utils/source_location.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace pangea {

enum class TokenType {
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    NULL_LITERAL,
    
    // Identifiers
    IDENTIFIER,
    
    // Keywords
    FN, CLASS, STRUCT, ENUM, IF, ELSE, WHILE, FOR, IN, RETURN,
    LET, MUT, CONST, TRUE, FALSE, NULL_KW, NEW, DELETE,
    THIS, SUPER, IMPL, TRAIT, SWITCH, CASE, IMPORT, EXPORT,
    MODULE, PUB, PRIV, STATIC, VIRTUAL, OVERRIDE, ABSTRACT,
    OPERATOR, SELF, LLVM_INLINE, CAST, TRY_CAST, AS, TYPE,
    
    // Types
    I8, I16, I32, I64, U8, U16, U32, U64, F32, F64, BOOL, STRING, VOID, UNIQUE, SHARED, WEAK,
    FOREIGN, CPTR, RAW_VA_LIST,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, MULTIPLY_ASSIGN, DIVIDE_ASSIGN, MODULO_ASSIGN,
    EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,
    BITWISE_AND, BITWISE_OR, BITWISE_XOR, BITWISE_NOT,
    BITWISE_LEFT_SHIFT, BITWISE_RIGHT_SHIFT,
    INCREMENT, DECREMENT,
    POWER,
    SCOPE_RESOLUTION, MEMBER_ACCESS, ARROW,
    
    // Punctuation
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    SEMICOLON, COMMA, COLON, QUESTION,
    
    // Special
    EOF_TOKEN,
    NEWLINE,
    COMMENT
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;
    
    // For literal values
    union {
        int64_t int_value;
        double float_value;
        bool bool_value;
    };
    
    Token(TokenType t, const std::string& lex, const SourceLocation& loc)
        : type(t), lexeme(lex), location(loc), int_value(0) {}
    
    Token(TokenType t, const std::string& lex, const SourceLocation& loc, int64_t value)
        : type(t), lexeme(lex), location(loc), int_value(value) {}
    
    Token(TokenType t, const std::string& lex, const SourceLocation& loc, double value)
        : type(t), lexeme(lex), location(loc), float_value(value) {}
    
    Token(TokenType t, const std::string& lex, const SourceLocation& loc, bool value)
        : type(t), lexeme(lex), location(loc), bool_value(value) {}
    
    std::string toString() const;
};

class TokenUtils {
public:
    static std::string tokenTypeToString(TokenType type);
    static TokenType getKeywordType(const std::string& identifier);
    static bool isKeyword(const std::string& identifier);
    
private:
    static const std::unordered_map<std::string, TokenType> keywords;
};

} // namespace pangea
