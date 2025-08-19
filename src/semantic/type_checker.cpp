#include "type_checker.h"
#include "../builtins/builtins.h"
#include <sstream>
#include <unordered_set>

namespace pangea {

// SemanticType implementation
bool SemanticType::isCompatibleWith(const SemanticType& other) const {
    if (kind == Kind::ERROR_TYPE || other.kind == Kind::ERROR_TYPE) {
        return true; // Error types are compatible with everything to avoid cascading errors
    }
    
    if (kind != other.kind) {
        return false;
    }
    
    switch (kind) {
        case Kind::PRIMITIVE:
        case Kind::VOID_TYPE:
            return name == other.name;
            
        case Kind::ARRAY:
            return element_type && other.element_type && 
                   element_type->isCompatibleWith(*other.element_type);
                   
        case Kind::POINTER:
            return element_type && other.element_type && 
                   element_type->isCompatibleWith(*other.element_type);
                   
        case Kind::FUNCTION:
            if (!return_type || !other.return_type || 
                !return_type->isCompatibleWith(*other.return_type)) {
                return false;
            }
            if (parameter_types.size() != other.parameter_types.size()) {
                return false;
            }
            for (size_t i = 0; i < parameter_types.size(); ++i) {
                if (!parameter_types[i]->isCompatibleWith(*other.parameter_types[i])) {
                    return false;
                }
            }
            return true;
            
        default:
            return false;
    }
}

std::string SemanticType::toString() const {
    switch (kind) {
        case Kind::PRIMITIVE:
        case Kind::VOID_TYPE:
            return name;
            
        case Kind::ARRAY:
            return "[" + (element_type ? element_type->toString() : "unknown") + "]";
            
        case Kind::POINTER:
            return "*" + (element_type ? element_type->toString() : "unknown");
            
        case Kind::FUNCTION: {
            std::ostringstream oss;
            oss << "fn(";
            for (size_t i = 0; i < parameter_types.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << parameter_types[i]->toString();
            }
            oss << ") -> " << (return_type ? return_type->toString() : "unknown");
            return oss.str();
        }
        
        case Kind::ERROR_TYPE:
            return "<error>";
            
        default:
            return "<unknown>";
    }
}

std::unique_ptr<SemanticType> SemanticType::createPrimitive(const std::string& name) {
    return std::make_unique<SemanticType>(Kind::PRIMITIVE, name);
}

std::unique_ptr<SemanticType> SemanticType::createArray(std::unique_ptr<SemanticType> element) {
    auto type = std::make_unique<SemanticType>(Kind::ARRAY);
    type->element_type = std::move(element);
    return type;
}

std::unique_ptr<SemanticType> SemanticType::createPointer(std::unique_ptr<SemanticType> pointee) {
    auto type = std::make_unique<SemanticType>(Kind::POINTER);
    type->element_type = std::move(pointee);
    return type;
}

std::unique_ptr<SemanticType> SemanticType::createFunction(
    std::vector<std::unique_ptr<SemanticType>> params,
    std::unique_ptr<SemanticType> ret_type) {
    auto type = std::make_unique<SemanticType>(Kind::FUNCTION);
    type->parameter_types = std::move(params);
    type->return_type = std::move(ret_type);
    return type;
}

std::unique_ptr<SemanticType> SemanticType::createVoid() {
    return std::make_unique<SemanticType>(Kind::VOID_TYPE, "void");
}

std::unique_ptr<SemanticType> SemanticType::createError() {
    return std::make_unique<SemanticType>(Kind::ERROR_TYPE, "<error>");
}

// Scope implementation
void Scope::define(const std::string& name, std::unique_ptr<Symbol> symbol) {
    symbols[name] = std::move(symbol);
}

Symbol* Scope::lookup(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second.get();
    }
    
    if (parent) {
        return parent->lookup(name);
    }
    
    return nullptr;
}

bool Scope::isDefined(const std::string& name) const {
    return symbols.find(name) != symbols.end();
}

// TypeChecker implementation
TypeChecker::TypeChecker(ErrorReporter* reporter, bool enable_builtins) 
    : error_reporter(reporter), global_scope(std::make_unique<Scope>()), current_scope(global_scope.get()) {
    initializeBuiltinTypes();
    
    // Register built-in functions only if enabled
    if (enable_builtins) {
        builtins::getBuiltinsRegistry().registerWithTypeChecker(*this);
    }
}

void TypeChecker::analyze(Program& program) {
    program.accept(*this);
}

void TypeChecker::visit(PrimitiveType& node) {
    // Type nodes don't need semantic analysis themselves
}

void TypeChecker::visit(ArrayType& node) {
    if (node.element_type) {
        node.element_type->accept(*this);
    }
    if (!node.size) {
        reportTypeError(node.location, "Array size must be specified");
    }
}

void TypeChecker::visit(PointerType& node) {
    if (node.pointee_type) {
        node.pointee_type->accept(*this);
    }
}

