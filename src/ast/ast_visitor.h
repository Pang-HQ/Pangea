#pragma once

#include "ast_nodes.h"

namespace pangea {

// Forward declarations
class PrimitiveType;
class ConstType;
class ArrayType;
class PointerType;
class GenericType;
class LiteralExpression;
class IdentifierExpression;
class BinaryExpression;
class UnaryExpression;
class CallExpression;
class MemberExpression;
class IndexExpression;
class AssignmentExpression;
class PostfixExpression;
class CastExpression;
class AsExpression;
class ExpressionStatement;
class BlockStatement;
class IfStatement;
class WhileStatement;
class ForStatement;
class ReturnStatement;
class DeclarationStatement;
class FunctionDeclaration;
class VariableDeclaration;
class ClassDeclaration;
class StructDeclaration;
class EnumDeclaration;
class ImportDeclaration;
class Module;
class Program;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Type visitors
    virtual void visit(PrimitiveType& node) = 0;
    virtual void visit(ConstType& node) = 0;
    virtual void visit(ArrayType& node) = 0;
    virtual void visit(PointerType& node) = 0;
    virtual void visit(GenericType& node) = 0;
    
    // Expression visitors
    virtual void visit(LiteralExpression& node) = 0;
    virtual void visit(IdentifierExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(MemberExpression& node) = 0;
    virtual void visit(IndexExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(PostfixExpression& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(AsExpression& node) = 0;
    
    // Statement visitors
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(BlockStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(DeclarationStatement& node) = 0;
    
    // Declaration visitors
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(ClassDeclaration& node) = 0;
    virtual void visit(StructDeclaration& node) = 0;
    virtual void visit(EnumDeclaration& node) = 0;
    
    // Import and Module visitors
    virtual void visit(ImportDeclaration& node) = 0;
    virtual void visit(Module& node) = 0;
    
    // Program visitor
    virtual void visit(Program& node) = 0;
};

} // namespace pangea
