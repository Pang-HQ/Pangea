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

class LLVMCodeGenerator : public ASTVisitor {
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    ErrorReporter* error_reporter;
    
    // Symbol table for LLVM values
    std::unordered_map<std::string, llvm::Value*> named_values;
    
    // Current function being generated
    llvm::Function* current_function = nullptr;
    
    // Expression results
    std::unordered_map<Expression*, llvm::Value*> expression_values;

    bool verbose;
    
public:
    explicit LLVMCodeGenerator(ErrorReporter* reporter, bool verbose, bool enable_builtins = true);
    ~LLVMCodeGenerator() = default;
    
    void generateCode(Program& program);
    void emitToFile(const std::string& filename);
    void emitToString(std::string& output);
    bool verify();
    
    // Compilation methods
    bool compileToExecutable(const std::string& filename);
    bool compileToObjectFile(const std::string& filename);
    
    // Getters for LLVM components (needed by builtins)
    llvm::LLVMContext& getContext() { return *context; }
    llvm::Module& getModule() { return *module; }
    llvm::IRBuilder<>& getBuilder() { return *builder; }
    
private:
    // Cross-platform linking
    bool linkObjectToExecutable(const std::string& obj_filename, const std::string& exe_filename);
    std::string detectOperatingSystem();
    std::vector<std::string> getLinkerCommands(const std::string& obj_filename, const std::string& exe_filename);
    bool isCommandAvailable(const std::string& command);
    void logVerbose(const std::string& message);
    
    // Type visitors
    void visit(PrimitiveType& node) override;
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
    
    // Helper functions for code generation
    llvm::Function* createFunction(const std::string& name, llvm::FunctionType* func_type);
    llvm::BasicBlock* createBasicBlock(const std::string& name, llvm::Function* func = nullptr);
    bool isTypeIdentifier(const std::string& name);
    bool isRawVaListType(const Type& type);
    bool isStringLiteral(llvm::Value* value);
};

} // namespace pangea
