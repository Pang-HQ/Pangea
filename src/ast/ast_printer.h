#pragma once

#include "ast_visitor.h"
#include <iostream>
#include <string>

namespace pangea {

class ASTPrinter : public ASTVisitor {
private:
    std::ostream& out;
    int indentLevel = 0;
    std::string indent() const { return std::string(indentLevel * 2, ' '); }
    void pushIndent() { indentLevel++; }
    void popIndent() { indentLevel--; }

public:
    explicit ASTPrinter(std::ostream& output = std::cout) : out(output) {}
    void printProgram(Program& program);

    // Simple inline methods
    void visit(LiteralExpression& node) override { out << indent() << "LiteralExpression(" << node.literal_token.toString() << ")" << std::endl; }
    void visit(IdentifierExpression& node) override { out << indent() << "IdentifierExpression(" << node.name << ")" << std::endl; }
    void visit(GenericType& node) override { out << indent() << "GenericType(" << node.toString() << ")" << std::endl; }
    void visit(ImportDeclaration& node) override {
        out << indent() << "ImportDeclaration(" << node.module_path;
        if (node.is_wildcard) out << ", wildcard";
        out << ")" << std::endl;
    }

    // Type visitors
    void visit(PrimitiveType& node) override;
    void visit(ConstType& node) override;
    void visit(ArrayType& node) override;
    void visit(PointerType& node) override;

    // Expression visitors
    void visit(UnaryExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(MemberExpression& node) override;
    void visit(IndexExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(PostfixExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(AsExpression& node) override;

    // Statement visitors
    void visit(ExpressionStatement& node) override;
    void visit(BlockStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(DeclarationStatement& node) override;

    // Declaration visitors
    void visit(FunctionDeclaration& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ClassDeclaration& node) override;
    void visit(StructDeclaration& node) override;
    void visit(EnumDeclaration& node) override;

    // Module visitor
    void visit(Module& node) override;

    // Program visitor
    void visit(Program& node) override;
};

} // namespace pangea
