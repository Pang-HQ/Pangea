#include "compile.h"
#include "llvm_codegen.h"
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Transforms/Utils/Cloning.h>

#ifdef _WIN32
    #include <windows.h>
    #ifdef VOID
    #undef VOID
    #endif
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace pangea {

Compiler::Compiler(LLVMCodeGenerator* cg, bool verbose)
    : codegen(cg), verbose(verbose) {
}

// Static member for getting executable filename
std::string Compiler::getExecutableFilename(const std::string& filename) {
    std::string exe_filename = filename;
    std::string os = detectOperatingSystem();

    if (os == "Windows" && exe_filename.find(".exe") == std::string::npos) {
        exe_filename += ".exe";
    }

    return exe_filename;
}

// Non-member helper function to report compilation errors
void reportCompilerError(const std::string& message) {
    std::cerr << "Compiler error: " << message << std::endl;
}

bool Compiler::compileToExecutable(const std::string& filename) {
    logVerbose("Starting cross-platform executable compilation for: " + filename);

    std::string exe_filename = getExecutableFilename(filename);
    logVerbose("Target OS detected: " + detectOperatingSystem());
    logVerbose("Output executable: " + exe_filename);

    // Generate object file first
    std::string obj_filename = filename + ".o";
    logVerbose("Generating object file: " + obj_filename);

    if (!compileToObjectFile(obj_filename)) {
        logVerbose("Failed to generate object file");
        return false;
    }

    logVerbose("Object file generated successfully");

    // Link to executable
    bool success = linkObjectToExecutable(obj_filename, exe_filename);

    if (success) {
        logVerbose("Executable created successfully: " + exe_filename);

        // clean up object
        std::remove(obj_filename.c_str());
    } else {
        logVerbose("Failed to create executable");
    }

    return success;
}

bool Compiler::compileToObjectFile(const std::string& filename) {
    // Initialize LLVM targets if not already done
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // Get the target triple for the current system
    std::string target_triple = llvm::sys::getDefaultTargetTriple();
    codegen->getModule().setTargetTriple(target_triple);

    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        // Using simplified error reporting
        reportCompilerError("Failed to lookup target: " + error);
        return false;
    }

    // Create target machine
    llvm::TargetOptions opt;
    auto reloc_model = std::optional<llvm::Reloc::Model>();
    llvm::TargetMachine* target_machine = target->createTargetMachine(
        target_triple, "generic", "", opt, reloc_model);

    codegen->getModule().setDataLayout(target_machine->createDataLayout());

    // Open output file
    std::error_code error_code;
    llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);

    if (error_code) {
        // Using simplified error reporting
        reportCompilerError("Could not open file: " + error_code.message());
        delete target_machine;
        return false;
    }

    // Create pass manager and add target-specific passes
    llvm::legacy::PassManager pass;
    auto file_type = llvm::CodeGenFileType::ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        // Using simplified error reporting
        reportCompilerError("TargetMachine can't emit a file of this type");
        delete target_machine;
        return false;
    }

    // Run the passes
    pass.run(codegen->getModule());
    dest.flush();

    delete target_machine;
    return true;
}

bool Compiler::linkObjectToExecutable(const std::string& obj_filename, const std::string& exe_filename) {
    logVerbose("Starting cross-platform linking process");
    logVerbose("Object file: " + obj_filename);
    logVerbose("Target executable: " + exe_filename);

    // Get platform-specific linker commands
    std::vector<std::string> linker_commands = getLinkerCommands(obj_filename, exe_filename);

    if (linker_commands.empty()) {
        // Using simplified error reporting
        reportCompilerError("No linker commands available for current platform");
        return false;
    }

    // Try each linker command in order of preference
    for (const auto& command : linker_commands) {
        logVerbose("Trying linker command: " + command);

        // Check if the base command is available before trying
        std::string linker = command.substr(0, command.find(' '));
        if (!isCommandAvailable(linker)) {
            logVerbose("Linker not available: " + linker);
            continue;
        }

        logVerbose("Executing: " + command);
        int result = std::system(command.c_str());

        if (result == 0) {
            logVerbose("Linking successful with: " + linker);
            return true;
        } else {
            logVerbose("Linking failed with exit code: " + std::to_string(result));
        }
    }

    std::string os = detectOperatingSystem();
    std::ostringstream error_msg;
    error_msg << "Failed to create executable: No compatible linker found.\n";
    error_msg << "Detected OS: " << os << "\n";
    error_msg << "Please install one of the following linkers:\n";

    if (os == "Windows") {
        error_msg << "  - Clang (clang-cl or clang) - Recommended\n";
        error_msg << "  - Microsoft Visual Studio (link.exe)\n";
        error_msg << "  - GCC (MinGW/MSYS2)\n";
    } else if (os == "Linux") {
        error_msg << "  - Clang (clang) - Recommended\n";
        error_msg << "  - GCC (gcc)\n";
        error_msg << "  - Install via: sudo apt install clang (Ubuntu/Debian)\n";
        error_msg << "  - Install via: sudo yum install clang (RHEL/CentOS)\n";
    } else if (os == "macOS") {
        error_msg << "  - Clang (clang) - Usually pre-installed with Xcode\n";
        error_msg << "  - GCC (gcc) - Install via Homebrew: brew install gcc\n";
        error_msg << "  - Install Xcode Command Line Tools: xcode-select --install\n";
    } else {
        error_msg << "  - Clang (clang)\n";
        error_msg << "  - GCC (gcc)\n";
    }

    error_msg << "\nAlternatively, use --llvm flag to generate LLVM IR instead.";

    reportCompilerError(error_msg.str());
    return false;
}

