#include "ast_printer.h"

namespace pangea {

void ASTPrinter::printProgram(Program& program) {
    program.accept(*this);
}

void ASTPrinter::visit(PrimitiveType& node) {
    out << indent() << "PrimitiveType(" << node.toString() << ")" << std::endl;
}

void ASTPrinter::visit(ConstType& node) {
    out << indent() << "ConstType(" << std::endl;
    pushIndent();
    out << indent() << "base_type:" << std::endl;
    pushIndent();
    node.base_type->accept(*this);
    popIndent();
    popIndent();
    out << indent() << ")" << std::endl;
}

void ASTPrinter::visit(ArrayType& node) {
    out << indent() << "ArrayType[" << node.size << "](" << std::endl;
    pushIndent();
    out << indent() << "element_type:" << std::endl;
    pushIndent();
    node.element_type->accept(*this);
    popIndent();
    popIndent();
    out << indent() << ")" << std::endl;
}

void ASTPrinter::visit(PointerType& node) {
    out << indent() << "PointerType(";
    switch (node.pointer_kind) {
        case TokenType::MULTIPLY: out << "raw"; break;
        case TokenType::UNIQUE: out << "unique"; break;
        case TokenType::SHARED: out << "shared"; break;
        case TokenType::WEAK: out << "weak"; break;
        default: out << "unknown"; break;
    }
    out << ")" << std::endl;
    pushIndent();
    out << indent() << "pointee_type:" << std::endl;
    pushIndent();
    node.pointee_type->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(UnaryExpression& node) {
    out << indent() << "UnaryExpression(";
    switch (node.operator_token) {
        case TokenType::PLUS: out << "+"; break;
        case TokenType::MINUS: out << "-"; break;
        case TokenType::LOGICAL_NOT: out << "!"; break;
        case TokenType::BITWISE_NOT: out << "~"; break;
        case TokenType::INCREMENT: out << "++"; break;
        case TokenType::DECREMENT: out << "--"; break;
        default: out << "unknown"; break;
    }
    out << ")" << std::endl;
    pushIndent();
    out << indent() << "operand:" << std::endl;
    pushIndent();
    node.operand->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(BinaryExpression& node) {
    out << indent() << "BinaryExpression(";
    switch (node.operator_token) {
        case TokenType::PLUS: out << "+"; break;
        case TokenType::MINUS: out << "-"; break;
        case TokenType::MULTIPLY: out << "*"; break;
        case TokenType::DIVIDE: out << "/"; break;
        case TokenType::EQUAL: out << "=="; break;
        case TokenType::NOT_EQUAL: out << "!="; break;
        case TokenType::LESS: out << "<"; break;
        case TokenType::GREATER: out << ">"; break;
        case TokenType::LESS_EQUAL: out << "<="; break;
        case TokenType::GREATER_EQUAL: out << ">="; break;
        case TokenType::LOGICAL_AND: out << "&&"; break;
        case TokenType::LOGICAL_OR: out << "||"; break;
        default: out << "unknown"; break;
    }
    out << ")" << std::endl;
    pushIndent();
    out << indent() << "left:" << std::endl;
    pushIndent();
    node.left->accept(*this);
    popIndent();
    out << indent() << "right:" << std::endl;
    pushIndent();
    node.right->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(CallExpression& node) {
    out << indent() << "CallExpression" << std::endl;
    pushIndent();
    out << indent() << "callee:" << std::endl;
    pushIndent();
    node.callee->accept(*this);
    popIndent();
    if (!node.arguments.empty()) {
        out << indent() << "arguments:" << std::endl;
        pushIndent();
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            out << indent() << "[" << i << "]:" << std::endl;
            pushIndent();
            node.arguments[i]->accept(*this);
            popIndent();
        }
        popIndent();
    }
    popIndent();
}

void ASTPrinter::visit(MemberExpression& node) {
    out << indent() << "MemberExpression(" << node.member_name << ")" << std::endl;
    pushIndent();
    out << indent() << "object:" << std::endl;
    pushIndent();
    node.object->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(IndexExpression& node) {
    out << indent() << "IndexExpression" << std::endl;
    pushIndent();
    out << indent() << "object:" << std::endl;
    pushIndent();
    node.object->accept(*this);
    popIndent();
    out << indent() << "index:" << std::endl;
    pushIndent();
    node.index->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(AssignmentExpression& node) {
    out << indent() << "AssignmentExpression(";
    switch (node.operator_token) {
        case TokenType::ASSIGN: out << "="; break;
        case TokenType::PLUS_ASSIGN: out << "+="; break;
        case TokenType::MINUS_ASSIGN: out << "-="; break;
        case TokenType::MULTIPLY_ASSIGN: out << "*="; break;
        case TokenType::DIVIDE_ASSIGN: out << "/="; break;
        default: out << "unknown"; break;
    }
    out << ")" << std::endl;
    pushIndent();
    out << indent() << "left:" << std::endl;
    pushIndent();
    node.left->accept(*this);
    popIndent();
    out << indent() << "right:" << std::endl;
    pushIndent();
    node.right->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(PostfixExpression& node) {
    out << indent() << "PostfixExpression(";
    switch (node.operator_token) {
        case TokenType::INCREMENT: out << "++"; break;
        case TokenType::DECREMENT: out << "--"; break;
        default: out << "unknown"; break;
    }
    out << ")" << std::endl;
    pushIndent();
    out << indent() << "operand:" << std::endl;
    pushIndent();
    node.operand->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(CastExpression& node) {
    out << indent() << "CastExpression(" << (node.is_safe_cast ? "safe" : "unsafe") << ")" << std::endl;
    pushIndent();
    out << indent() << "target_type:" << std::endl;
    pushIndent();
    node.target_type->accept(*this);
    popIndent();
    out << indent() << "expression:" << std::endl;
    pushIndent();
    node.expression->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(AsExpression& node) {
    out << indent() << "AsExpression" << std::endl;
    pushIndent();
    out << indent() << "expression:" << std::endl;
    pushIndent();
    node.expression->accept(*this);
    popIndent();
    out << indent() << "target_type:" << std::endl;
    pushIndent();
    node.target_type->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(ExpressionStatement& node) {
    out << indent() << "ExpressionStatement" << std::endl;
    pushIndent();
    out << indent() << "expression:" << std::endl;
    pushIndent();
    node.expression->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(BlockStatement& node) {
    out << indent() << "BlockStatement" << std::endl;
    if (!node.statements.empty()) {
        pushIndent();
        out << indent() << "statements:" << std::endl;
        pushIndent();
        for (size_t i = 0; i < node.statements.size(); ++i) {
            out << indent() << "[" << i << "]:" << std::endl;
            pushIndent();
            node.statements[i]->accept(*this);
            popIndent();
        }
        popIndent();
        popIndent();
    }
}

void ASTPrinter::visit(IfStatement& node) {
    out << indent() << "IfStatement" << std::endl;
    pushIndent();
    out << indent() << "condition:" << std::endl;
    pushIndent();
    node.condition->accept(*this);
    popIndent();
    out << indent() << "then_branch:" << std::endl;
    pushIndent();
    node.then_branch->accept(*this);
    popIndent();
    if (node.else_branch) {
        out << indent() << "else_branch:" << std::endl;
        pushIndent();
        node.else_branch->accept(*this);
        popIndent();
    }
    popIndent();
}

void ASTPrinter::visit(WhileStatement& node) {
    out << indent() << "WhileStatement" << std::endl;
    pushIndent();
    out << indent() << "condition:" << std::endl;
    pushIndent();
    node.condition->accept(*this);
    popIndent();
    out << indent() << "body:" << std::endl;
    pushIndent();
    node.body->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(ForStatement& node) {
    out << indent() << "ForStatement(iterator: " << node.iterator_name << ")" << std::endl;
    pushIndent();
    out << indent() << "iterable:" << std::endl;
    pushIndent();
    node.iterable->accept(*this);
    popIndent();
    out << indent() << "body:" << std::endl;
    pushIndent();
    node.body->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(ReturnStatement& node) {
    out << indent() << "ReturnStatement" << std::endl;
    if (node.value) {
        pushIndent();
        out << indent() << "value:" << std::endl;
        pushIndent();
        node.value->accept(*this);
        popIndent();
        popIndent();
    }
}

void ASTPrinter::visit(DeclarationStatement& node) {
    out << indent() << "DeclarationStatement" << std::endl;
    pushIndent();
    out << indent() << "declaration:" << std::endl;
    pushIndent();
    node.declaration->accept(*this);
    popIndent();
    popIndent();
}

void ASTPrinter::visit(FunctionDeclaration& node) {
    out << indent() << "FunctionDeclaration(" << node.name << ")" << std::endl;
    pushIndent();
    out << indent() << "return_type:" << std::endl;
    pushIndent();
    node.return_type->accept(*this);
    popIndent();
    if (!node.parameters.empty()) {
        out << indent() << "parameters:" << std::endl;
        pushIndent();
        for (const auto& param : node.parameters) {
            out << indent() << param.name << " :" << std::endl;
            pushIndent();
            param.type->accept(*this);
            popIndent();
        }
        popIndent();
    }
    if (node.body) {
        out << indent() << "body:" << std::endl;
        pushIndent();
        node.body->accept(*this);
        popIndent();
    }
    popIndent();
}

void ASTPrinter::visit(VariableDeclaration& node) {
    out << indent() << "VariableDeclaration(" << node.name;
    if (node.is_mutable) {
        out << ", mutable";
    } else {
        out << ", const";
    }
    out << ")" << std::endl;
    if (node.type) {
        pushIndent();
        out << indent() << "type:" << std::endl;
        pushIndent();
        node.type->accept(*this);
        popIndent();
        if (node.initializer) {
            out << indent() << "initializer:" << std::endl;
            pushIndent();
            node.initializer->accept(*this);
            popIndent();
        }
        popIndent();
    }
}

void ASTPrinter::visit(ClassDeclaration& node) {
    out << indent() << "ClassDeclaration(" << node.name << ")" << std::endl;
    if (!node.members.empty()) {
        pushIndent();
        out << indent() << "members:" << std::endl;
        pushIndent();
        for (size_t i = 0; i < node.members.size(); ++i) {
            out << indent() << "[" << i << "]: " << node.members[i]->name << std::endl;
        }
        popIndent();
        popIndent();
    }
}

void ASTPrinter::visit(StructDeclaration& node) {
    out << indent() << "StructDeclaration(" << node.name << ")" << std::endl;
    if (!node.fields.empty()) {
        pushIndent();
        out << indent() << "fields:" << std::endl;
        pushIndent();
        for (const auto& field : node.fields) {
            out << indent() << field.name << " :" << std::endl;
            pushIndent();
            field.type->accept(*this);
            popIndent();
        }
        popIndent();
        popIndent();
    }
}

void ASTPrinter::visit(EnumDeclaration& node) {
    out << indent() << "EnumDeclaration(" << node.name << ")" << std::endl;
    if (!node.variants.empty()) {
        pushIndent();
        out << indent() << "variants:" << std::endl;
        pushIndent();
        for (size_t i = 0; i < node.variants.size(); ++i) {
            out << indent() << "[" << i << "]: " << node.variants[i].name << std::endl;
        }
        popIndent();
        popIndent();
    }
}

void ASTPrinter::visit(Module& node) {
    out << indent() << "Module(" << node.module_name << ", " << node.file_path << ")" << std::endl;
    if (!node.imports.empty() || !node.declarations.empty()) {
        pushIndent();
        if (!node.imports.empty()) {
            out << indent() << "imports:" << std::endl;
            pushIndent();
            for (size_t i = 0; i < node.imports.size(); ++i) {
                out << indent() << "[" << i << "]:" << std::endl;
                pushIndent();
                node.imports[i]->accept(*this);
                popIndent();
            }
            popIndent();
        }
        if (!node.declarations.empty()) {
            out << indent() << "declarations:" << std::endl;
            pushIndent();
            for (size_t i = 0; i < node.declarations.size(); ++i) {
                out << indent() << "[" << i << "]:" << std::endl;
                pushIndent();
                node.declarations[i]->accept(*this);
                popIndent();
            }
            popIndent();
        }
        popIndent();
    }
}

void ASTPrinter::visit(Program& node) {
    out << "Program" << std::endl;
    pushIndent();
    if (!node.modules.empty()) {
        out << indent() << "modules:" << std::endl;
        pushIndent();
        for (size_t i = 0; i < node.modules.size(); ++i) {
            out << indent() << "[" << i << "]:" << std::endl;
            pushIndent();
            node.modules[i]->accept(*this);
            popIndent();
        }
        popIndent();
    }
    if (node.main_module) {
        out << indent() << "main_module:" << std::endl;
        pushIndent();
        node.main_module->accept(*this);
        popIndent();
    }
    popIndent();
}

} // namespace pangea
