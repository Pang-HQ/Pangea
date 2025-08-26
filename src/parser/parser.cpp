#include "parser.h"
#include <stdexcept>

namespace pangea {

Parser::Parser(std::vector<Token> token_list, ErrorReporter* reporter)
    : tokens(std::move(token_list)), error_reporter(reporter) {}

std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>(SourceLocation());
    
    // For now, create a single main module containing all declarations
    auto main_module = std::make_unique<Module>(SourceLocation(), "main", "main.pang");
    
    while (!isAtEnd()) {
        skipNewlines();
        if (auto decl = parseDeclaration()) {
            if (auto import_decl = dynamic_cast<ImportDeclaration*>(decl.get())) {
                // Move import to module's import list
                main_module->imports.push_back(std::unique_ptr<ImportDeclaration>(
                    static_cast<ImportDeclaration*>(decl.release())
                ));
            } else {
                // Regular declaration goes to module's declarations
                main_module->declarations.push_back(std::move(decl));
            }
        } else {
            // If parsing failed, synchronize should already have skipped to the next statement
            // so no need to do anything here :D
        }
    }
    
    program->main_module = std::move(main_module);
    return program;
}

// Utility methods
bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE)) {
        advance();
    }
}

void Parser::consumeOptionalSemicolon() {
    if (check(TokenType::SEMICOLON)) {
        advance();
        // Check for extra semicolons (common error)
        while (check(TokenType::SEMICOLON)) {
            reportError("Unexpected extra semicolon");
            advance();
        }
    } else if (check(TokenType::NEWLINE) || check(TokenType::RIGHT_BRACE) || isAtEnd()) {
        // Newline, closing brace, or EOF acts as statement terminator
        return;
    } else {
        reportError("Expected ';' or newline after statement");
        throw std::runtime_error("Parse error: Expected statement terminator");
    }
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    reportError(message);
    throw std::runtime_error("Parse error: " + message);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type) {
            case TokenType::CLASS:
            case TokenType::FN:
            case TokenType::LET:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
            case TokenType::CONST:
            case TokenType::IMPORT:
            case TokenType::STRUCT:
            case TokenType::ENUM:
            case TokenType::FOREIGN:
            case TokenType::TYPE:
                return;
            default:
                break;
        }
        
        advance();
    }
}

void Parser::synchronizeStatement() {
    // Skip tokens until we find a statement boundary within a function body
    while (!isAtEnd() && !check(TokenType::RIGHT_BRACE)) {
        // If we find a semicolon, we can start parsing the next statement
        if (check(TokenType::SEMICOLON)) {
            advance(); // consume the semicolon
            return;
        }
        
        // If we find a newline, that's also a statement boundary
        if (check(TokenType::NEWLINE)) {
            return; // don't consume the newline, let skipNewlines handle it
        }
        
        // If we find tokens that typically start statements, stop here
        switch (peek().type) {
            case TokenType::LET:
            case TokenType::CONST:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::RETURN:
            case TokenType::LEFT_BRACE:
                return;
            default:
                break;
        }
        
        advance();
    }
}

void Parser::reportError(const std::string& message) {
    if (error_reporter) {
        error_reporter->reportError(peek().location, message + " " + peek().toString());
    }
}

