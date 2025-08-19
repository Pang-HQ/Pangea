#pragma once

#include <string>
#include <vector>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

// Forward declarations to avoid circular dependencies
namespace pangea {
    class TypeChecker;
    class LLVMCodeGenerator;
}

namespace pangea {
namespace builtins {

// Built-in function signature structure
struct BuiltinFunction {
    std::string name;
    std::string return_type;
    std::vector<std::pair<std::string, std::string>> parameters; // (name, type) pairs
    std::string description;
};

// Built-in functions registry
class BuiltinsRegistry {
public:
    BuiltinsRegistry();
    
    // Get all built-in functions
    const std::vector<BuiltinFunction>& getFunctions() const;
    
    // Check if a function is a built-in
    bool isBuiltinFunction(const std::string& name) const;
    
    // Get built-in function by name
    const BuiltinFunction* getFunction(const std::string& name) const;
    
    // Register built-ins with type checker
    void registerWithTypeChecker(TypeChecker& type_checker);
    
    // Register built-ins with code generator
    void registerWithCodeGenerator(LLVMCodeGenerator& codegen);

private:
    std::vector<BuiltinFunction> functions;
    void initializeBuiltins();
    
    // Helper methods for creating LLVM functions
    void createPrintFunction(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    void createPrintlnFunction(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    void createMathFunctions(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    void createAbsFunction(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    void createClampFunction(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    void createStringFunctions(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    void createConversionFunctions(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
    void createIOFunctions(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder);
};

// Global registry instance
extern BuiltinsRegistry& getBuiltinsRegistry();

} // namespace builtins
} // namespace pangea
