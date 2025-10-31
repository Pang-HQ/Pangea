#pragma once

#include "llvm_codegen.h"
#include <string>
#include <memory>

namespace pangea {

    class LLVMCodeGenerator;

    /**
     * Compiler class responsible for managing cross-platform compilation and linking.
     * Separated from LLVMCodeGenerator to maintain single responsibility principle.
     */
    class Compiler {
private:
        LLVMCodeGenerator* codegen;
        bool verbose;

        // Cross-platform linking
        bool linkObjectToExecutable(const std::string& obj_filename, const std::string& exe_filename);
        static std::string detectOperatingSystem();
        std::vector<std::string> getLinkerCommands(const std::string& obj_filename, const std::string& exe_filename);
        bool isCommandAvailable(const std::string& command) const;
        void logVerbose(const std::string& message) const;

public:
        /**
         * Construct compiler with LLVM code generator
         * @param cg Reference to initialized LLVMCodeGenerator
         * @param verbose Enable verbose compilation output
         */
        explicit Compiler(LLVMCodeGenerator* cg, bool verbose = false);
        ~Compiler() = default;

        /**
         * Compile LLVM IR to executable through object file
         * @param filename Output executable filename
         * @return true if successful
         */
        bool compileToExecutable(const std::string& filename);

        /**
         * Compile LLVM IR to object file only
         * @param filename Output object filename
         * @return true if successful
         */
        bool compileToObjectFile(const std::string& filename);

        /**
         * Get cross-platform executable filename
         * @param filename Base filename
         * @return Filename with appropriate extension
         */
        static std::string getExecutableFilename(const std::string& filename);
    };

} // namespace pangea
