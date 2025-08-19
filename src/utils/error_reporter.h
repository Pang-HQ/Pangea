#pragma once

#include "source_location.h"
#include <string>
#include <vector>

namespace pangea {

enum class ErrorLevel {
    INFO,
    WARNING,
    ERROR,
    FATAL
};

struct DiagnosticMessage {
    ErrorLevel level;
    SourceLocation location;
    std::string message;
    std::string code_snippet;

    DiagnosticMessage(ErrorLevel lvl, const SourceLocation& loc, const std::string& msg)
        : level(lvl), location(loc), message(msg) {}
};

class ErrorReporter {
private:
    std::vector<DiagnosticMessage> diagnostics;
    bool has_errors = false;

public:
    void reportError(const SourceLocation& location, const std::string& message, const bool is_warning = false);
    void reportWarning(const SourceLocation& location, const std::string& message);
    void reportInfo(const SourceLocation& location, const std::string& message);
    
    bool hasErrors() const { return has_errors; }
    size_t getErrorCount() const;
    size_t getWarningCount() const;
    
    void printDiagnostics() const;
    void clear();
    
    const std::vector<DiagnosticMessage>& getDiagnostics() const { return diagnostics; }
};

} // namespace pangea