// Declaration parsing
std::unique_ptr<Declaration> Parser::parseDeclaration() {
    try {
        skipNewlines();
        
        // If we've reached EOF after skipping newlines, that's fine
        if (isAtEnd()) {
            return nullptr;
        }
        
        // Handle export declarations
        if (match({TokenType::EXPORT})) {
            // Parse the exported declaration
            auto exported_decl = parseDeclaration();
            // For now, just return the declaration (export is handled at module level)
            return exported_decl;
        }
        
        // Handle foreign declarations
        if (match({TokenType::FOREIGN})) {
            if (match({TokenType::FN})) {
                return parseForeignFunctionDeclaration();
            }
            if (match({TokenType::STRUCT})) {
                return parseForeignStructDeclaration();
            }
            if (match({TokenType::ENUM})) {
                return parseForeignEnumDeclaration();
            }
            if (match({TokenType::CLASS})) {
                reportError("Foreign classes are not supported - C standard library has no classes");
                return nullptr;
            }
            if (match({TokenType::CONST})) {
                return parseForeignConstDeclaration();
            }
            reportError("Expected 'fn', 'struct', 'enum', or 'const' after 'foreign'");
            return nullptr;
        }
        
        // Handle type aliases
        if (match({TokenType::TYPE})) {
            return parseTypeAlias();
        }
        
        if (match({TokenType::FN})) {
            return parseFunctionDeclaration();
        }
        
        if (match({TokenType::CLASS})) {
            return parseClassDeclaration();
        }
        
        if (match({TokenType::STRUCT})) {
            return parseStructDeclaration();
        }
        
        if (match({TokenType::ENUM})) {
            return parseEnumDeclaration();
        }
        
        if (match({TokenType::IMPORT})) {
            return parseImportDeclaration();
        }
        
        if (match({TokenType::LET})) {
            if (match({TokenType::MUT})) {
                return parseVariableDeclaration(true); // mutable variable
            }
            return parseVariableDeclaration(false); // let variables are immutable
        }
        
        if (match({TokenType::CONST})) {
            return parseConstDeclaration();
        }
        
        reportError("Expected declaration");
        return nullptr;
    } catch (const std::runtime_error&) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    std::vector<Parameter> parameters = parseParameterList();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");

    std::unique_ptr<Type> return_type;
    if (!match({TokenType::ARROW}))
    {
        error_reporter->reportError(previous().location, "Function return type inference not yet implemented, defaulting to void.", true);
        return_type = std::make_unique<PrimitiveType>(previous().location, TokenType::VOID);
    }
    else
    {
        return_type = parseType();
    }
    
    while (match({TokenType::NEWLINE}));

    consume(TokenType::LEFT_BRACE, "Expected '{' before function body");
    auto body = parseBlockStatement();
    
    return std::make_unique<FunctionDeclaration>(
        name.location, name.lexeme, std::move(parameters), 
        std::move(return_type), std::move(body)
    );
}

std::unique_ptr<VariableDeclaration> Parser::parseVariableDeclaration(bool is_mutable) {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");

    std::unique_ptr<Type> type = nullptr;
    if (match({TokenType::COLON})) {
        type = parseType();
    }
    
    std::unique_ptr<Expression> initializer = nullptr;
    if (match({TokenType::ASSIGN})) {
        initializer = parseExpression();
    }

    consumeOptionalSemicolon();
    
    return std::make_unique<VariableDeclaration>(
        name.location, name.lexeme, std::move(type), 
        std::move(initializer), is_mutable
    );
}

// Statement parsing
std::unique_ptr<Statement> Parser::parseStatement() {
    skipNewlines();
    if (match({TokenType::IF})) return parseIfStatement();
    if (match({TokenType::WHILE})) return parseWhileStatement();
    if (match({TokenType::FOR})) return parseForStatement();
    if (match({TokenType::RETURN})) return parseReturnStatement();
    if (match({TokenType::LEFT_BRACE})) return parseBlockStatement();
    
    return parseExpressionStatement();
}

std::unique_ptr<BlockStatement> Parser::parseBlockStatement() {
    auto block = std::make_unique<BlockStatement>(previous().location);
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RIGHT_BRACE) || isAtEnd()) break;
        
        try {
            std::unique_ptr<Statement> stmt = nullptr;
            
            // Check if it's a variable declaration
            if (check(TokenType::LET) || check(TokenType::CONST)) {
                if (auto decl = parseDeclaration()) {
                    // Wrap declaration in DeclarationStatement
                    stmt = std::make_unique<DeclarationStatement>(decl->location, std::move(decl));
                }
            } else {
                stmt = parseStatement();
            }
            
            if (stmt) {
                block->statements.push_back(std::move(stmt));
            }
        } catch (const std::runtime_error&) {
            // Error recovery: synchronize to next statement boundary
            synchronizeStatement();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
    return block;
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    auto condition = parseExpression();
    auto then_branch = parseStatement();
    
    std::unique_ptr<Statement> else_branch = nullptr;
    if (match({TokenType::ELSE})) {
        else_branch = parseStatement();
    }
    
    return std::make_unique<IfStatement>(
        previous().location, std::move(condition), 
        std::move(then_branch), std::move(else_branch)
    );
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement() {
    auto condition = parseExpression();
    auto body = parseStatement();
    
    return std::make_unique<WhileStatement>(
        previous().location, std::move(condition), std::move(body)
    );
}

std::unique_ptr<ForStatement> Parser::parseForStatement() {
    Token iterator = consume(TokenType::IDENTIFIER, "Expected iterator name");
    consume(TokenType::IN, "Expected 'in' after iterator");
    auto iterable = parseExpression();
    auto body = parseStatement();
    
    return std::make_unique<ForStatement>(
        iterator.location, iterator.lexeme, 
        std::move(iterable), std::move(body)
    );
}

std::unique_ptr<ReturnStatement> Parser::parseReturnStatement() {
    SourceLocation location = previous().location;
    
    std::unique_ptr<Expression> value = nullptr;
    if (!check(TokenType::SEMICOLON) && !check(TokenType::NEWLINE) && !check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        value = parseExpression();
    }
    
    consumeOptionalSemicolon();
    return std::make_unique<ReturnStatement>(location, std::move(value));
}

std::unique_ptr<ExpressionStatement> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    consumeOptionalSemicolon();
    return std::make_unique<ExpressionStatement>(expr->location, std::move(expr));
}

