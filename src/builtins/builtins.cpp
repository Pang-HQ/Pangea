#include "builtins.h"
#include "../semantic/type_checker.h"
#include "../codegen/llvm_codegen.h"
#include <iostream>
#include <unordered_map>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>

namespace pangea {
namespace builtins {

BuiltinsRegistry::BuiltinsRegistry() {
    initializeBuiltins();
}

void BuiltinsRegistry::initializeBuiltins() {
    // No hardcoded builtins - all functions should be resolved via
    // standard library or foreign function declarations
    // The compiler now supports dynamic symbol resolution
}

const std::vector<BuiltinFunction>& BuiltinsRegistry::getFunctions() const {
    return functions;
}

bool BuiltinsRegistry::isBuiltinFunction(const std::string& name) const {
    for (const auto& func : functions) {
        if (func.name == name) {
            return true;
        }
    }
    return false;
}

const BuiltinFunction* BuiltinsRegistry::getFunction(const std::string& name) const {
    for (const auto& func : functions) {
        if (func.name == name) {
            return &func;
        }
    }
    return nullptr;
}

void BuiltinsRegistry::registerWithTypeChecker(TypeChecker& type_checker) {
    // Register all built-in functions with the type checker
    for (const auto& func : functions) {
        type_checker.registerBuiltinFunction(func.name, func.return_type, func.parameters);
    }
}

void BuiltinsRegistry::registerWithCodeGenerator(LLVMCodeGenerator& codegen) {
    // No hardcoded function registration - all functions are resolved dynamically
    // through foreign function declarations in the standard library
    // The LLVM code generator will create external linkage declarations
    // for foreign functions as needed during compilation
}

void BuiltinsRegistry::createPrintFunction(llvm::LLVMContext& context, llvm::Module& module, llvm::IRBuilder<>& builder) {
    // Create variadic print function signature - takes no fixed parameters, variadic
    // print was implemented hardcoded but shouldn't be, instead here it should be created
    // here was the old implementation's code:
    //llvm::Function* print_func = module->getFunction("print");
    //if (!print_func) {
    //    reportCodegenError(node.location, "Unknown function: print (builtins disabled)");
    //    return;
    //}
    //
    //// Generate arguments
    //std::vector<llvm::Value*> args;
    //for (auto& arg : node.arguments) {
    //    arg->accept(*this);
    //    llvm::Value* arg_val = getExpressionValue(*arg);
    //    if (!arg_val) {
    //        reportCodegenError(arg->location, "Invalid argument");
    //        return;
    //    }
    //    args.push_back(arg_val);
    //}
    //
    //// Get printf function
    //llvm::Function* printf_func = module->getFunction("printf");
    //if (!printf_func) {
    //    reportCodegenError(node.location, "printf function not found");
    //    return;
    //}
    //
    //// Handle different argument types and create format string
    //std::string format_str = "";
    //std::vector<llvm::Value*> printf_args;
    //
    //for (size_t i = 0; i < args.size(); ++i) {
    //    llvm::Value* arg = args[i];
    //    llvm::Type* arg_type = arg->getType();
    //    
    //    if (i > 0) {
    //        format_str += " "; // Space separator like Python's print
    //    }
    //    
    //    if (arg_type->isIntegerTy(32)) {
    //        format_str += "%d";
    //        printf_args.push_back(arg);
    //    } else if (arg_type->isIntegerTy(64)) {
    //        format_str += "%lld";
    //        printf_args.push_back(arg);
    //    } else if (arg_type->isDoubleTy()) {
    //        format_str += "%g";
    //        printf_args.push_back(arg);
    //    } else if (arg_type->isFloatTy()) {
    //        // Convert float to double for printf
    //        llvm::Value* double_val = builder->CreateFPExt(arg, llvm::Type::getDoubleTy(*context), "f2d");
    //        format_str += "%g";
    //        printf_args.push_back(double_val);
    //    } else if (arg_type->isIntegerTy(1)) {
    //        // Boolean - convert to string
    //        llvm::Value* true_str = builder->CreateGlobalStringPtr("true");
    //        llvm::Value* false_str = builder->CreateGlobalStringPtr("false");
    //        llvm::Value* bool_str = builder->CreateSelect(arg, true_str, false_str, "boolstr");
    //        format_str += "%s";
    //        printf_args.push_back(bool_str);
    //    } else if (arg_type->isPointerTy()) {
    //        // Assume it's a string
    //        format_str += "%s";
    //        printf_args.push_back(arg);
    //    } else {
    //        // Unknown type - just print as pointer
    //        format_str += "%p";
    //        printf_args.push_back(arg);
    //    }
    //}
    //
    //// Add newline at the end (like Python's print)
    //format_str += "\n";
    //
    //// Create format string constant
    //llvm::Value* format_const = builder->CreateGlobalStringPtr(format_str);
    //
    //// Insert format string at the beginning of arguments
    //printf_args.insert(printf_args.begin(), format_const);
    //
    //// Call printf
    //llvm::Value* result = builder->CreateCall(printf_func, printf_args);
    //setExpressionValue(node, result);
    //return;
}


// Global registry instance
static BuiltinsRegistry global_registry;

BuiltinsRegistry& getBuiltinsRegistry() {
    return global_registry;
}

} // namespace builtins
} // namespace pangea
