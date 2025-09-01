#pragma once

#include "../lexer/token.h"
#include "../ast/ast_nodes.h"
#include "../utils/error_reporter.h"
#include <vector>
#include <memory>

namespace pangea {

class Parser {
private:
    std::vector<Token> tokens;
    size_t current = 0;
    ErrorReporter* error_reporter;

public:
    explicit Parser(std::vector<Token> token_list, ErrorReporter* reporter = nullptr);
    
    std::unique_ptr<Program> parseProgram();
    
private:
    // Utility methods
    bool isAtEnd() const;
    void skipNewlines();
    void consumeOptionalSemicolon();
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    void synchronize();
    void synchronizeStatement();
    void reportError(const std::string& message);
    
    // Parsing methods
    std::unique_ptr<Declaration> parseDeclaration();
    std::unique_ptr<FunctionDeclaration> parseFunctionDeclaration();
    std::unique_ptr<FunctionDeclaration> parseForeignFunctionDeclaration();
    std::unique_ptr<VariableDeclaration> parseVariableDeclaration(bool is_mutable = false);
    std::unique_ptr<VariableDeclaration> parseForeignVariableDeclaration(bool is_mutable = false);
    std::unique_ptr<VariableDeclaration> parseTypeAlias();
    std::unique_ptr<ClassDeclaration> parseClassDeclaration();
    std::unique_ptr<StructDeclaration> parseStructDeclaration();
    std::unique_ptr<StructDeclaration> parseForeignStructDeclaration();
    std::unique_ptr<EnumDeclaration> parseEnumDeclaration();
    std::unique_ptr<EnumDeclaration> parseForeignEnumDeclaration();
    std::unique_ptr<ImportDeclaration> parseImportDeclaration();
    
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<BlockStatement> parseBlockStatement();
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<WhileStatement> parseWhileStatement();
    std::unique_ptr<ForStatement> parseForStatement();
    std::unique_ptr<ReturnStatement> parseReturnStatement();
    std::unique_ptr<ExpressionStatement> parseExpressionStatement();
    
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseAssignment();
    std::unique_ptr<Expression> parseAsExpression();
    std::unique_ptr<Expression> parseLogicalOr();
    std::unique_ptr<Expression> parseLogicalAnd();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseBitwiseShift();
    std::unique_ptr<Expression> parseTerm();
    std::unique_ptr<Expression> parseFactor();
    std::unique_ptr<Expression> parsePower();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parseCall();
    std::unique_ptr<Expression> parsePrimary();
    
    std::unique_ptr<Type> parseType();
    std::unique_ptr<Type> parsePrimitiveType();
    std::unique_ptr<Type> parsePointerType();
    
    std::vector<Parameter> parseParameterList();
    Parameter parseParameter();
    std::vector<std::unique_ptr<Expression>> parseArgumentList();
};

} // namespace pangea