// Expression parsing (operator precedence)
std::unique_ptr<Expression> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expression> Parser::parseAssignment() {
    auto expr = parseAsExpression();
    
    if (match({TokenType::ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN, 
               TokenType::MULTIPLY_ASSIGN, TokenType::DIVIDE_ASSIGN, TokenType::MODULO_ASSIGN})) {
        TokenType operator_token = previous().type;
        auto right = parseAssignment(); // Right associative
        return std::make_unique<AssignmentExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseAsExpression() {
    auto expr = parseLogicalOr();
    
    while (match({TokenType::AS})) {
        auto target_type = parseType();
        expr = std::make_unique<AsExpression>(
            expr->location, std::move(expr), std::move(target_type)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    
    while (match({TokenType::LOGICAL_OR})) {
        TokenType operator_token = previous().type;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    
    while (match({TokenType::LOGICAL_AND})) {
        TokenType operator_token = previous().type;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto expr = parseComparison();
    
    while (match({TokenType::NOT_EQUAL, TokenType::EQUAL})) {
        TokenType operator_token = previous().type;
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseBitwiseShift();
    
    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
        TokenType operator_token = previous().type;
        auto right = parseBitwiseShift();
        expr = std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseBitwiseShift() {
    auto expr = parseTerm();
    
    while (match({TokenType::BITWISE_LEFT_SHIFT, TokenType::BITWISE_RIGHT_SHIFT})) {
        TokenType operator_token = previous().type;
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseTerm() {
    auto expr = parseFactor();
    
    while (match({TokenType::MINUS, TokenType::PLUS})) {
        TokenType operator_token = previous().type;
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseFactor() {
    auto expr = parsePower();
    
    while (match({TokenType::DIVIDE, TokenType::MULTIPLY, TokenType::MODULO})) {
        TokenType operator_token = previous().type;
        auto right = parsePower();
        expr = std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parsePower() {
    auto expr = parseUnary();
    
    // Power is right associative
    if (match({TokenType::POWER})) {
        TokenType operator_token = previous().type;
        auto right = parsePower(); // Right associative
        return std::make_unique<BinaryExpression>(
            expr->location, std::move(expr), operator_token, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match({TokenType::LOGICAL_NOT, TokenType::MINUS})) {
        TokenType operator_token = previous().type;
        auto right = parseUnary();
        return std::make_unique<UnaryExpression>(
            previous().location, operator_token, std::move(right)
        );
    }
    
    return parseCall();
}

std::unique_ptr<Expression> Parser::parseCall() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match({TokenType::LEFT_PAREN})) {
            auto arguments = parseArgumentList();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
            expr = std::make_unique<CallExpression>(
                expr->location, std::move(expr), std::move(arguments)
            );
        } else if (match({TokenType::MEMBER_ACCESS})) {
            Token name = consume(TokenType::IDENTIFIER, "Expected property name after '.'");
            expr = std::make_unique<MemberExpression>(
                expr->location, std::move(expr), name.lexeme
            );
        } else if (match({TokenType::LEFT_BRACKET})) {
            auto index = parseExpression();
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after index");
            expr = std::make_unique<IndexExpression>(
                expr->location, std::move(expr), std::move(index)
            );
        } else if (match({TokenType::INCREMENT, TokenType::DECREMENT})) {
            TokenType operator_token = previous().type;
            expr = std::make_unique<PostfixExpression>(
                expr->location, std::move(expr), operator_token
            );
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    // Handle cast<T>(x) and try_cast<T>(x)
    if (match({TokenType::CAST, TokenType::TRY_CAST})) {
        bool is_safe_cast = previous().type == TokenType::TRY_CAST;
        SourceLocation location = previous().location;
        
        consume(TokenType::LESS, "Expected '<' after cast");
        auto target_type = parseType();
        consume(TokenType::GREATER, "Expected '>' after cast type");
        consume(TokenType::LEFT_PAREN, "Expected '(' after cast<T>");
        auto expression = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after cast expression");
        
        return std::make_unique<CastExpression>(location, std::move(target_type), std::move(expression), is_safe_cast);
    }
    
    if (match({TokenType::BOOLEAN_LITERAL})) {
        return std::make_unique<LiteralExpression>(previous().location, previous());
    }
    
    if (match({TokenType::NULL_LITERAL})) {
        return std::make_unique<LiteralExpression>(previous().location, previous());
    }
    
    if (match({TokenType::INTEGER_LITERAL, TokenType::FLOAT_LITERAL, TokenType::STRING_LITERAL})) {
        return std::make_unique<LiteralExpression>(previous().location, previous());
    }
    
    if (match({TokenType::IDENTIFIER, TokenType::SELF})) {
        return std::make_unique<IdentifierExpression>(previous().location, previous().lexeme);
    }
    
    if (match({TokenType::LEFT_PAREN})) {
        auto expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }
    
    reportError("Expected expression");
    throw std::runtime_error("Parse error: Expected expression");
}

// Type parsing
std::unique_ptr<Type> Parser::parseType() {
    // Handle nested pointer types: cptr, unique, shared, weak
    // This allows for types like: shared unique weak Type, cptr cptr Type, etc.
    if (match({TokenType::CPTR, TokenType::UNIQUE, TokenType::SHARED, TokenType::WEAK})) {
        return parsePointerType();
    }
    
    auto base_type = parsePrimitiveType();
    
    if (match({TokenType::LEFT_BRACKET})) {
        if (peek().int_value <= 0) {
            reportError("Expected positive array size");
            throw std::runtime_error("Parse error: Expected positive array size");
        }

        size_t size = peek().int_value;

        consume(TokenType::INTEGER_LITERAL, "Expected array size");
        consume(TokenType::RIGHT_BRACKET, "Expected ']' after array type");
        return std::make_unique<ArrayType>(base_type->location, std::move(base_type), size);
    }
    
    return base_type;
}

std::unique_ptr<Type> Parser::parsePrimitiveType() {
    // Handle all primitive types including new foreign types
    if (match({TokenType::I8, TokenType::I16, TokenType::I32, TokenType::I64, 
               TokenType::U8, TokenType::U16, TokenType::U32, TokenType::U64, 
               TokenType::F32, TokenType::F64, TokenType::BOOL, TokenType::STRING, 
               TokenType::VOID, TokenType::SELF, TokenType::RAW_VA_LIST})) {
        return std::make_unique<PrimitiveType>(previous().location, previous().type);
    }
    
    // Handle void type for foreign functions
    if (match({TokenType::IDENTIFIER})) {
        Token type_name = previous();
        
        // Special handling for 'void' type in foreign contexts
        if (type_name.lexeme == "void") {
            return std::make_unique<PrimitiveType>(type_name.location, TokenType::VOID);
        }
        
        // Check if it's a generic type
        if (match({TokenType::LESS})) {
            std::vector<std::unique_ptr<Type>> type_arguments;
            do {
                type_arguments.push_back(parseType());
            } while (match({TokenType::COMMA}));
            consume(TokenType::GREATER, "Expected '>' after generic type arguments");
            
            return std::make_unique<GenericType>(type_name.location, type_name.lexeme, std::move(type_arguments));
        } else {
            // Simple identifier type (user-defined type)
            // For now, treat as a primitive type with the identifier name
            return std::make_unique<PrimitiveType>(type_name.location, TokenType::IDENTIFIER);
        }
    }
    
    reportError("Expected type");
    throw std::runtime_error("Parse error: Expected type");
}

std::unique_ptr<Type> Parser::parsePointerType() {
    TokenType pointer_kind = previous().type;
    SourceLocation location = previous().location;
    
    if (pointer_kind == TokenType::CPTR) {
        // C pointer: cptr Type (can be nested: cptr cptr Type)
        auto pointee = parseType(); // This will recursively handle nested pointers
        return std::make_unique<PointerType>(location, std::move(pointee), pointer_kind);
    } else {
        // Pang smart pointers: unique Type, shared Type, weak Type
        // These can be nested: shared unique weak Type
        auto pointee = parseType(); // This will recursively handle nested pointers
        return std::make_unique<PointerType>(location, std::move(pointee), pointer_kind);
    }
}

// Parameter and argument parsing
std::vector<Parameter> Parser::parseParameterList() {
    std::vector<Parameter> parameters;
    skipNewlines();

    if (check(TokenType::RIGHT_PAREN))
        return parameters; // empty parameter list

    while (!check(TokenType::EOF_TOKEN)) {
        parameters.push_back(parseParameter());

        if (check(TokenType::RIGHT_PAREN))
            break;
        
        consume(TokenType::COMMA, "Expected ',' after parameter");
        skipNewlines();
    }

    if (check(TokenType::EOF_TOKEN))
    {
        error_reporter->reportError(peek().location, "Expected ')' to close parameter list, but reached end of file");
        throw std::runtime_error("Expected ')' to close parameter list");
    }

    return parameters;
}

Parameter Parser::parseParameter() {
    // Handle 'self' parameter (no type annotation needed)
    if (match({TokenType::SELF})) {
        Token self_token = previous();
        auto self_type = std::make_unique<PrimitiveType>(self_token.location, TokenType::SELF);
        return Parameter("self", std::move(self_type), self_token.location);
    }
    
    Token name = consume(TokenType::IDENTIFIER, "Expected parameter name");
    consume(TokenType::COLON, "Expected ':' after parameter name");
    auto type = parseType();
    
    return Parameter(name.lexeme, std::move(type), name.location);
}

std::vector<std::unique_ptr<Expression>> Parser::parseArgumentList() {
    std::vector<std::unique_ptr<Expression>> arguments;
    skipNewlines();

    if (check(TokenType::RIGHT_PAREN))
        return arguments; // empty argument list

    
    while (!check(TokenType::EOF_TOKEN)) {
        arguments.push_back(parseExpression());

        if (check(TokenType::RIGHT_PAREN))
            break;

        consume(TokenType::COMMA, "Expected ',' after argument");
        skipNewlines();
    }

    if (check(TokenType::EOF_TOKEN))
    {
        error_reporter->reportError(peek().location, "Expected ')' to close argument list, but reached end of file");
        throw std::runtime_error("Expected ')' to close argument list");
    }
    
    return arguments;
}

// Class, Struct, and Enum parsing
std::unique_ptr<ClassDeclaration> Parser::parseClassDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected class name");
    SourceLocation location = name.location;
    
    // Parse generic parameters if present (e.g., Array<T>, Map<K,V>)
    std::vector<std::string> generic_parameters;
    if (match({TokenType::LESS})) {
        do {
            Token generic_param = consume(TokenType::IDENTIFIER, "Expected generic parameter name");
            generic_parameters.push_back(generic_param.lexeme);
        } while (match({TokenType::COMMA}));
        consume(TokenType::GREATER, "Expected '>' after generic parameters");
    }
    
    // Parse inheritance if present
    std::string base_class;
    if (match({TokenType::COLON})) {
        Token base = consume(TokenType::IDENTIFIER, "Expected base class name");
        base_class = base.lexeme;
    }
    
    auto class_decl = std::make_unique<ClassDeclaration>(location, name.lexeme, std::move(generic_parameters), base_class);
    
    consume(TokenType::LEFT_BRACE, "Expected '{' after class declaration");
    
    // Parse class members
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RIGHT_BRACE)) break;
        
        // Parse field declarations
        if (match({TokenType::LET})) {
            Token field_name = consume(TokenType::IDENTIFIER, "Expected field name");
            consume(TokenType::COLON, "Expected ':' after field name");
            auto field_type = parseType();
            
            // Create field member
            auto field_member = std::make_unique<FieldMember>(
                field_name.lexeme, field_name.location, std::move(field_type)
            );
            class_decl->members.push_back(std::move(field_member));
            
            skipNewlines();
        }
        // Parse constructor (class name followed by parameters)
        else if (check(TokenType::IDENTIFIER) && peek().lexeme == name.lexeme) {
            advance(); // consume class name
            consume(TokenType::LEFT_PAREN, "Expected '(' after constructor name");
            std::vector<Parameter> parameters = parseParameterList();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after constructor parameters");
            consume(TokenType::ARROW, "Expected '->' after constructor parameters");
            
            // Constructor return type should be 'self'
            if (!match({TokenType::SELF})) {
                reportError("Constructor must return 'self'");
            }
            
            consume(TokenType::LEFT_BRACE, "Expected '{' before constructor body");
            auto body = parseBlockStatement();
            
            // Create constructor as a special method
            auto self_type = std::make_unique<PrimitiveType>(name.location, TokenType::SELF);
            auto constructor_member = std::make_unique<MethodMember>(
                name.lexeme, name.location, std::move(parameters),
                std::move(self_type), std::move(body)
            );
            class_decl->members.push_back(std::move(constructor_member));
        }
        // Parse method declarations
        else if (match({TokenType::FN})) {
            auto method = parseFunctionDeclaration();
            // Convert function to method member
            auto method_member = std::make_unique<MethodMember>(
                method->name, method->location, std::move(method->parameters),
                std::move(method->return_type), std::move(method->body)
            );
            class_decl->members.push_back(std::move(method_member));
        } else {
            // Skip unknown members
            reportError("Expected field, constructor, or method declaration");
            advance();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after class body");
    return class_decl;
}

std::unique_ptr<StructDeclaration> Parser::parseStructDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected struct name");
    auto struct_decl = std::make_unique<StructDeclaration>(name.location, name.lexeme);
    
    consume(TokenType::LEFT_BRACE, "Expected '{' after struct name");
    
    // Parse struct fields
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RIGHT_BRACE)) break;
        
        Token field_name = consume(TokenType::IDENTIFIER, "Expected field name");
        consume(TokenType::COLON, "Expected ':' after field name");
        auto field_type = parseType();
        
        struct_decl->fields.emplace_back(field_name.lexeme, std::move(field_type), field_name.location);
        
        // Optional comma or newline between fields
        if (!match({TokenType::COMMA})) {
            skipNewlines();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after struct body");
    return struct_decl;
}

std::unique_ptr<EnumDeclaration> Parser::parseEnumDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected enum name");
    auto enum_decl = std::make_unique<EnumDeclaration>(name.location, name.lexeme);
    
    consume(TokenType::LEFT_BRACE, "Expected '{' after enum name");
    
    // Parse enum variants
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RIGHT_BRACE)) break;
        
        Token variant_name = consume(TokenType::IDENTIFIER, "Expected variant name");
        EnumVariant variant(variant_name.lexeme, variant_name.location);
        
        // For now, just support simple variants without associated data
        // In a full implementation, we'd parse associated types like Some(T)
        
        enum_decl->variants.push_back(std::move(variant));
        
        // Optional comma between variants
        if (!match({TokenType::COMMA})) {
            skipNewlines();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after enum body");
    return enum_decl;
}

// Import parsing
std::unique_ptr<ImportDeclaration> Parser::parseImportDeclaration() {
    SourceLocation location = previous().location;
    
    // Parse module path (e.g., "stdlib/io", "math/vector")
    Token module_path_token = consume(TokenType::STRING_LITERAL, "Expected module path string after 'import'");
    std::string module_path = module_path_token.lexeme;
    
    // Remove quotes from string literal
    if (module_path.length() >= 2 && module_path.front() == '"' && module_path.back() == '"') {
        module_path = module_path.substr(1, module_path.length() - 2);
    }
    
    std::vector<std::string> imported_items;
    bool is_wildcard = false;
    
    // Check for specific imports: import "module" { item1, item2, ... }
    if (match({TokenType::LEFT_BRACE})) {
        if (match({TokenType::MULTIPLY})) {
            // Wildcard import: import "module" { * }
            is_wildcard = true;
        } else {
            // Specific imports: import "module" { item1, item2, ... }
            do {
                Token item = consume(TokenType::IDENTIFIER, "Expected import item name");
                imported_items.push_back(item.lexeme);
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_BRACE, "Expected '}' after import items");
    } else {
        // Default wildcard import: import "module"
        is_wildcard = true;
    }
    
    consumeOptionalSemicolon();
    
    return std::make_unique<ImportDeclaration>(location, module_path, std::move(imported_items), is_wildcard);
}

// Foreign declaration parsing
std::unique_ptr<FunctionDeclaration> Parser::parseForeignFunctionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected foreign function name");
    
    consume(TokenType::LEFT_PAREN, "Expected '(' after foreign function name");
    std::vector<Parameter> parameters = parseParameterList();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    
    consume(TokenType::ARROW, "Expected '->' after parameters");
    auto return_type = parseType();
    
    // Foreign functions don't have bodies
    consumeOptionalSemicolon();
    
    return std::make_unique<FunctionDeclaration>(
        name.location, name.lexeme, std::move(parameters), 
        std::move(return_type), nullptr, true // is_foreign = true
    );
}

std::unique_ptr<StructDeclaration> Parser::parseForeignStructDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected foreign struct name");
    auto struct_decl = std::make_unique<StructDeclaration>(name.location, name.lexeme, true); // is_foreign = true
    
    consume(TokenType::LEFT_BRACE, "Expected '{' after foreign struct name");
    
    // Parse struct fields (same as regular struct)
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RIGHT_BRACE)) break;
        
        Token field_name = consume(TokenType::IDENTIFIER, "Expected field name");
        consume(TokenType::COLON, "Expected ':' after field name");
        auto field_type = parseType();
        
        struct_decl->fields.emplace_back(field_name.lexeme, std::move(field_type), field_name.location);
        
        // Optional comma or newline between fields
        if (!match({TokenType::COMMA})) {
            skipNewlines();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after foreign struct body");
    return struct_decl;
}

std::unique_ptr<EnumDeclaration> Parser::parseForeignEnumDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected foreign enum name");
    auto enum_decl = std::make_unique<EnumDeclaration>(name.location, name.lexeme, true); // is_foreign = true
    
    consume(TokenType::LEFT_BRACE, "Expected '{' after foreign enum name");
    
    // Parse enum variants (same as regular enum)
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        skipNewlines();
        if (check(TokenType::RIGHT_BRACE)) break;
        
        Token variant_name = consume(TokenType::IDENTIFIER, "Expected variant name");
        EnumVariant variant(variant_name.lexeme, variant_name.location);
        
        enum_decl->variants.push_back(std::move(variant));
        
        // Optional comma between variants
        if (!match({TokenType::COMMA})) {
            skipNewlines();
        }
    }
    
    consume(TokenType::RIGHT_BRACE, "Expected '}' after foreign enum body");
    return enum_decl;
}

std::unique_ptr<VariableDeclaration> Parser::parseConstDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected constant name");
    
    consume(TokenType::COLON, "Expected ':' after constant name");
    auto type = parseType();
    
    consume(TokenType::ASSIGN, "Expected '=' after constant type");
    auto initializer = parseExpression();
    
    consumeOptionalSemicolon();
    
    return std::make_unique<VariableDeclaration>(
        name.location, name.lexeme, std::move(type), 
        std::move(initializer), false // constants are immutable
    );
}

std::unique_ptr<VariableDeclaration> Parser::parseForeignConstDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected foreign constant name");
    
    consume(TokenType::COLON, "Expected ':' after foreign constant name");
    auto type = parseType();
    
    // Foreign constants don't have initializers (they're defined in C)
    consumeOptionalSemicolon();
    
    return std::make_unique<VariableDeclaration>(
        name.location, name.lexeme, std::move(type), 
        nullptr, false // foreign constants are immutable
    );
}

std::unique_ptr<VariableDeclaration> Parser::parseTypeAlias() {
    Token name = consume(TokenType::IDENTIFIER, "Expected type alias name");
    
    consume(TokenType::ASSIGN, "Expected '=' after type alias name");
    auto aliased_type = parseType();
    
    consumeOptionalSemicolon();
    
    // For now, treat type aliases as constant declarations
    // In a full implementation, we'd have a separate TypeAlias AST node
    return std::make_unique<VariableDeclaration>(
        name.location, name.lexeme, std::move(aliased_type), 
        nullptr, false // type aliases are immutable
    );
}

} // namespace pangea
