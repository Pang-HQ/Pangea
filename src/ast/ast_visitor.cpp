//#include "ast_visitor.h"
//#include "ast_nodes.h"
//#include <iostream>
//
//// Simple AST printer visitor for debugging
//class ASTPrinter : public ASTVisitor {
//private:
//    int indent_level = 0;
//    
//    void printIndent() {
//        for (int i = 0; i < indent_level; ++i) {
//            std::cout << "  ";
//        }
//    }
//    
//public:
//    void visit(PrimitiveType& node) override {
//        std::cout << node.toString();
//    }
//    
//    void visit(ArrayType& node) override {
//        std::cout << node.toString();
//    }
//    
//    void visit(PointerType& node) override {
//        std::cout << node.toString();
//    }
//    
//    void visit(LiteralExpression& node) override {
//        std::cout << "Literal(" << node.literal_token.lexeme << ")";
//    }
//    
//    void visit(IdentifierExpression& node) override {
//        std::cout << "Identifier(" << node.name << ")";
//    }
//    
//    void visit(BinaryExpression& node) override {
//        std::cout << "Binary(";
//        node.left->accept(*this);
//        std::cout << " " << TokenUtils::tokenTypeToString(node.operator_token) << " ";
//        node.right->accept(*this);
//        std::cout << ")";
//    }
//    
//    void visit(UnaryExpression& node) override {
//        std::cout << "Unary(" << TokenUtils::tokenTypeToString(node.operator_token) << " ";
//        node.operand->accept(*this);
//        std::cout << ")";
//    }
//    
//    void visit(CallExpression& node) override {
//        std::cout << "Call(";
//        node.callee->accept(*this);
//        std::cout << ", [";
//        for (size_t i = 0; i < node.arguments.size(); ++i) {
//            if (i > 0) std::cout << ", ";
//            node.arguments[i]->accept(*this);
//        }
//        std::cout << "])";
//    }
//    
//    void visit(MemberExpression& node) override {
//        std::cout << "Member(";
//        node.object->accept(*this);
//        std::cout << "." << node.member_name << ")";
//    }
//    
//    void visit(IndexExpression& node) override {
//        std::cout << "Index(";
//        node.object->accept(*this);
//        std::cout << "[";
//        node.index->accept(*this);
//        std::cout << "])";
//    }
//    
//    void visit(ExpressionStatement& node) override {
//        printIndent();
//        std::cout << "ExpressionStatement(";
//        node.expression->accept(*this);
//        std::cout << ")" << std::endl;
//    }
//    
//    void visit(BlockStatement& node) override {
//        printIndent();
//        std::cout << "Block {" << std::endl;
//        indent_level++;
//        for (auto& stmt : node.statements) {
//            stmt->accept(*this);
//        }
//        indent_level--;
//        printIndent();
//        std::cout << "}" << std::endl;
//    }
//    
//    void visit(IfStatement& node) override {
//        printIndent();
//        std::cout << "If(";
//        node.condition->accept(*this);
//        std::cout << ")" << std::endl;
//        indent_level++;
//        node.then_branch->accept(*this);
//        if (node.else_branch) {
//            indent_level--;
//            printIndent();
//            std::cout << "Else" << std::endl;
//            indent_level++;
//            node.else_branch->accept(*this);
//        }
//        indent_level--;
//    }
//    
//    void visit(WhileStatement& node) override {
//        printIndent();
//        std::cout << "WhileStatement(";
//        node.condition->accept(*this);
//        std::cout << ", body: ";
//        indent_level++;
//        node.body->accept(*this);
//        indent_level--;
//    }
//    
//    void visit(ForStatement& node) override {
//        printIndent();
//        std::cout << "For(" << node.iterator_name << " in ";
//        node.iterable->accept(*this);
//        std::cout << ")" << std::endl;
//        indent_level++;
//        node.body->accept(*this);
//        indent_level--;
//    }
//    
//    void visit(ReturnStatement& node) override {
//        printIndent();
//        std::cout << "Return(";
//        if (node.value) {
//            node.value->accept(*this);
//        }
//        std::cout << ")" << std::endl;
//    }
//    
//    void visit(FunctionDeclaration& node) override {
//        printIndent();
//        std::cout << "Function " << node.name << "(";
//        for (size_t i = 0; i < node.parameters.size(); ++i) {
//            if (i > 0) std::cout << ", ";
//            std::cout << node.parameters[i].name << ": ";
//            node.parameters[i].type->accept(*this);
//        }
//        std::cout << ") -> ";
//        node.return_type->accept(*this);
//        std::cout << std::endl;
//        indent_level++;
//        node.body->accept(*this);
//        indent_level--;
//    }
//    
//    void visit(VariableDeclaration& node) override {
//        printIndent();
//        std::cout << "Variable " << node.name;
//        if (node.type) {
//            std::cout << ": ";
//            node.type->accept(*this);
//        }
//        if (node.initializer) {
//            std::cout << " = ";
//            node.initializer->accept(*this);
//        }
//        std::cout << " (mutable: " << (node.is_mutable ? "true" : "false") << ")" << std::endl;
//    }
//    
//    void visit(Program& node) override {
//        std::cout << "Program:" << std::endl;
//        indent_level++;
//        for (auto& decl : node.declarations) {
//            decl->accept(*this);
//        }
//        indent_level--;
//    }
//};