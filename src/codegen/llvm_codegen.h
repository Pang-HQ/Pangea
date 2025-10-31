#pragma once
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "../ast/ast_visitor.h"
#include "../ast/ast_nodes.h"
#include "../utils/error_reporter.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <unordered_map>
#include <memory>
#include <string>

namespace pangea {

// ============================================================================
// VARIABLE MANAGEMENT SYSTEM - REWRITTEN FOR BETTER MODULARITY
// ============================================================================

class LLVMCodeGenerator : public ASTVisitor {
private:
    // LLVM Infrastructure (keeping existing)
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    ErrorReporter* error_reporter;
    bool verbose;

    // ===== NEW VARIABLE MANAGEMENT SYSTEM =====
    struct VariableInfo {
        llvm::Value* value;           // AllocaInst, GlobalVariable, or Constant
        bool is_const;               // Whether this variable is const
        bool is_exported;            // Whether this variable is exported
        bool is_initialised;         // Whether the variable has been initialized
        bool is_global;              // Whether this is a global variable
        SourceLocation location;     // Source location for error reporting

        VariableInfo(llvm::Value* val = nullptr,
                    bool constant = false,
                    const SourceLocation& loc = SourceLocation(),
                    bool exported = false,
                    bool global = false)
            : value(val), is_const(constant), is_exported(exported),
              is_initialised(true), is_global(global), location(loc) {}

        bool canBeGlobalInitializer() const {
            // Check if this variable can be used as a global initializer
            return value && (llvm::isa<llvm::Constant>(value) ||
                          (llvm::isa<llvm::GlobalVariable>(value) &&
                           llvm::cast<llvm::GlobalVariable>(value)->isConstant()));
        }
    };

    // Comprehensive symbol table with metadata
    std::unordered_map<std::string, VariableInfo> symbol_table;

    // Hierarchical scopes for local variables
    std::vector<std::unordered_map<std::string, VariableInfo*>> local_scopes;

    // Current function context
    llvm::Function* current_function = nullptr;

    // Expression value cache (renamed for clarity)
    std::unordered_map<Expression*, llvm::Value*> expression_cache;
    
public:
    explicit LLVMCodeGenerator(ErrorReporter* reporter, bool verbose, bool enable_builtins = true);
    ~LLVMCodeGenerator() = default;
    
    void generateCode(Program& program);
    void emitToFile(const std::string& filename);
    void emitToString(std::string& output);
    bool verify();

    // Getters for LLVM components (needed by builtins)
    llvm::LLVMContext& getContext() { return *context; }
    llvm::Module& getModule() { return *module; }
    llvm::IRBuilder<>& getBuilder() { return *builder; }
    
private:
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
    llvm::Type* convertType(Type& ast_type);
    llvm::Type* getPrimitiveType(TokenType token_type);

    llvm::Value* getExpressionValue(Expression& expr);
    void setExpressionValue(Expression& expr, llvm::Value* value);

    void reportCodegenError(const SourceLocation& location, const std::string& message);

    // ===== NEW VARIABLE MANAGEMENT METHODS =====
    VariableInfo* declareVariable(const std::string& name, VariableInfo&& info);
    VariableInfo* lookupVariable(const std::string& name);
    const VariableInfo* lookupVariable(const std::string& name) const;
    bool hasVariable(const std::string& name) const;

    // Scope management
    void enterScope();
    void exitScope();
    void enterFunctionScope(llvm::Function* function);
    void exitFunctionScope();

    // Global variable special handling
    bool canVariableBeGlobalInitializer(const VariableInfo& var) const;
    llvm::Value* resolveInitializerValue(llvm::Value* initializer_val, SourceLocation location);

    // Variable allocation
    llvm::AllocaInst* createLocalVariable(const std::string& name, llvm::Type* type, const SourceLocation& location);
    llvm::GlobalVariable* createGlobalVariable(const std::string& name, llvm::Type* type,
                                             llvm::Constant* initializer, bool is_const,
                                             bool is_exported, const SourceLocation& location);

    // Helper functions for code generation
    llvm::Function* createFunction(const std::string& name, llvm::FunctionType* func_type);
    llvm::BasicBlock* createBasicBlock(const std::string& name, llvm::Function* func = nullptr);
    bool isTypeIdentifier(const std::string& name);
    bool isRawVaListType(const Type& type);
    bool isStringLiteral(llvm::Value* value);

    // Binary operations helpers (DRY refactoring)
    llvm::Value* generateArithmeticOperation(TokenType op,
                                          llvm::Value* left_val,
                                          llvm::Value* right_val,
                                          const llvm::Type* common_type);
    llvm::Value* generateComparisonOperation(TokenType op,
                                          llvm::Value* left_val,
                                          llvm::Value* right_val);
    llvm::Value* generateBooleanOperation(TokenType op,
                                        llvm::Value* left_val,
                                        llvm::Value* right_val);
    llvm::Value* evaluateCondition(llvm::Value* condition_val);

    // Type conversion helpers
    bool isNumericType(llvm::Type* type);
    std::pair<llvm::Value*, llvm::Value*> promoteToCommonType(llvm::Value* left, llvm::Value* right);
    llvm::Type* getCommonNumericType(llvm::Type* left, llvm::Type* right);
    int getNumericTypeRank(llvm::Type* type);
};

} // namespace pangea