std::string Compiler::detectOperatingSystem() {
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#elif defined(__unix__) || defined(__unix)
    return "Unix";
#else
    return "Unknown";
#endif
}

std::vector<std::string> Compiler::getLinkerCommands(const std::string& obj_filename, const std::string& exe_filename) {
    std::vector<std::string> commands;
    std::string os = detectOperatingSystem();

    // Escape filenames for shell safety
    std::string safe_obj = "\"" + obj_filename + "\"";
    std::string safe_exe = "\"" + exe_filename + "\"";

    // Output redirection for quiet operation
    std::string quiet_redirect;
    if (os == "Windows") {
        quiet_redirect = " >nul 2>nul";
    } else {
        quiet_redirect = " >/dev/null 2>&1";
    }

    if (os == "Windows") {
        // Windows linker options in order of preference
        // 1. Clang (most compatible with LLVM) - link with MSVCRT for printf, math functions
        commands.push_back("clang -o " + safe_exe + " " + safe_obj + " -lmsvcrt" + quiet_redirect);

        // 2. GCC (MinGW) - link with standard C library and math library
        commands.push_back("gcc -o " + safe_exe + " " + safe_obj + " -lm -lmsvcrt" + quiet_redirect);
        commands.push_back("x86_64-w64-mingw32-gcc -o " + safe_exe + " " + safe_obj + " -lm" + quiet_redirect);

        // 3. Clang-cl (MSVC-compatible interface)
        commands.push_back("clang-cl /Fe:" + safe_exe + " " + safe_obj + " msvcrt.lib legacy_stdio_definitions.lib" + quiet_redirect);

        // 4. Microsoft linker (if available)
        commands.push_back("link.exe /OUT:" + safe_exe + " " + safe_obj + " /SUBSYSTEM:CONSOLE msvcrt.lib legacy_stdio_definitions.lib" + quiet_redirect);

    } else if (os == "Linux") {
        // Linux linker options
        // 1. Clang (preferred for LLVM compatibility)
        commands.push_back("clang -o " + safe_exe + " " + safe_obj + " -lm -lpthread" + quiet_redirect);

        // 2. GCC
        commands.push_back("gcc -o " + safe_exe + " " + safe_obj + " -lm -lpthread" + quiet_redirect);

        // 3. Alternative clang names
        commands.push_back("clang-15 -o " + safe_exe + " " + safe_obj + " -lm -lpthread" + quiet_redirect);
        commands.push_back("clang-14 -o " + safe_exe + " " + safe_obj + " -lm -lpthread" + quiet_redirect);

    } else if (os == "macOS") {
        // macOS linker options
        // 1. Clang (standard on macOS)
        commands.push_back("clang -o " + safe_exe + " " + safe_obj + quiet_redirect);

        // 2. GCC (if installed via Homebrew)
        commands.push_back("gcc -o " + safe_exe + " " + safe_obj + quiet_redirect);
        commands.push_back("gcc-13 -o " + safe_exe + " " + safe_obj + quiet_redirect);
        commands.push_back("gcc-12 -o " + safe_exe + " " + safe_obj + quiet_redirect);

    } else {
        // Generic Unix-like system
        commands.push_back("clang -o " + safe_exe + " " + safe_obj + " -lm" + quiet_redirect);
        commands.push_back("gcc -o " + safe_exe + " " + safe_obj + " -lm" + quiet_redirect);
    }

    return commands;
}

bool Compiler::isCommandAvailable(const std::string& command) const {
    // Test if a command is available by trying to run it with --version or similar
    std::string test_command;

#ifdef _WIN32
    // On Windows, redirect stderr to null and check exit code
    test_command = command + " --version >nul 2>nul";
#else
    // On Unix-like systems, redirect both stdout and stderr to /dev/null
    test_command = "command -v " + command + " >/dev/null 2>&1";
#endif

    int result = std::system(test_command.c_str());
    return result == 0;
}

void Compiler::logVerbose(const std::string& message) const {
    // Only log verbose messages if verbose mode is enabled
    if (verbose) {
        std::cout << "[Pangea Linker] " << message << std::endl;
    }
}

} // namespace pangea
