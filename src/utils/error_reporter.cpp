#include "error_reporter.h"
#include <iostream>
#include <algorithm>

namespace pangea {

void ErrorReporter::reportError(const SourceLocation& location, const std::string& message, const bool is_warning) {
    diagnostics.emplace_back(is_warning ? ErrorLevel::WARNING : ErrorLevel::ERROR, location, message);

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
        switch (diagnostic.level) {
            case ErrorLevel::INFO:    level_str = "info"; break;
            case ErrorLevel::WARNING: level_str = "warning"; break;
            case ErrorLevel::ERROR:   level_str = "error"; break;
            case ErrorLevel::FATAL:   level_str = "fatal"; break;
        }
        
        std::cerr << diagnostic.location.toString() << ": " 
                  << level_str << ": " << diagnostic.message << std::endl;
        
        if (!diagnostic.code_snippet.empty()) {
            std::cerr << diagnostic.code_snippet << std::endl;
        }
    }
}

void ErrorReporter::clear() {
    diagnostics.clear();
    has_errors = false;
}

} // namespace pangea
