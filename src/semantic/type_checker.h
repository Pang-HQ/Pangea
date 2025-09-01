#pragma once

#include "../ast/ast_visitor.h"
#include "../ast/ast_nodes.h"
#include "../utils/error_reporter.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

namespace pangea {

// Type representation for semantic analysis
class SemanticType {
public:
    enum class Kind {
        PRIMITIVE,
        ARRAY,
        POINTER,
        FUNCTION,
        VOID_TYPE,
        ERROR_TYPE
    };

    bool is_const;
    
    // Single constructor for kind and optional name
    explicit SemanticType(Kind kind, const std::string& name = "", bool is_const = false)
        : kind(kind), name(name), is_const(is_const) {}

    // Copy constructor
    SemanticType(const SemanticType& other)
        : kind(other.kind),
        name(other.name),
        element_type(other.element_type ? std::make_unique<SemanticType>(*other.element_type) : nullptr),
        return_type(other.return_type ? std::make_unique<SemanticType>(*other.return_type) : nullptr)
    {
        for (const auto& param : other.parameter_types) {
            parameter_types.push_back(std::make_unique<SemanticType>(*param));
        }
    }

    // Move constructor
    SemanticType(SemanticType&& other) noexcept
        : kind(other.kind),
        name(std::move(other.name)),
        element_type(std::move(other.element_type)),
        parameter_types(std::move(other.parameter_types)),
        return_type(std::move(other.return_type)) {}

    ~SemanticType() = default;
    
    Kind kind;
    std::string name;
    std::unique_ptr<SemanticType> element_type;
    std::vector<std::unique_ptr<SemanticType>> parameter_types;
    std::unique_ptr<SemanticType> return_type;
    
    bool isCompatibleWith(const SemanticType& other) const;
    bool isNumberType() const;
    bool isFloatingPointType() const;
    std::string toString() const;

    static std::unique_ptr<SemanticType> createPrimitive(const std::string& name, bool is_const = false);
    static std::unique_ptr<SemanticType> createArray(std::unique_ptr<SemanticType> element, bool is_const = false);
    static std::unique_ptr<SemanticType> createPointer(std::unique_ptr<SemanticType> pointee, TokenType kind, bool is_const = false);
    static std::unique_ptr<SemanticType> createFunction(
        std::vector<std::unique_ptr<SemanticType>> params,
        std::unique_ptr<SemanticType> ret_type
    );
    static std::unique_ptr<SemanticType> createVoid();
    static std::unique_ptr<SemanticType> createError();
};

// Symbol table entry
struct Symbol {
    std::string name;
    std::unique_ptr<SemanticType> type;
    bool is_mutable;
    bool is_initialized;
    // Module where this symbol was declared (empty for built-ins)
    std::string declared_module;
    // Whether this symbol is exported from its module
    bool is_exported = false;
    SourceLocation declaration_location;
    
    Symbol(const std::string& symbol_name, std::unique_ptr<SemanticType> symbol_type, 
           bool mutable_flag, const SourceLocation& loc)
        : name(symbol_name), type(std::move(symbol_type)), is_mutable(mutable_flag), 
          is_initialized(false), declaration_location(loc) {}
};

// Scope management
class Scope {
private:
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    Scope* parent;
    
public:
    explicit Scope(Scope* parent_scope = nullptr) : parent(parent_scope) {}
    
    void define(const std::string& name, std::unique_ptr<Symbol> symbol);
    Symbol* lookup(const std::string& name);
    bool isDefined(const std::string& name) const;
    
    Scope* getParent() const { return parent; }

    // Expose symbols for module export collection
    const std::unordered_map<std::string, std::unique_ptr<Symbol>>& getSymbols() const { return symbols; }
};

// Type checker and semantic analyzer
class TypeChecker : public ASTVisitor {
private:
    ErrorReporter* error_reporter;
    std::unique_ptr<Scope> global_scope;
    Scope* current_scope;
    
    // Scope stack for proper lifetime management
    std::vector<std::unique_ptr<Scope>> scope_stack;
    
    // Type information for expressions
    std::unordered_map<Expression*, std::unique_ptr<SemanticType>> expression_types;
    
    // Current function return type for return statement checking
    SemanticType* current_function_return_type = nullptr;

    // Current module being analyzed (used for visibility checks)
    std::string current_module_name;
    
public:
    explicit TypeChecker(ErrorReporter* reporter, bool enable_builtins = true);
    ~TypeChecker() = default;
    
    void analyze(Program& program);
    
    // Type visitors
    void visit(PrimitiveType& node) override;
    void visit(ConstType& node) override;
    void visit(ArrayType& node) override;
    void visit(PointerType& node) override;
    void visit(GenericType& node) override;
    
    // Expression visitors
    void visit(LiteralExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
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
    void visit(ImportDeclaration& node) override;
    
    // Module and Program visitors
    void visit(Module& node) override;
    void visit(Program& node) override;
    
private:
    void enterScope();
    void exitScope();
    
    std::unique_ptr<SemanticType> convertASTType(Type& ast_type);
    std::unique_ptr<SemanticType> getExpressionType(Expression& expr);
    void setExpressionType(Expression& expr, std::unique_ptr<SemanticType> type);
    
    bool checkTypeCompatibility(const SemanticType& expected, const SemanticType& actual);
    void reportTypeError(const SourceLocation& location, const std::string& message, const bool is_warning = false);
    
    // Built-in type creation
    void initializeBuiltinTypes();

    // Numeric and conversion helpers
    bool isIntegerType(const SemanticType& t) const;
    int numericRank(const SemanticType& t) const;
    std::string commonNumericTypeName(const SemanticType& a, const SemanticType& b) const;
    bool isImplicitlyConvertible(const SemanticType& from, const SemanticType& to) const;

    // Import/Export visibility helpers
    struct ImportInfo {
        std::string module_path;
        std::vector<std::string> items; // empty means wildcard
        bool is_wildcard = false;
    };
    // Map: current module -> its list of imports
    std::unordered_map<std::string, std::vector<ImportInfo>> module_imports;

    // Export table: module -> (symbol name -> exported symbol copy)
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Symbol>>> exports_by_module;

    bool isSymbolVisibleInCurrentModule(const Symbol& sym) const;

    // Module utilities
    void collectModuleExports(Module& node);
    void injectImportsIntoScope(const Module& node);

public:
    // Register built-in functions
    void registerBuiltinFunction(const std::string& name, const std::string& return_type,
                                const std::vector<std::pair<std::string, std::string>>& parameters);
    
    // Foreign function support
    bool isForeignVariadicFunction(const std::string& name) const;
    bool isVariadicCompatible(const SemanticType& type) const;
    bool isTypeCompatibleWithParameter(const SemanticType& arg_type, const SemanticType& param_type) const;
    
    // Null comparison support
    bool isNullComparison(const SemanticType& left_type, const SemanticType& right_type) const;
};

} // namespace pangea
