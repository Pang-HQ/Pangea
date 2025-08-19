#include "ast_nodes.h"
#include "ast_visitor.h"

namespace pangea {

// Type implementations
void PrimitiveType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string PrimitiveType::toString() const {
    switch (type_token) {
        case TokenType::I8: return "i8";
        case TokenType::I16: return "i16";
        case TokenType::I32: return "i32";
        case TokenType::I64: return "i64";
        case TokenType::U8: return "u8";
        case TokenType::U16: return "u16";
        case TokenType::U32: return "u32";
        case TokenType::U64: return "u64";
        case TokenType::F32: return "f32";
        case TokenType::F64: return "f64";
        case TokenType::BOOL: return "bool";
        case TokenType::STRING: return "string";
        case TokenType::VOID: return "void";
        case TokenType::SELF: return "self";
        case TokenType::UNIQUE: return "unique";
        case TokenType::SHARED: return "shared";
        case TokenType::WEAK: return "weak";
        case TokenType::CPTR: return "cptr";
        case TokenType::RAW_VA_LIST: return "raw_va_list";

        // user-defined types, assume all identifiers are "assumed_type" here - the name will be in the token's lexeme
        case TokenType::IDENTIFIER: return "assumed_type";
        
        default: return "unknown";
    }
}

void ArrayType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ArrayType::toString() const {
    return element_type->toString() + "[" + std::to_string(size) + "]";
}

void PointerType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string PointerType::toString() const {
    switch (pointer_kind) {
        case TokenType::CPTR: return "cptr<" + pointee_type->toString() + ">";
        case TokenType::UNIQUE: return "unique<" + pointee_type->toString() + ">";
        case TokenType::SHARED: return "shared<" + pointee_type->toString() + ">";
        case TokenType::WEAK: return "weak<" + pointee_type->toString() + ">";
        default: return "unknown_ptr<" + pointee_type->toString() + ">";
    }
}

// Expression implementations
void LiteralExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IdentifierExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BinaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void UnaryExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CallExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void MemberExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IndexExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AssignmentExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void PostfixExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CastExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AsExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Statement implementations
void ExpressionStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BlockStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IfStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void WhileStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ForStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ReturnStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void DeclarationStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Declaration implementations
void FunctionDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void VariableDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

// Program implementation
void ImportDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void Module::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void Program::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ClassDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void StructDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void EnumDeclaration::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void GenericType::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string GenericType::toString() const {
    std::string result = base_name + "<";
    for (size_t i = 0; i < type_arguments.size(); ++i) {
        if (i > 0) result += ", ";
        result += type_arguments[i]->toString();
    }
    result += ">";
    return result;
}

} // namespace pangea