void TypeChecker::visit(LiteralExpression& node) {
    std::unique_ptr<SemanticType> type;
    
    switch (node.literal_token.type) {
        case TokenType::INTEGER_LITERAL:
            // Default integer literals to i32
            type = SemanticType::createPrimitive("i32");
            break;
        case TokenType::FLOAT_LITERAL:
            // Default float literals to f64
            type = SemanticType::createPrimitive("f64");
            break;
        case TokenType::BOOLEAN_LITERAL:
            type = SemanticType::createPrimitive("bool");
            break;
        case TokenType::STRING_LITERAL:
            type = SemanticType::createPrimitive("string");
            break;
        case TokenType::NULL_LITERAL:
            type = SemanticType::createPrimitive("null");
            break;
        default:
            type = SemanticType::createError();
            reportTypeError(node.location, "Unknown literal type");
            break;
    }
    
    setExpressionType(node, std::move(type));
}

void TypeChecker::visit(IdentifierExpression& node) {
    Symbol* symbol = current_scope->lookup(node.name);
    if (!symbol) {
        reportTypeError(node.location, "Undefined identifier: " + node.name);
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // Clone the type for this expression
    setExpressionType(node, std::make_unique<SemanticType>(*symbol->type));
}

void TypeChecker::visit(BinaryExpression& node) {
    node.left->accept(*this);
    node.right->accept(*this);
    
    auto left_type = getExpressionType(*node.left);
    auto right_type = getExpressionType(*node.right);
    
    if (!left_type || !right_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    std::unique_ptr<SemanticType> result_type;
    
    switch (node.operator_token) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
        case TokenType::MODULO:
        case TokenType::POWER:
            if (left_type->isCompatibleWith(*right_type) && 
                (left_type->name == "i8" || left_type->name == "i16" || left_type->name == "i32" || left_type->name == "i64" ||
                 left_type->name == "f32" || left_type->name == "f64")) {
                result_type = std::make_unique<SemanticType>(*left_type);
            } else {
                reportTypeError(node.location, "Invalid operands for arithmetic operation");
                result_type = SemanticType::createError();
            }
            break;
            
        case TokenType::BITWISE_LEFT_SHIFT:
        case TokenType::BITWISE_RIGHT_SHIFT:
            if (left_type->isCompatibleWith(*right_type) && 
                (left_type->name == "i8" || left_type->name == "i16" || left_type->name == "i32" || left_type->name == "i64" ||
                 left_type->name == "u8" || left_type->name == "u16" || left_type->name == "u32" || left_type->name == "u64")) {
                result_type = std::make_unique<SemanticType>(*left_type);
            } else {
                reportTypeError(node.location, "Invalid operands for bitwise shift operation");
                result_type = SemanticType::createError();
            }
            break;
            
        case TokenType::EQUAL:
        case TokenType::NOT_EQUAL:
        case TokenType::LESS:
        case TokenType::LESS_EQUAL:
        case TokenType::GREATER:
        case TokenType::GREATER_EQUAL:
            if (left_type->isCompatibleWith(*right_type)) {
                result_type = SemanticType::createPrimitive("bool");
            } else {
                reportTypeError(node.location, "Cannot compare incompatible types");
                result_type = SemanticType::createError();
            }
            break;
            
        case TokenType::LOGICAL_AND:
        case TokenType::LOGICAL_OR:
            if (left_type->name == "bool" && right_type->name == "bool") {
                result_type = SemanticType::createPrimitive("bool");
            } else if (left_type->isCompatibleWith(*right_type) && 
                      (left_type->name == "i8" || left_type->name == "i16" || left_type->name == "i32" || left_type->name == "i64" ||
                       left_type->name == "u8" || left_type->name == "u16" || left_type->name == "u32" || left_type->name == "u64" ||
                       left_type->name == "f32" || left_type->name == "f64")) {
                // Allow logical operators on numeric types (treat non-zero as true)
                result_type = SemanticType::createPrimitive("bool");
            } else {
                reportTypeError(node.location, "Logical operators require boolean or numeric operands");
                result_type = SemanticType::createError();
            }
            break;
            
        default:
            reportTypeError(node.location, "Unknown binary operator");
            result_type = SemanticType::createError();
            break;
    }
    
    setExpressionType(node, std::move(result_type));
}

void TypeChecker::visit(UnaryExpression& node) {
    node.operand->accept(*this);
    
    auto operand_type = getExpressionType(*node.operand);
    if (!operand_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    std::unique_ptr<SemanticType> result_type;
    
    switch (node.operator_token) {
        case TokenType::MINUS:
            if (operand_type->name == "i8" || operand_type->name == "i16" || operand_type->name == "i32" || operand_type->name == "i64" ||
                operand_type->name == "f32" || operand_type->name == "f64") {
                result_type = std::make_unique<SemanticType>(*operand_type);
            } else {
                reportTypeError(node.location, "Unary minus requires numeric operand");
                result_type = SemanticType::createError();
            }
            break;
            
        case TokenType::LOGICAL_NOT:
            if (operand_type->name == "bool") {
                result_type = SemanticType::createPrimitive("bool");
            } else if (operand_type->name == "i8" || operand_type->name == "i16" || operand_type->name == "i32" || operand_type->name == "i64" ||
                       operand_type->name == "u8" || operand_type->name == "u16" || operand_type->name == "u32" || operand_type->name == "u64" ||
                       operand_type->name == "f32" || operand_type->name == "f64") {
                // Logical not on numeric types: !0 = true, !nonzero = false
                result_type = SemanticType::createPrimitive("bool");
            } else {
                reportTypeError(node.location, "Logical not requires boolean or numeric operand");
                result_type = SemanticType::createError();
            }
            break;
            
        default:
            reportTypeError(node.location, "Unknown unary operator");
            result_type = SemanticType::createError();
            break;
    }
    
    setExpressionType(node, std::move(result_type));
}

void TypeChecker::visit(CallExpression& node) {
    node.callee->accept(*this);
    
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
    
    // Special handling for method calls (member expressions)
    if (auto member_expr = dynamic_cast<MemberExpression*>(node.callee.get())) {
        auto object_type = getExpressionType(*member_expr->object);
        // Dynamically resolve method calls based on object type
        // This would be implemented by looking up the method in the type's method table
        if (object_type) {
            // For now, assume method calls are valid and return a placeholder type
            // In a full implementation, we'd look up the method signature
            setExpressionType(node, SemanticType::createPrimitive("unknown"));
        } else {
            setExpressionType(node, SemanticType::createError());
        }
        return;
    }
    
    auto callee_type = getExpressionType(*node.callee);
    if (!callee_type || callee_type->kind != SemanticType::Kind::FUNCTION) {
        reportTypeError(node.location, "Cannot call non-function");
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // Handle variadic functions (foreign functions with raw_va_list)
    if (auto callee_id = dynamic_cast<IdentifierExpression*>(node.callee.get())) {
        // Check if this is a foreign variadic function (like printf)
        if (isForeignVariadicFunction(callee_id->name)) {
            // Variadic foreign functions accept any number of compatible arguments
            // Basic type checking for variadic arguments
            for (auto& arg : node.arguments) {
                auto arg_type = getExpressionType(*arg);
                if (arg_type && !isVariadicCompatible(*arg_type)) {
                    reportTypeError(arg->location, 
                        "Argument type not compatible with variadic function: " + arg_type->toString());
                }
            }
            setExpressionType(node, std::make_unique<SemanticType>(*callee_type->return_type));
            return;
        }
    }
    
    // Check argument count for non-variadic functions
    if (node.arguments.size() != callee_type->parameter_types.size()) {
        reportTypeError(node.location, "Incorrect number of arguments");
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // Check argument types with special handling for string-to-cptr conversions
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        auto arg_type = getExpressionType(*node.arguments[i]);
        auto expected_type = callee_type->parameter_types[i].get();
        
        if (arg_type && !isTypeCompatibleWithParameter(*arg_type, *expected_type)) {
            reportTypeError(node.arguments[i]->location, 
                "Argument type mismatch: expected " + expected_type->toString() +
                ", got " + arg_type->toString());
        }
    }
    
    setExpressionType(node, std::make_unique<SemanticType>(*callee_type->return_type));
}

void TypeChecker::visit(MemberExpression& node) {
    node.object->accept(*this);
    
    auto object_type = getExpressionType(*node.object);
    if (!object_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // TODO: implement member access for class instances
    // by detecting members of class programmatically
    if (false) {
        // setExpressionType(node, SemanticType::createPrimitive(type));
    } else {
        reportTypeError(node.location, "Member access not supported for type: " + object_type->toString());
        setExpressionType(node, SemanticType::createError());
    }
}

void TypeChecker::visit(IndexExpression& node) {
    node.object->accept(*this);
    node.index->accept(*this);
    
    auto object_type = getExpressionType(*node.object);
    auto index_type = getExpressionType(*node.index);
    
    if (!object_type || !index_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    if (object_type->kind != SemanticType::Kind::ARRAY) {
        reportTypeError(node.location, "Cannot index non-array type");
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    if (!index_type->isCompatibleWith(*SemanticType::createPrimitive("int"))) {
        reportTypeError(node.location, "Array index must be integer");
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    setExpressionType(node, std::make_unique<SemanticType>(*object_type->element_type));
}

void TypeChecker::visit(AssignmentExpression& node) {
    node.left->accept(*this);
    node.right->accept(*this);
    
    auto left_type = getExpressionType(*node.left);
    auto right_type = getExpressionType(*node.right);
    
    if (!left_type || !right_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // Check if left side is assignable (for now, just check if it's an identifier)
    if (auto identifier = dynamic_cast<IdentifierExpression*>(node.left.get())) {
        Symbol* symbol = current_scope->lookup(identifier->name);
        if (symbol && !symbol->is_mutable) {
            reportTypeError(node.location, "Cannot assign to immutable variable: " + identifier->name);
        }
    }
    
    // For compound assignments, check that the operation is valid
    if (node.operator_token != TokenType::ASSIGN) {
        // For +=, -=, *=, /=, %= the types should be compatible for the underlying operation
        if (!left_type->isCompatibleWith(*right_type)) {
            reportTypeError(node.location, "Type mismatch in compound assignment");
            setExpressionType(node, SemanticType::createError());
            return;
        }
    } else {
        // For simple assignment, right type should be compatible with left type
        if (!right_type->isCompatibleWith(*left_type)) {
            reportTypeError(node.location, 
                "Type mismatch in assignment: expected " + left_type->toString() +
                ", got " + right_type->toString());
            setExpressionType(node, SemanticType::createError());
            return;
        }
    }
    
    // Assignment expression evaluates to the assigned value
    setExpressionType(node, std::make_unique<SemanticType>(*left_type));
}

void TypeChecker::visit(PostfixExpression& node) {
    node.operand->accept(*this);
    
    auto operand_type = getExpressionType(*node.operand);
    if (!operand_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // Check if operand is assignable (for now, just check if it's an identifier)
    if (auto identifier = dynamic_cast<IdentifierExpression*>(node.operand.get())) {
        Symbol* symbol = current_scope->lookup(identifier->name);
        if (symbol && !symbol->is_mutable) {
            reportTypeError(node.location, "Cannot modify immutable variable: " + identifier->name);
        }
    }
    
    // Check that the operand type supports increment/decrement
    if (!(operand_type->name == "i8" || operand_type->name == "i16" || operand_type->name == "i32" || operand_type->name == "i64" ||
          operand_type->name == "u8" || operand_type->name == "u16" || operand_type->name == "u32" || operand_type->name == "u64" ||
          operand_type->name == "f32" || operand_type->name == "f64")) {
        reportTypeError(node.location, "Increment/decrement requires numeric operand");
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // Postfix increment/decrement returns the original value
    setExpressionType(node, std::make_unique<SemanticType>(*operand_type));
}

void TypeChecker::visit(CastExpression& node) {
    // Analyze the expression being cast
    node.expression->accept(*this);
    
    // Convert the target type
    auto target_type = convertASTType(*node.target_type);
    
    auto source_type = getExpressionType(*node.expression);
    if (!source_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // Check if the cast is valid
    bool is_valid_cast = false;
    
    // Allow casting between primitive numeric types
    if (source_type->kind == SemanticType::Kind::PRIMITIVE && target_type->kind == SemanticType::Kind::PRIMITIVE) {
        std::vector<std::string> numeric_types = {"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64"};
        std::vector<std::string> all_castable_types = {"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64", "bool", "string"};
        
        bool source_is_numeric = std::find(numeric_types.begin(), numeric_types.end(), source_type->name) != numeric_types.end();
        bool target_is_numeric = std::find(numeric_types.begin(), numeric_types.end(), target_type->name) != numeric_types.end();
        bool source_is_castable = std::find(all_castable_types.begin(), all_castable_types.end(), source_type->name) != all_castable_types.end();
        bool target_is_castable = std::find(all_castable_types.begin(), all_castable_types.end(), target_type->name) != all_castable_types.end();
        
        // Allow casting between any castable types
        if (source_is_castable && target_is_castable) {
            is_valid_cast = true;
        }
    }
    
    if (!is_valid_cast) {
        if (node.is_safe_cast) {
            // For try_cast, invalid casts return the original value
            reportTypeError(node.location, "try_cast failed: cannot cast from " + source_type->toString() + " to " + target_type->toString(), true);
            setExpressionType(node, std::make_unique<SemanticType>(*source_type));
        } else {
            // For cast, invalid casts are warnings but still proceed
            reportTypeError(node.location, "Warning: potentially unsafe cast from " + source_type->toString() + " to " + target_type->toString(), true);
            setExpressionType(node, std::move(target_type));
        }
    } else {
        // Valid cast
        setExpressionType(node, std::move(target_type));
    }
}

void TypeChecker::visit(AsExpression& node) {
    // Analyze the expression being cast
    node.expression->accept(*this);
    
    // Convert the target type
    auto target_type = convertASTType(*node.target_type);
    
    auto source_type = getExpressionType(*node.expression);
    if (!source_type) {
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // 'as' operator is equivalent to cast<T>(x) - always succeeds but may truncate
    std::vector<std::string> all_castable_types = {"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64", "bool", "string"};
    bool source_is_castable = std::find(all_castable_types.begin(), all_castable_types.end(), source_type->name) != all_castable_types.end();
    bool target_is_castable = std::find(all_castable_types.begin(), all_castable_types.end(), target_type->name) != all_castable_types.end();
    
    if (!source_is_castable || !target_is_castable) {
        reportTypeError(node.location, "Cannot cast from " + source_type->toString() + " to " + target_type->toString() + " using 'as' operator");
        setExpressionType(node, SemanticType::createError());
        return;
    }
    
    // 'as' cast always succeeds
    setExpressionType(node, std::move(target_type));
}

void TypeChecker::visit(ExpressionStatement& node) {
    node.expression->accept(*this);
}

void TypeChecker::visit(BlockStatement& node) {
    enterScope();
    
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    
    exitScope();
}

void TypeChecker::visit(IfStatement& node) {
    node.condition->accept(*this);
    
    auto condition_type = getExpressionType(*node.condition);
    if (condition_type && !condition_type->isCompatibleWith(*SemanticType::createPrimitive("bool"))) {
        reportTypeError(node.condition->location, "If condition must be boolean");
    }
    
    node.then_branch->accept(*this);
    
    if (node.else_branch) {
        node.else_branch->accept(*this);
    }
}

void TypeChecker::visit(WhileStatement& node) {
    node.condition->accept(*this);
    
    auto condition_type = getExpressionType(*node.condition);
    if (condition_type && !condition_type->isCompatibleWith(*SemanticType::createPrimitive("bool"))) {
        reportTypeError(node.condition->location, "While condition must be boolean");
    }
    
    node.body->accept(*this);
}

void TypeChecker::visit(ForStatement& node) {
    // For now, simplified - would need iterator protocol implementation
    node.iterable->accept(*this);
    
    enterScope();
    
    // Define iterator variable (simplified as int for now)
    auto iterator_symbol = std::make_unique<Symbol>(
        node.iterator_name, 
        SemanticType::createPrimitive("int"), 
        false, 
        node.location
    );
    iterator_symbol->is_initialized = true;
    current_scope->define(node.iterator_name, std::move(iterator_symbol));
    
    node.body->accept(*this);
    
    exitScope();
}

void TypeChecker::visit(ReturnStatement& node) {
    if (node.value) {
        node.value->accept(*this);
        
        if (current_function_return_type) {
            auto return_type = getExpressionType(*node.value);
            if (return_type && !return_type->isCompatibleWith(*current_function_return_type)) {
                reportTypeError(node.location, 
                    "Return type mismatch: expected " + current_function_return_type->toString() +
                    ", got " + return_type->toString());
            }
        }
    } else {
        if (current_function_return_type && current_function_return_type->kind != SemanticType::Kind::VOID_TYPE) {
            reportTypeError(node.location, "Missing return value");
        }
    }
}

void TypeChecker::visit(DeclarationStatement& node) {
    if (node.declaration) {
        node.declaration->accept(*this);
    }
}

void TypeChecker::visit(FunctionDeclaration& node) {
    // Convert parameter types
    std::vector<std::unique_ptr<SemanticType>> param_types;
    for (auto& param : node.parameters) {
        param_types.push_back(convertASTType(*param.type));
    }
    
    // Convert return type
    auto return_type = convertASTType(*node.return_type);
    
    // Keep a copy of return type for function body checking
    auto return_type_copy = std::make_unique<SemanticType>(*return_type);
    
    // Create function type
    auto function_type = SemanticType::createFunction(std::move(param_types), std::move(return_type));
    
    // Define function in current scope
    auto function_symbol = std::make_unique<Symbol>(
        node.name, 
        std::move(function_type), 
        false, 
        node.location
    );
    function_symbol->is_initialized = true;
    current_scope->define(node.name, std::move(function_symbol));
    
    // Only analyze function body for non-foreign functions
    if (!node.is_foreign && node.body) {
        // Enter function scope
        enterScope();
        
        // Define parameters
        for (auto& param : node.parameters) {
            auto param_type = convertASTType(*param.type);
            auto param_symbol = std::make_unique<Symbol>(
                param.name, 
                std::move(param_type), 
                false, 
                param.location
            );
            param_symbol->is_initialized = true;
            current_scope->define(param.name, std::move(param_symbol));
        }
        
        // Set current function return type for return statement checking
        auto old_return_type = current_function_return_type;
        current_function_return_type = return_type_copy.get();
        
        // Analyze function body
        node.body->accept(*this);
        
        // Restore previous function return type
        current_function_return_type = old_return_type;
        
        exitScope();
    }
    // Foreign functions don't have bodies to analyze
}

void TypeChecker::visit(VariableDeclaration& node) {
    std::unique_ptr<SemanticType> var_type;
    
    if (node.type) {
        var_type = convertASTType(*node.type);
    }
    
    if (node.initializer) {
        node.initializer->accept(*this);
        auto init_type = getExpressionType(*node.initializer);
        
        if (var_type && init_type) {
            if (!init_type->isCompatibleWith(*var_type)) {
                reportTypeError(node.location, 
                    "Type mismatch in variable initialization: expected " + var_type->toString() +
                    ", got " + init_type->toString());
            }
        } else if (!var_type && init_type) {
            // Type inference
            var_type = std::make_unique<SemanticType>(*init_type);
        }
    }
    
    if (!var_type) {
        reportTypeError(node.location, "Cannot infer type for variable " + node.name);
        var_type = SemanticType::createError();
    }
    
    // Check for redefinition in current scope
    if (current_scope->isDefined(node.name)) {
        reportTypeError(node.location, "Redefinition of variable " + node.name);
        return;
    }
    
    auto symbol = std::make_unique<Symbol>(
        node.name, 
        std::move(var_type), 
        node.is_mutable, 
        node.location
    );
    
    if (node.initializer) {
        symbol->is_initialized = true;
    }
    
    current_scope->define(node.name, std::move(symbol));
}

void TypeChecker::visit(ImportDeclaration& node) {
    // Import declarations don't need type checking themselves
    // Symbol resolution is handled at the module loading stage
}

void TypeChecker::visit(Module& node) {
    // Process all imports first (for symbol resolution)
    for (auto& import : node.imports) {
        import->accept(*this);
    }
    
    // Then process all declarations in the module
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void TypeChecker::visit(Program& node) {
    // Process all modules
    for (auto& module : node.modules) {
        module->accept(*this);
    }
    
    // Process the main module
    if (node.main_module) {
        node.main_module->accept(*this);
    }
}

void TypeChecker::visit(GenericType& node) {
    // For now, just treat generic types as placeholders
    // Full generic support would require template instantiation
    // We'll register the base type for now
}

void TypeChecker::visit(ClassDeclaration& node) {
    // Register the class as a new type in the current scope
    auto class_type = SemanticType::createPrimitive(node.name);
    auto class_symbol = std::make_unique<Symbol>(
        node.name,
        std::move(class_type),
        false,
        node.location
    );
    class_symbol->is_initialized = true;
    current_scope->define(node.name, std::move(class_symbol));
    
    // Also register the class as a constructor function
    // Constructor takes the field parameters and returns an instance of the class
    std::vector<std::unique_ptr<SemanticType>> constructor_params;
    
    // Find constructor parameters from the constructor method
    for (auto& member : node.members) {
        if (auto method = dynamic_cast<MethodMember*>(member.get())) {
            if (method->name == node.name) { // Constructor has same name as class
                for (auto& param : method->parameters) {
                    constructor_params.push_back(convertASTType(*param.type));
                }
                break;
            }
        }
    }
    
    auto constructor_return_type = SemanticType::createPrimitive(node.name);
    auto constructor_type = SemanticType::createFunction(std::move(constructor_params), std::move(constructor_return_type));
    
    // Register constructor as a function with the class name
    auto constructor_symbol = std::make_unique<Symbol>(
        node.name,
        std::move(constructor_type),
        false,
        node.location
    );
    constructor_symbol->is_initialized = true;
    current_scope->define(node.name, std::move(constructor_symbol));
    
    // Enter class scope for methods
    enterScope();
    
    // Process class members (methods and fields)
    for (auto& member : node.members) {
        if (auto method = dynamic_cast<MethodMember*>(member.get())) {
            // Convert method parameters
            std::vector<std::unique_ptr<SemanticType>> param_types;
            for (auto& param : method->parameters) {
                param_types.push_back(convertASTType(*param.type));
            }
            
            // Convert return type
            auto return_type = convertASTType(*method->return_type);
            auto return_type_copy = std::make_unique<SemanticType>(*return_type);
            
            // Create method type
            auto method_type = SemanticType::createFunction(std::move(param_types), std::move(return_type));
            
            // Register method in class scope
            auto method_symbol = std::make_unique<Symbol>(
                method->name,
                std::move(method_type),
                false,
                method->location
            );
            method_symbol->is_initialized = true;
            current_scope->define(method->name, std::move(method_symbol));
            
            // Analyze method body
            enterScope();
            
            // Define method parameters (including self)
            for (auto& param : method->parameters) {
                auto param_type = convertASTType(*param.type);
                
                // Special handling for 'self' parameter
                if (param.name == "self") {
                    param_type = SemanticType::createPrimitive(node.name);
                }
                
                auto param_symbol = std::make_unique<Symbol>(
                    param.name,
                    std::move(param_type),
                    false,
                    param.location
                );
                param_symbol->is_initialized = true;
                current_scope->define(param.name, std::move(param_symbol));
            }
            
            // For constructors, also define 'self' if it's not already a parameter
            if (method->name == node.name) { // This is a constructor
                bool has_self_param = false;
                for (auto& param : method->parameters) {
                    if (param.name == "self") {
                        has_self_param = true;
                        break;
                    }
                }
                
                if (!has_self_param) {
                    // Define 'self' as an instance of the class
                    auto self_type = SemanticType::createPrimitive(node.name);
                    auto self_symbol = std::make_unique<Symbol>(
                        "self",
                        std::move(self_type),
                        true, // self is mutable in constructors
                        method->location
                    );
                    self_symbol->is_initialized = true;
                    current_scope->define("self", std::move(self_symbol));
                }
            }
            
            // Set current function return type
            auto old_return_type = current_function_return_type;
            current_function_return_type = return_type_copy.get();
            
            // Analyze method body
            method->body->accept(*this);
            
            // Restore previous function return type
            current_function_return_type = old_return_type;
            
            exitScope();
        }
        else if (auto field = dynamic_cast<FieldMember*>(member.get())) {
            // Process field declarations - they don't need analysis here
            // but we could validate their types
            auto field_type = convertASTType(*field->type);
            if (!field_type || field_type->kind == SemanticType::Kind::ERROR_TYPE) {
                reportTypeError(field->location, "Invalid field type: " + field->name);
            }
        }
    }
    
    exitScope();
}

void TypeChecker::visit(StructDeclaration& node) {
    // Register the struct as a new type in the current scope
    auto struct_type = SemanticType::createPrimitive(node.name);
    auto struct_symbol = std::make_unique<Symbol>(
        node.name,
        std::move(struct_type),
        false,
        node.location
    );
    struct_symbol->is_initialized = true;
    current_scope->define(node.name, std::move(struct_symbol));
    
    // Validate field types
    for (auto& field : node.fields) {
        auto field_type = convertASTType(*field.type);
        if (!field_type || field_type->kind == SemanticType::Kind::ERROR_TYPE) {
            reportTypeError(field.location, "Invalid field type: " + field.name);
        }
    }
}

void TypeChecker::visit(EnumDeclaration& node) {
    // Register the enum as a new type in the current scope
    auto enum_type = SemanticType::createPrimitive(node.name);
    auto enum_symbol = std::make_unique<Symbol>(
        node.name,
        std::move(enum_type),
        false,
        node.location
    );
    enum_symbol->is_initialized = true;
    current_scope->define(node.name, std::move(enum_symbol));
    
    // Register each variant as a constant of the enum type
    for (auto& variant : node.variants) {
        auto variant_type = SemanticType::createPrimitive(node.name);
        auto variant_symbol = std::make_unique<Symbol>(
            variant.name,
            std::move(variant_type),
            false,
            variant.location
        );
        variant_symbol->is_initialized = true;
        current_scope->define(variant.name, std::move(variant_symbol));
    }
}

void TypeChecker::enterScope() {
    auto new_scope = std::make_unique<Scope>(current_scope);
    current_scope = new_scope.get();
    scope_stack.push_back(std::move(new_scope));
}

void TypeChecker::exitScope() {
    if (current_scope->getParent()) {
        current_scope = current_scope->getParent();
        if (!scope_stack.empty()) {
            scope_stack.pop_back();
        }
    }
}

std::unique_ptr<SemanticType> TypeChecker::convertASTType(Type& ast_type) {
    if (auto primitive = dynamic_cast<PrimitiveType*>(&ast_type)) {
        switch (primitive->type_token) {
            case TokenType::I8: return SemanticType::createPrimitive("i8");
            case TokenType::I16: return SemanticType::createPrimitive("i16");
            case TokenType::I32: return SemanticType::createPrimitive("i32");
            case TokenType::I64: return SemanticType::createPrimitive("i64");
            case TokenType::U8: return SemanticType::createPrimitive("u8");
            case TokenType::U16: return SemanticType::createPrimitive("u16");
            case TokenType::U32: return SemanticType::createPrimitive("u32");
            case TokenType::U64: return SemanticType::createPrimitive("u64");
            case TokenType::F32: return SemanticType::createPrimitive("f32");
            case TokenType::F64: return SemanticType::createPrimitive("f64");
            case TokenType::BOOL: return SemanticType::createPrimitive("bool");
            case TokenType::STRING: return SemanticType::createPrimitive("string");
            case TokenType::VOID: return SemanticType::createVoid();
            case TokenType::SELF: return SemanticType::createPrimitive("self");
            case TokenType::RAW_VA_LIST: return SemanticType::createPrimitive("raw_va_list");
            case TokenType::IDENTIFIER: 
                // Handle user-defined types (classes, structs, enums)
                return SemanticType::createPrimitive("UserDefinedType");
            default: return SemanticType::createError();
        }
    } else if (auto array = dynamic_cast<ArrayType*>(&ast_type)) {
        auto element_type = convertASTType(*array->element_type);
        return SemanticType::createArray(std::move(element_type));
    } else if (auto pointer = dynamic_cast<PointerType*>(&ast_type)) {
        auto pointee_type = convertASTType(*pointer->pointee_type);
        
        // Handle different pointer types
        std::string pointer_type_name;
        switch (pointer->pointer_kind) {
            case TokenType::CPTR:
                pointer_type_name = "cptr";
                break;
            case TokenType::UNIQUE:
                pointer_type_name = "unique_ptr";
                break;
            case TokenType::SHARED:
                pointer_type_name = "shared_ptr";
                break;
            case TokenType::WEAK:
                pointer_type_name = "weak_ptr";
                break;
            default:
                pointer_type_name = "unknown_ptr";
                break;
        }
        
        // Create a specialized pointer type that includes the pointer kind information
        auto ptr_type = SemanticType::createPointer(std::move(pointee_type));
        ptr_type->name = pointer_type_name; // Override the name to include pointer kind
        return ptr_type;
    } else if (auto generic = dynamic_cast<GenericType*>(&ast_type)) {
        // For now, treat generic types as the base type
        return SemanticType::createPrimitive(generic->base_name);
    }
    
    return SemanticType::createError();
}

std::unique_ptr<SemanticType> TypeChecker::getExpressionType(Expression& expr) {
    auto it = expression_types.find(&expr);
    if (it != expression_types.end()) {
        return std::make_unique<SemanticType>(*it->second);
    }
    return nullptr;
}

void TypeChecker::setExpressionType(Expression& expr, std::unique_ptr<SemanticType> type) {
    expression_types[&expr] = std::move(type);
}

bool TypeChecker::checkTypeCompatibility(const SemanticType& expected, const SemanticType& actual) {
    return actual.isCompatibleWith(expected);
}

void TypeChecker::reportTypeError(const SourceLocation& location, const std::string& message, const bool is_warning) {
    if (error_reporter) {
        error_reporter->reportError(location, message, is_warning);
    }
}

void TypeChecker::initializeBuiltinTypes() {
    // Built-in types are created on-demand in convertASTType
    // This could be expanded to include built-in functions, constants, etc.
}

void TypeChecker::registerBuiltinFunction(const std::string& name, const std::string& return_type,
                                        const std::vector<std::pair<std::string, std::string>>& parameters) {
    // Special handling for variadic functions (like print)
    if (parameters.empty() && name == "print") {
        // For print function, create a special variadic function type
        // We'll handle this specially in CallExpression visitor
        auto ret_type = SemanticType::createVoid();
        auto function_type = SemanticType::createFunction({}, std::move(ret_type));
        
        auto function_symbol = std::make_unique<Symbol>(
            name, 
            std::move(function_type), 
            false, 
            SourceLocation{} // Built-ins don't have source locations
        );
        function_symbol->is_initialized = true;
        global_scope->define(name, std::move(function_symbol));
        return;
    }
    
    // Convert parameter types
    std::vector<std::unique_ptr<SemanticType>> param_types;
    for (const auto& param : parameters) {
        if (param.second == "int") {
            param_types.push_back(SemanticType::createPrimitive("int"));
        } else if (param.second == "float") {
            param_types.push_back(SemanticType::createPrimitive("float"));
        } else if (param.second == "bool") {
            param_types.push_back(SemanticType::createPrimitive("bool"));
        } else if (param.second == "string") {
            param_types.push_back(SemanticType::createPrimitive("string"));
        } else {
            // Default to error type for unknown types
            param_types.push_back(SemanticType::createError());
        }
    }
    
    // Convert return type
    std::unique_ptr<SemanticType> ret_type;
    if (return_type == "void") {
        ret_type = SemanticType::createVoid();
    } else if (return_type == "int") {
        ret_type = SemanticType::createPrimitive("int");
    } else if (return_type == "float") {
        ret_type = SemanticType::createPrimitive("float");
    } else if (return_type == "bool") {
        ret_type = SemanticType::createPrimitive("bool");
    } else if (return_type == "string") {
        ret_type = SemanticType::createPrimitive("string");
    } else {
        ret_type = SemanticType::createError();
    }
    
    // Create function type
    auto function_type = SemanticType::createFunction(std::move(param_types), std::move(ret_type));
    
    // Create symbol for the built-in function
    auto function_symbol = std::make_unique<Symbol>(
        name, 
        std::move(function_type), 
        false, 
        SourceLocation{} // Built-ins don't have source locations
    );
    function_symbol->is_initialized = true;
    
    // Register in global scope
    global_scope->define(name, std::move(function_symbol));
}

bool TypeChecker::isForeignVariadicFunction(const std::string& name) const {
    // List of known foreign variadic functions from C standard library
    static const std::unordered_set<std::string> variadic_functions{
        "printf", "fprintf", "sprintf", "snprintf",
        "scanf", "fscanf", "sscanf"
    };
    
    return variadic_functions.find(name) != variadic_functions.end();
}

bool TypeChecker::isVariadicCompatible(const SemanticType& type) const {
    // Types that are compatible with variadic functions (C varargs)
    // Basic primitive types that can be passed to printf-style functions
    if (type.kind == SemanticType::Kind::PRIMITIVE) {
        return type.name == "i8" || type.name == "i16" || type.name == "i32" || type.name == "i64" ||
               type.name == "u8" || type.name == "u16" || type.name == "u32" || type.name == "u64" ||
               type.name == "f32" || type.name == "f64" || type.name == "bool" || type.name == "string";
    }
    
    // Pointers are also compatible with variadic functions
    if (type.kind == SemanticType::Kind::POINTER) {
        return true;
    }
    
    return false;
}

bool TypeChecker::isTypeCompatibleWithParameter(const SemanticType& arg_type, const SemanticType& param_type) const {
    // First check standard compatibility
    if (arg_type.isCompatibleWith(param_type)) {
        return true;
    }
    
    // Special case: string literals can be passed to cptr u8 parameters
    if (arg_type.kind == SemanticType::Kind::PRIMITIVE && arg_type.name == "string" &&
        param_type.kind == SemanticType::Kind::POINTER && param_type.name == "cptr" &&
        param_type.element_type && param_type.element_type->kind == SemanticType::Kind::PRIMITIVE &&
        param_type.element_type->name == "u8") {
        return true;
    }
    
    // Special case: string literals can be passed to cptr void parameters (for generic pointers)
    if (arg_type.kind == SemanticType::Kind::PRIMITIVE && arg_type.name == "string" &&
        param_type.kind == SemanticType::Kind::POINTER && param_type.name == "cptr" &&
        param_type.element_type && param_type.element_type->kind == SemanticType::Kind::VOID_TYPE) {
        return true;
    }
    
    return false;
}

} // namespace pangea
