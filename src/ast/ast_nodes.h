#pragma once

#include "../utils/source_location.h"
#include "../lexer/token.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>

namespace pangea {

// Forward declarations
class ASTVisitor;

// Base AST Node
class ASTNode {
public:
    SourceLocation location;
    
    explicit ASTNode(const SourceLocation& loc) : location(loc) {}
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

// Type system
class Type : public ASTNode {
public:
    explicit Type(const SourceLocation& loc) : ASTNode(loc) {}
    virtual std::string toString() const = 0;
};

class PrimitiveType : public Type {
public:
    TokenType type_token;
    
    PrimitiveType(const SourceLocation& loc, TokenType token)
        : Type(loc), type_token(token) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ArrayType : public Type {
public:
    std::unique_ptr<Type> element_type;
    size_t size;
    
    ArrayType(const SourceLocation& loc, std::unique_ptr<Type> elem_type, size_t array_size)
        : Type(loc), element_type(std::move(elem_type)), size(array_size) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class PointerType : public Type {
public:
    std::unique_ptr<Type> pointee_type;
    TokenType pointer_kind; // MULTIPLY for raw, UNIQUE, SHARED, WEAK
    
    PointerType(const SourceLocation& loc, std::unique_ptr<Type> pointee, TokenType kind)
        : Type(loc), pointee_type(std::move(pointee)), pointer_kind(kind) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

// Expressions
class Expression : public ASTNode {
public:
    explicit Expression(const SourceLocation& loc) : ASTNode(loc) {}
};

class LiteralExpression : public Expression {
public:
    Token literal_token;
    
    LiteralExpression(const SourceLocation& loc, const Token& token)
        : Expression(loc), literal_token(token) {}
    
    void accept(ASTVisitor& visitor) override;
};

class IdentifierExpression : public Expression {
public:
    std::string name;
    
    IdentifierExpression(const SourceLocation& loc, const std::string& identifier)
        : Expression(loc), name(identifier) {}
    
    void accept(ASTVisitor& visitor) override;
};

class BinaryExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    TokenType operator_token;
    std::unique_ptr<Expression> right;
    
    BinaryExpression(const SourceLocation& loc, std::unique_ptr<Expression> lhs, TokenType op, std::unique_ptr<Expression> rhs)
        : Expression(loc), left(std::move(lhs)), operator_token(op), right(std::move(rhs)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class UnaryExpression : public Expression {
public:
    TokenType operator_token;
    std::unique_ptr<Expression> operand;
    
    UnaryExpression(const SourceLocation& loc, TokenType op, std::unique_ptr<Expression> expr)
        : Expression(loc), operator_token(op), operand(std::move(expr)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class CallExpression : public Expression {
public:
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    
    CallExpression(const SourceLocation& loc, std::unique_ptr<Expression> func, std::vector<std::unique_ptr<Expression>> args)
        : Expression(loc), callee(std::move(func)), arguments(std::move(args)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class MemberExpression : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::string member_name;
    
    MemberExpression(const SourceLocation& loc, std::unique_ptr<Expression> obj, const std::string& member)
        : Expression(loc), object(std::move(obj)), member_name(member) {}
    
    void accept(ASTVisitor& visitor) override;
};

class IndexExpression : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> index;
    
    IndexExpression(const SourceLocation& loc, std::unique_ptr<Expression> obj, std::unique_ptr<Expression> idx)
        : Expression(loc), object(std::move(obj)), index(std::move(idx)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class AssignmentExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    TokenType operator_token; // ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, etc.
    std::unique_ptr<Expression> right;
    
    AssignmentExpression(const SourceLocation& loc, std::unique_ptr<Expression> lhs, TokenType op, std::unique_ptr<Expression> rhs)
        : Expression(loc), left(std::move(lhs)), operator_token(op), right(std::move(rhs)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class PostfixExpression : public Expression {
public:
    std::unique_ptr<Expression> operand;
    TokenType operator_token; // INCREMENT, DECREMENT
    
    PostfixExpression(const SourceLocation& loc, std::unique_ptr<Expression> expr, TokenType op)
        : Expression(loc), operand(std::move(expr)), operator_token(op) {}
    
    void accept(ASTVisitor& visitor) override;
};

class CastExpression : public Expression {
public:
    std::unique_ptr<Type> target_type; // The type to cast to
    std::unique_ptr<Expression> expression; // The expression to cast
    bool is_safe_cast; // true for try_cast, false for cast
    
    CastExpression(const SourceLocation& loc, std::unique_ptr<Type> type, std::unique_ptr<Expression> expr, bool safe = false)
        : Expression(loc), target_type(std::move(type)), expression(std::move(expr)), is_safe_cast(safe) {}
    
    void accept(ASTVisitor& visitor) override;
};

class AsExpression : public Expression {
public:
    std::unique_ptr<Expression> expression; // The expression to cast
    std::unique_ptr<Type> target_type; // The type to cast to
    
    AsExpression(const SourceLocation& loc, std::unique_ptr<Expression> expr, std::unique_ptr<Type> type)
        : Expression(loc), expression(std::move(expr)), target_type(std::move(type)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Statements
class Statement : public ASTNode {
public:
    explicit Statement(const SourceLocation& loc) : ASTNode(loc) {}
};

class ExpressionStatement : public Statement {
public:
    std::unique_ptr<Expression> expression;
    
    ExpressionStatement(const SourceLocation& loc, std::unique_ptr<Expression> expr)
        : Statement(loc), expression(std::move(expr)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class BlockStatement : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    explicit BlockStatement(const SourceLocation& loc)
        : Statement(loc) {}
    
    void accept(ASTVisitor& visitor) override;
};

class IfStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> then_branch;
    std::unique_ptr<Statement> else_branch; // nullable
    
    IfStatement(const SourceLocation& loc, std::unique_ptr<Expression> cond, std::unique_ptr<Statement> then_stmt, std::unique_ptr<Statement> else_stmt = nullptr)
        : Statement(loc), condition(std::move(cond)), then_branch(std::move(then_stmt)), else_branch(std::move(else_stmt)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class WhileStatement : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    
    WhileStatement(const SourceLocation& loc, std::unique_ptr<Expression> cond, std::unique_ptr<Statement> loop_body)
        : Statement(loc), condition(std::move(cond)), body(std::move(loop_body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class ForStatement : public Statement {
public:
    std::string iterator_name;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;
    
    ForStatement(const SourceLocation& loc, const std::string& iter, std::unique_ptr<Expression> iter_expr, std::unique_ptr<Statement> loop_body)
        : Statement(loc), iterator_name(iter), iterable(std::move(iter_expr)), body(std::move(loop_body)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class ReturnStatement : public Statement {
public:
    std::unique_ptr<Expression> value; // nullable
    
    ReturnStatement(const SourceLocation& loc, std::unique_ptr<Expression> return_value = nullptr)
        : Statement(loc), value(std::move(return_value)) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Declarations
class Declaration : public ASTNode {
public:
    explicit Declaration(const SourceLocation& loc) : ASTNode(loc) {}
};

class DeclarationStatement : public Statement {
public:
    std::unique_ptr<Declaration> declaration;
    
    DeclarationStatement(const SourceLocation& loc, std::unique_ptr<Declaration> decl)
        : Statement(loc), declaration(std::move(decl)) {}
    
    void accept(ASTVisitor& visitor) override;
};

class Parameter {
public:
    std::string name;
    std::unique_ptr<Type> type;
    SourceLocation location;
    
    Parameter(const std::string& param_name, std::unique_ptr<Type> param_type, const SourceLocation& loc)
        : name(param_name), type(std::move(param_type)), location(loc) {}
};

class FunctionDeclaration : public Declaration {
public:
    std::string name;
    std::vector<Parameter> parameters;
    std::unique_ptr<Type> return_type;
    std::unique_ptr<BlockStatement> body; // nullptr for foreign functions
    bool is_foreign;
    
    FunctionDeclaration(const SourceLocation& loc, const std::string& func_name, std::vector<Parameter> params, std::unique_ptr<Type> ret_type, std::unique_ptr<BlockStatement> func_body = nullptr, bool foreign = false)
        : Declaration(loc), name(func_name), parameters(std::move(params)), return_type(std::move(ret_type)), body(std::move(func_body)), is_foreign(foreign) {}
    
    void accept(ASTVisitor& visitor) override;
};

class VariableDeclaration : public Declaration {
public:
    std::string name;
    std::unique_ptr<Type> type; // nullable for type inference
    std::unique_ptr<Expression> initializer; // nullable
    bool is_mutable;
    
    VariableDeclaration(const SourceLocation& loc, const std::string& var_name, std::unique_ptr<Type> var_type, std::unique_ptr<Expression> init, bool mutable_flag)
        : Declaration(loc), name(var_name), type(std::move(var_type)), initializer(std::move(init)), is_mutable(mutable_flag) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Class member (field or method)
class ClassMember {
public:
    std::string name;
    SourceLocation location;
    bool is_public;
    
    ClassMember(const std::string& member_name, const SourceLocation& loc, bool public_access = true)
        : name(member_name), location(loc), is_public(public_access) {}
    
    virtual ~ClassMember() = default;
};

class FieldMember : public ClassMember {
public:
    std::unique_ptr<Type> type;
    std::unique_ptr<Expression> initializer; // nullable
    
    FieldMember(const std::string& field_name, const SourceLocation& loc, std::unique_ptr<Type> field_type, std::unique_ptr<Expression> init = nullptr, bool public_access = true)
        : ClassMember(field_name, loc, public_access), type(std::move(field_type)), initializer(std::move(init)) {}
};

class MethodMember : public ClassMember {
public:
    std::vector<Parameter> parameters;
    std::unique_ptr<Type> return_type;
    std::unique_ptr<BlockStatement> body;
    bool is_static;
    bool is_virtual;
    bool is_override;
    
    MethodMember(const std::string& method_name, const SourceLocation& loc, std::vector<Parameter> params, std::unique_ptr<Type> ret_type, std::unique_ptr<BlockStatement> method_body, bool public_access = true, bool static_method = false, bool virtual_method = false, bool override_method = false)
        : ClassMember(method_name, loc, public_access), parameters(std::move(params)), return_type(std::move(ret_type)), body(std::move(method_body)), is_static(static_method), is_virtual(virtual_method), is_override(override_method) {}
};

class ClassDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::string> generic_parameters; // For generic classes like Array<T>
    std::string base_class; // For inheritance (empty if no base class)
    std::vector<std::unique_ptr<ClassMember>> members;
    
    ClassDeclaration(const SourceLocation& loc, const std::string& class_name, std::vector<std::string> generics = {}, const std::string& base = "")
        : Declaration(loc), name(class_name), generic_parameters(std::move(generics)), base_class(base) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Struct member (field only - structs don't have methods in this implementation)
class StructField {
public:
    std::string name;
    std::unique_ptr<Type> type;
    SourceLocation location;
    
    StructField(const std::string& field_name, std::unique_ptr<Type> field_type, const SourceLocation& loc)
        : name(field_name), type(std::move(field_type)), location(loc) {}
};

class StructDeclaration : public Declaration {
public:
    std::string name;
    std::vector<StructField> fields;
    bool is_foreign;
    
    StructDeclaration(const SourceLocation& loc, const std::string& struct_name, bool foreign = false)
        : Declaration(loc), name(struct_name), is_foreign(foreign) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Enum variant
class EnumVariant {
public:
    std::string name;
    std::vector<std::unique_ptr<Type>> associated_types; // For variants with data
    SourceLocation location;
    
    EnumVariant(const std::string& variant_name, const SourceLocation& loc)
        : name(variant_name), location(loc) {}
};

class EnumDeclaration : public Declaration {
public:
    std::string name;
    std::vector<EnumVariant> variants;
    bool is_foreign;
    
    EnumDeclaration(const SourceLocation& loc, const std::string& enum_name, bool foreign = false)
        : Declaration(loc), name(enum_name), is_foreign(foreign) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Generic type (for Array<T>, Map<K,V>, etc.)
class GenericType : public Type {
public:
    std::string base_name; // e.g., "Array", "Map"
    std::vector<std::unique_ptr<Type>> type_arguments; // e.g., [T], [K, V]
    
    GenericType(const SourceLocation& loc, const std::string& base, std::vector<std::unique_ptr<Type>> args)
        : Type(loc), base_name(base), type_arguments(std::move(args)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

// Import declaration
class ImportDeclaration : public Declaration {
public:
    std::string module_path; // e.g., "stdlib/io", "math/vector"
    std::vector<std::string> imported_items; // empty for wildcard import
    bool is_wildcard; // true for "import *"
    
    ImportDeclaration(const SourceLocation& loc, const std::string& path, std::vector<std::string> items = {}, bool wildcard = false)
        : Declaration(loc), module_path(path), imported_items(std::move(items)), is_wildcard(wildcard) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Module (compilation unit)
class Module : public ASTNode {
public:
    std::string module_name;
    std::string file_path;
    std::vector<std::unique_ptr<ImportDeclaration>> imports;
    std::vector<std::unique_ptr<Declaration>> declarations;
    
    Module(const SourceLocation& loc, const std::string& name, const std::string& path)
        : ASTNode(loc), module_name(name), file_path(path) {}
    
    void accept(ASTVisitor& visitor) override;
};

// Program (root node) - now contains modules
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<Module>> modules;
    std::unique_ptr<Module> main_module; // The entry point module
    
    explicit Program(const SourceLocation& loc) : ASTNode(loc) {}
    
    void accept(ASTVisitor& visitor) override;
};

} // namespace pangea
