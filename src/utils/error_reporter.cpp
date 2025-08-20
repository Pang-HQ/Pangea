#include "error_reporter.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
    #undef ERROR
#else
    #include <unistd.h>
#endif

namespace pangea {

ErrorReporter::ErrorReporter(const std::string& color_mode_str) {
    if (color_mode_str == "always") {
        color_mode = ColorMode::ALWAYS;
    } else if (color_mode_str == "never") {
        color_mode = ColorMode::NEVER;
    } else {
        color_mode = ColorMode::AUTO;
    }
}

bool ErrorReporter::shouldUseColors() const {
    switch (color_mode) {
        case ColorMode::ALWAYS:
            return true;
        case ColorMode::NEVER:
            return false;
        case ColorMode::AUTO:
            // Check if stderr is a terminal
            return isatty(fileno(stderr));
        default:
            return false;
    }
}

std::string ErrorReporter::colorize(const std::string& text, const std::string& color) const {
    if (!shouldUseColors()) {
        return text;
    }
    
    // ANSI color codes
    std::string color_code;
    if (color == "red") {
        color_code = "\033[1;31m";
    } else if (color == "yellow") {
        color_code = "\033[1;33m";
    } else if (color == "cyan") {
        color_code = "\033[1;36m";
    } else if (color == "blue") {
        color_code = "\033[1;34m";
    } else if (color == "green") {
        color_code = "\033[1;32m";
    } else if (color == "bold") {
        color_code = "\033[1m";
    } else {
        return text;
    }
    
    return color_code + text + "\033[0m";
}

void ErrorReporter::reportError(const SourceLocation& location, const std::string& message, const bool is_warning) {
    diagnostics.emplace_back(is_warning ? ErrorLevel::WARNING : ErrorLevel::ERROR, location, message);

    if (!is_warning) has_errors = true;
}

void ErrorReporter::reportError(const SourceLocation& location, const std::string& message, const std::string& token_lexeme, const bool is_warning) {
    diagnostics.emplace_back(is_warning ? ErrorLevel::WARNING : ErrorLevel::ERROR, location, message, token_lexeme);

    if (!is_warning) has_errors = true;
}

void ErrorReporter::reportWarning(const SourceLocation& location, const std::string& message) {
    diagnostics.emplace_back(ErrorLevel::WARNING, location, message);
}

void ErrorReporter::reportInfo(const SourceLocation& location, const std::string& message) {
    diagnostics.emplace_back(ErrorLevel::INFO, location, message);
}

size_t ErrorReporter::getErrorCount() const {
    return std::count_if(diagnostics.begin(), diagnostics.end(),
        [](const DiagnosticMessage& msg) { return msg.level == ErrorLevel::ERROR || msg.level == ErrorLevel::FATAL; });
}

size_t ErrorReporter::getWarningCount() const {
    return std::count_if(diagnostics.begin(), diagnostics.end(),
        [](const DiagnosticMessage& msg) { return msg.level == ErrorLevel::WARNING; });
}

void ErrorReporter::printDiagnostics() const {
    for (const auto& diagnostic : diagnostics) {
        std::string level_str;
        std::string level_color;
        
        switch (diagnostic.level) {
            case ErrorLevel::INFO:    
                level_str = "info"; 
                level_color = "cyan";
                break;
            case ErrorLevel::WARNING: 
                level_str = "warning"; 
                level_color = "yellow";
                break;
            case ErrorLevel::ERROR:   
                level_str = "error"; 
                level_color = "red";
                break;
            case ErrorLevel::FATAL:   
                level_str = "fatal"; 
                level_color = "red";
                break;
        }
        
        // Modern error format: error[ErrorType]: message
        std::cerr << colorize(level_str, level_color) << ": " << diagnostic.message << std::endl;
        
        // Location info with arrow pointer
        if (!diagnostic.location.filename.empty()) {
            std::cerr << colorize("  --> ", "blue") << diagnostic.location.filename 
                      << ":" << diagnostic.location.line << ":" << diagnostic.location.column << std::endl;
            
            // Try to show source context if we can read the file
            std::ifstream file(diagnostic.location.filename);
            if (file.is_open()) {
                std::string line;
                size_t current_line = 1;
                
                // Read to the error line
                while (std::getline(file, line) && current_line <= diagnostic.location.line) {
                    if (current_line == diagnostic.location.line) {
                        // Show the line with error
                        std::cerr << colorize("   |", "blue") << std::endl;
                        std::cerr << colorize(std::to_string(current_line) + " |", "blue") << " " << line << std::endl;
                        
                        // Show pointer to error location
                        std::string pointer_line = "   " + colorize("|", "blue") + " ";
                        for (size_t i = 1; i < diagnostic.location.column; ++i) {
                            pointer_line += " ";
                        }
                        
                        // Use token lexeme length for multi-character underlining
                        if (!diagnostic.token_lexeme.empty() && diagnostic.token_lexeme.length() > 1) {
                            pointer_line += colorize("^", "red");
                            for (size_t i = 1; i < diagnostic.token_lexeme.length(); ++i) {
                                pointer_line += colorize("~", "red");
                            }
                        } else {
                            pointer_line += colorize("^", "red");
                        }
                        std::cerr << pointer_line << std::endl;
                        break;
                    }
                    current_line++;
                }
            }
        }
        
        std::cerr << std::endl; // Add spacing between diagnostics
    }
}

void ErrorReporter::printDiagnosticWithContext(const DiagnosticMessage& diagnostic, const std::string& source_content) const {
    std::string level_str;
    std::string level_color;
    
    switch (diagnostic.level) {
        case ErrorLevel::INFO:    
            level_str = "info"; 
            level_color = "cyan";
            break;
        case ErrorLevel::WARNING: 
            level_str = "warning"; 
            level_color = "yellow";
            break;
        case ErrorLevel::ERROR:   
            level_str = "error"; 
            level_color = "red";
            break;
        case ErrorLevel::FATAL:   
            level_str = "fatal"; 
            level_color = "red";
            break;
    }
    
    // Modern error format
    std::cerr << colorize(level_str, level_color) << ": " << diagnostic.message << std::endl;
    
    // Location info with arrow pointer
    if (!diagnostic.location.filename.empty()) {
        std::cerr << colorize("  --> ", "blue") << diagnostic.location.filename 
                  << ":" << diagnostic.location.line << ":" << diagnostic.location.column << std::endl;
        
        // Extract the specific line from source content
        std::istringstream source_stream(source_content);
        std::string line;
        size_t current_line = 1;
        
        while (std::getline(source_stream, line) && current_line <= diagnostic.location.line) {
            if (current_line == diagnostic.location.line) {
                // Show the line with error
                std::cerr << colorize("   |", "blue") << std::endl;
                std::cerr << colorize(std::to_string(current_line) + " |", "blue") << " " << line << std::endl;
                
                // Show pointer to error location
                std::string pointer_line = "   " + colorize("|", "blue") + " ";
                for (size_t i = 1; i < diagnostic.location.column; ++i) {
                    pointer_line += " ";
                }
                
                // Use token lexeme length for multi-character underlining
                if (!diagnostic.token_lexeme.empty() && diagnostic.token_lexeme.length() > 1) {
                    pointer_line += colorize("^", "red");
                    for (size_t i = 1; i < diagnostic.token_lexeme.length(); ++i) {
                        pointer_line += colorize("~", "red");
                    }
                } else {
                    pointer_line += colorize("^", "red");
                }
                std::cerr << pointer_line << std::endl;
                break;
            }
            current_line++;
        }
    }
    
    std::cerr << std::endl;
}

void ErrorReporter::clear() {
    diagnostics.clear();
    has_errors = false;
}

} // namespace pangea
