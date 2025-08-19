#include "token.h"
#include <sstream>

namespace pangea {

const std::unordered_map<std::string, TokenType> TokenUtils::keywords = {
    // Control flow
    {"fn", TokenType::FN},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"return", TokenType::RETURN},
    {"switch", TokenType::SWITCH},
    {"case", TokenType::CASE},
    
    // Declarations
    {"class", TokenType::CLASS},
    {"struct", TokenType::STRUCT},
    {"enum", TokenType::ENUM},
    {"let", TokenType::LET},
    {"mut", TokenType::MUT},
    {"const", TokenType::CONST},
    
    // Object-oriented
    {"this", TokenType::THIS},
    {"super", TokenType::SUPER},
    {"impl", TokenType::IMPL},
    {"trait", TokenType::TRAIT},
    {"virtual", TokenType::VIRTUAL},
    {"override", TokenType::OVERRIDE},
    {"abstract", TokenType::ABSTRACT},
    {"operator", TokenType::OPERATOR},
    {"self", TokenType::SELF},
    
    // Type casting
    {"cast", TokenType::CAST},
    {"try_cast", TokenType::TRY_CAST},
    {"as", TokenType::AS},
    
    // Inline LLVM
    {"__llvm_inline__", TokenType::LLVM_INLINE},
    
    // Memory management
    {"new", TokenType::NEW},
    {"delete", TokenType::DELETE},
    
    // Visibility
    {"pub", TokenType::PUB},
    {"priv", TokenType::PRIV},
    {"static", TokenType::STATIC},
    
    // Modules
    {"import", TokenType::IMPORT},
    {"export", TokenType::EXPORT},
    {"module", TokenType::MODULE},
    
    // Literals
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"null", TokenType::NULL_KW},
    
    // Types
    {"i8", TokenType::I8},
    {"i16", TokenType::I16},
    {"i32", TokenType::I32},
    {"i64", TokenType::I64},
    {"u8", TokenType::U8},
    {"u16", TokenType::U16},
    {"u32", TokenType::U32},
    {"u64", TokenType::U64},
    {"f32", TokenType::F32},
    {"f64", TokenType::F64},
    {"bool", TokenType::BOOL},
    {"string", TokenType::STRING},
    {"void", TokenType::VOID},
    {"unique", TokenType::UNIQUE},
    {"shared", TokenType::SHARED},
    {"weak", TokenType::WEAK},
    
    // Foreign function interface
    {"foreign", TokenType::FOREIGN},
    {"cptr", TokenType::CPTR},
    {"raw_va_list", TokenType::RAW_VA_LIST},
    {"type", TokenType::TYPE}
};

std::string Token::toString() const {
    std::ostringstream oss;
    oss << TokenUtils::tokenTypeToString(type) << " '" << lexeme << "' at " << location.toString();
    return oss.str();
}

std::string TokenUtils::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        case TokenType::NULL_LITERAL: return "NULL_LITERAL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        
        // Keywords
        case TokenType::FN: return "FN";
        case TokenType::CLASS: return "CLASS";
        case TokenType::STRUCT: return "STRUCT";
        case TokenType::ENUM: return "ENUM";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FOR: return "FOR";
        case TokenType::IN: return "IN";
        case TokenType::RETURN: return "RETURN";
        case TokenType::LET: return "LET";
        case TokenType::MUT: return "MUT";
        case TokenType::CONST: return "CONST";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::NULL_KW: return "NULL";
        case TokenType::NEW: return "NEW";
        case TokenType::DELETE: return "DELETE";
        case TokenType::THIS: return "THIS";
        case TokenType::SUPER: return "SUPER";
        case TokenType::IMPL: return "IMPL";
        case TokenType::TRAIT: return "TRAIT";
        case TokenType::SWITCH: return "MATCH";
        case TokenType::CASE: return "CASE";
        case TokenType::IMPORT: return "IMPORT";
        case TokenType::EXPORT: return "EXPORT";
        case TokenType::MODULE: return "MODULE";
        case TokenType::PUB: return "PUB";
        case TokenType::PRIV: return "PRIV";
        case TokenType::STATIC: return "STATIC";
        case TokenType::VIRTUAL: return "VIRTUAL";
        case TokenType::OVERRIDE: return "OVERRIDE";
        case TokenType::ABSTRACT: return "ABSTRACT";
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::SELF: return "SELF";
        case TokenType::LLVM_INLINE: return "LLVM_INLINE";
        case TokenType::CAST: return "CAST";
        case TokenType::TRY_CAST: return "TRY_CAST";
        case TokenType::AS: return "AS";
        case TokenType::TYPE: return "TYPE";
        
        // Types
        case TokenType::I8: return "I8";
        case TokenType::I16: return "I16";
        case TokenType::I32: return "I32";
        case TokenType::I64: return "I64";
        case TokenType::U8: return "U8";
        case TokenType::U16: return "U16";
        case TokenType::U32: return "U32";
        case TokenType::U64: return "U64";
        case TokenType::F32: return "F32";
        case TokenType::F64: return "F64";
        case TokenType::BOOL: return "BOOL";
        case TokenType::STRING: return "STRING";
        case TokenType::VOID: return "VOID";
        case TokenType::UNIQUE: return "UNIQUE";
        case TokenType::SHARED: return "SHARED";
        case TokenType::WEAK: return "WEAK";
        case TokenType::FOREIGN: return "FOREIGN";
        case TokenType::CPTR: return "CPTR";
        case TokenType::RAW_VA_LIST: return "RAW_VA_LIST";
        
        // Operators
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::MODULO: return "MODULO";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TokenType::MULTIPLY_ASSIGN: return "MULTIPLY_ASSIGN";
        case TokenType::DIVIDE_ASSIGN: return "DIVIDE_ASSIGN";
        case TokenType::MODULO_ASSIGN: return "MODULO_ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER: return "GREATER";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LOGICAL_AND: return "LOGICAL_AND";
        case TokenType::LOGICAL_OR: return "LOGICAL_OR";
        case TokenType::LOGICAL_NOT: return "LOGICAL_NOT";
        case TokenType::BITWISE_AND: return "BITWISE_AND";
        case TokenType::BITWISE_OR: return "BITWISE_OR";
        case TokenType::BITWISE_XOR: return "BITWISE_XOR";
        case TokenType::BITWISE_NOT: return "BITWISE_NOT";
        case TokenType::BITWISE_LEFT_SHIFT: return "BITWISE_LEFT_SHIFT";
        case TokenType::BITWISE_RIGHT_SHIFT: return "BITWISE_RIGHT_SHIFT";
        case TokenType::INCREMENT: return "INCREMENT";
        case TokenType::DECREMENT: return "DECREMENT";
        case TokenType::POWER: return "POWER";
        case TokenType::SCOPE_RESOLUTION: return "SCOPE_RESOLUTION";
        case TokenType::MEMBER_ACCESS: return "MEMBER_ACCESS";
        case TokenType::ARROW: return "ARROW";
        
        // Punctuation
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::LEFT_BRACE: return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
        case TokenType::LEFT_BRACKET: return "LEFT_BRACKET";
        case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::COLON: return "COLON";
        case TokenType::QUESTION: return "QUESTION";
        
        // Special
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::COMMENT: return "COMMENT";
        
        default: return "UNKNOWN";
    }
}

TokenType TokenUtils::getKeywordType(const std::string& identifier) {
    auto it = keywords.find(identifier);
    return (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
}

bool TokenUtils::isKeyword(const std::string& identifier) {
    return keywords.find(identifier) != keywords.end();
}

} // namespace pangea
