#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "ast/ast_visitor.h"
#include "semantic/type_checker.h"
#include "utils/error_reporter.h"
#include "codegen/llvm_codegen.h"
#include "builtins/builtins.h"

using namespace pangea;

// Module manager for handling separate compilation
class ModuleManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Module>> loaded_modules;
    std::unordered_set<std::string> loading_modules; // For circular dependency detection
    ErrorReporter* error_reporter;
    bool verbose;

public:
    ModuleManager(ErrorReporter* reporter, bool verbose_mode) 
        : error_reporter(reporter), verbose(verbose_mode) {}

    std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            if (verbose) {
                std::cerr << "Warning: Could not open file '" << filename << "'" << std::endl;
            }
            return "";
        }
        
        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        
        return content;
    }

    std::string resolveModulePath(const std::string& module_path) {
        // Try different extensions and paths
        std::vector<std::string> candidates = {
            module_path + ".pang",
            module_path,
            "stdlib/" + module_path + ".pang",
            "stdlib/" + module_path
        };

        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate)) {
                return candidate;
            }
        }

        return ""; // Not found
    }

    std::unique_ptr<Module> loadModule(const std::string& module_path) {
        // Check if already loaded
        if (loaded_modules.find(module_path) != loaded_modules.end()) {
            return nullptr; // Already loaded
        }

        // Check for circular dependencies
        if (loading_modules.find(module_path) != loading_modules.end()) {
            std::cerr << "Error: Circular dependency detected for module: " << module_path << std::endl;
            return nullptr;
        }

        // Resolve the actual file path
        std::string file_path = resolveModulePath(module_path);
        if (file_path.empty()) {
            std::cerr << "Error: Could not find module: " << module_path << std::endl;
            return nullptr;
        }

        if (verbose) {
            std::cout << "Loading module: " << module_path << " from " << file_path << std::endl;
        }

        // Mark as loading
        loading_modules.insert(module_path);

        // Read and parse the module
        std::string source = readFile(file_path);
        if (source.empty()) {
            loading_modules.erase(module_path);
            return nullptr;
        }

        // Lexical analysis
        Lexer lexer(source, file_path, error_reporter);
        auto tokens = lexer.tokenize();

        if (error_reporter->hasErrors()) {
            loading_modules.erase(module_path);
            return nullptr;
        }

        // Parse the module
        Parser parser(std::move(tokens), error_reporter);
        auto program = parser.parseProgram();

        if (error_reporter->hasErrors()) {
            loading_modules.erase(module_path);
            return nullptr;
        }

        // Extract the main module from the program
        auto module = std::move(program->main_module);
        module->module_name = module_path;
        module->file_path = file_path;

        // Load dependencies first
        for (auto& import : module->imports) {
            auto dependency = loadModule(import->module_path);
            if (dependency) {
                loaded_modules[import->module_path] = std::move(dependency);
            }
        }

        // Mark as loaded
        loading_modules.erase(module_path);
        
        if (verbose) {
            std::cout << "Successfully loaded module: " << module_path << std::endl;
        }

        return module;
    }

    std::unique_ptr<Program> createProgram(const std::string& main_file, bool auto_import_stdlib = true, bool auto_import_builtins = true) {
        auto program = std::make_unique<Program>(SourceLocation());

        // Load the main module
        std::string main_module_name = std::filesystem::path(main_file).stem().string();
        
        // Read and parse main file
        std::string source = readFile(main_file);
        if (source.empty()) {
            return nullptr;
        }

        // Lexical analysis
        Lexer lexer(source, main_file, error_reporter);
        auto tokens = lexer.tokenize();

        if (error_reporter->hasErrors()) {
            return nullptr;
        }

        // Parse the main file
        Parser parser(std::move(tokens), error_reporter);
        auto main_program = parser.parseProgram();

        if (error_reporter->hasErrors()) {
            return nullptr;
        }

        // Set up the main module
        program->main_module = std::move(main_program->main_module);
        program->main_module->module_name = main_module_name;
        program->main_module->file_path = main_file;

        // Auto-import standard library modules if enabled
        if (auto_import_stdlib) {
            std::vector<std::string> stdlib_modules = {"io"};
            
            for (const auto& stdlib_module : stdlib_modules) {
                if (verbose) {
                    std::cout << "Auto-importing standard library module: " << stdlib_module << std::endl;
                }
                
                auto module = loadModule(stdlib_module);
                if (module) {
                    loaded_modules[stdlib_module] = std::move(module);
                    
                    // Create an implicit import declaration for the auto-imported module
                    auto import_decl = std::make_unique<ImportDeclaration>(SourceLocation(), stdlib_module, std::vector<std::string>{}, true);
                    program->main_module->imports.push_back(std::move(import_decl));
                }
            }
        }

        // Load all explicitly imported modules
        for (auto& import : program->main_module->imports) {
            auto module = loadModule(import->module_path);
            if (module) {
                loaded_modules[import->module_path] = std::move(module);
            } else {
                std::cout << "Failed to load module: " << import->module_path << std::endl;
                exit(1);
            }
            if (verbose)
            {
                std::cout << "[VERBOSE] Loaded module: " << import->module_path << std::endl;
            }
        }

        // Move all loaded modules to the program
        for (auto& [name, module] : loaded_modules) {
            program->modules.push_back(std::move(module));
        }

        return program;
    }
};

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return "";
    }
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    
    return content;
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] <input_file>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -o <file>     Specify output file (default: a.exe)" << std::endl;
    std::cout << "  -v, --verbose Enable verbose output (show all compilation steps)" << std::endl;
    std::cout << "  --color=MODE  Control colored output (always|auto|never, default: auto)" << std::endl;
    std::cout << "  --llvm        Output LLVM IR instead of executable" << std::endl;
    std::cout << "  --tokens      Print tokens and exit" << std::endl;
    std::cout << "  --ast         Print AST and exit" << std::endl;
    std::cout << "  --no-stdlib   Don't auto-import standard library" << std::endl;
    std::cout << "  --no-builtins Don't auto-import builtins" << std::endl;
    std::cout << "  --help        Show this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    bool print_tokens = false;
    bool print_ast = false;
    bool output_llvm = false;
    bool verbose = false;
    bool no_stdlib = false;
    bool no_builtins = false;
    std::string color_mode = "auto";
    std::string input_file;
    std::string output_file("a.exe");
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-o") {
            i++;
            if (i >= argc) {
                std::cerr << "No output file declared." << std::endl;
                return 1;
            }
            output_file = argv[i];
        } else if (arg == "--llvm") {
            output_llvm = true;
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--tokens") {
            print_tokens = true;
        } else if (arg == "--ast") {
            print_ast = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;  
        } else if (arg.starts_with("--color=")) {
            color_mode = arg.substr(8);
            if (color_mode != "always" && color_mode != "auto" && color_mode != "never") {
                std::cerr << "Error: Invalid color mode '" << color_mode << "'. Use always, auto, or never." << std::endl;
                return 1;
            }
        } else if (arg == "--no-stdlib") {
            no_stdlib = true;
        } else if (arg == "--no-builtins") {
            no_builtins = true;
        } else if (arg.starts_with("--")) {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 1;
        } else {
            input_file = arg;
        }
    }
    
    if (input_file.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Initialize error reporter with color support
    ErrorReporter error_reporter(color_mode);
    
    // Create module manager for separate compilation
    ModuleManager module_manager(&error_reporter, verbose);
    
    if (print_tokens) {
        // Just tokenize the main file for debugging
        std::string source = readFile(input_file);
        if (source.empty()) {
            return 1;
        }
        
        Lexer lexer(source, input_file, &error_reporter);
        auto tokens = lexer.tokenize();
        
        if (error_reporter.hasErrors()) {
            error_reporter.printDiagnostics();
            return 1;
        }
        
        std::cout << "Tokens:" << std::endl;
        for (const auto& token : tokens) {
            std::cout << token.toString() << std::endl;
        }
        return 0;
    }
    
    // Create program with separate module compilation

    if (verbose)
    {
        std::cout << "[VERBOSE] Creating program: " << input_file << std::endl;
    }

    auto program = module_manager.createProgram(input_file, !no_stdlib, !no_builtins);
    
    if (!program || error_reporter.hasErrors()) {
        error_reporter.printDiagnostics();
        return 1;
    }
    
    if (print_ast) {
        std::cout << "Abstract Syntax Tree:" << std::endl;
        std::cout << "Main module: " << program->main_module->module_name << std::endl;
        std::cout << "Imported modules: " << program->modules.size() << std::endl;
        for (const auto& module : program->modules) {
            std::cout << "  - " << module->module_name << " (" << module->file_path << ")" << std::endl;
        }
        return 0;
    }
    
    if (verbose)
    {
        std::cout << "[VERBOSE] Running semantic analysis..." << std::endl;
    }

    // Semantic analysis
    TypeChecker type_checker(&error_reporter, !no_builtins);
    
    type_checker.analyze(*program);

    if (error_reporter.hasErrors()) {
        error_reporter.printDiagnostics();
        return 1;
    }

    if (verbose)
    {
        std::cout << "[VERBOSE] Generating LLVM IR..." << std::endl;
    }
    
    // LLVM code generation
    LLVMCodeGenerator codegen(&error_reporter, verbose, !no_builtins);
    
    codegen.generateCode(*program);
    if (!codegen.verify()) {
        return 1;
    }
    
    if (error_reporter.hasErrors()) {
        error_reporter.printDiagnostics();
        return 1;
    }

    if (verbose)
    {
        std::cout << "[VERBOSE] Code generation completed." << std::endl;
        std::cout << "[VERBOSE] Emitting code to file: " << output_file << std::endl;
    }

    // Choose output format based on flags
    if (output_llvm) {
        // Output LLVM IR
        codegen.emitToFile(output_file);
        std::cout << "LLVM IR generated successfully: " << output_file << std::endl;
    } else {
        // Compile to executable
        if (codegen.compileToExecutable(output_file)) {
            std::cout << "Compiled successfully: " << output_file << std::endl;
        } else {
            return 1; // Error messages already printed by codegen
        }
    }
    
    return 0;
}
