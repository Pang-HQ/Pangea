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
    std::string token_lexeme;  // For multi-character token underlining

    DiagnosticMessage(ErrorLevel lvl, const SourceLocation& loc, const std::string& msg)
        : level(lvl), location(loc), message(msg) {}
    
    DiagnosticMessage(ErrorLevel lvl, const SourceLocation& loc, const std::string& msg, const std::string& lexeme)
        : level(lvl), location(loc), message(msg), token_lexeme(lexeme) {}
};

enum class ColorMode {
    NEVER,
    AUTO,
    ALWAYS
};

class ErrorReporter {
private:
    std::vector<DiagnosticMessage> diagnostics;
    bool has_errors = false;
    ColorMode color_mode = ColorMode::AUTO;
    
    bool shouldUseColors() const;
    std::string colorize(const std::string& text, const std::string& color) const;

public:
    ErrorReporter() = default;
    explicit ErrorReporter(const std::string& color_mode_str);
    
    void reportError(const SourceLocation& location, const std::string& message, const bool is_warning = false);
    void reportError(const SourceLocation& location, const std::string& message, const std::string& token_lexeme, const bool is_warning = false);
    void reportWarning(const SourceLocation& location, const std::string& message);
    void reportInfo(const SourceLocation& location, const std::string& message);
    
    bool hasErrors() const { return has_errors; }
    size_t getErrorCount() const;
    size_t getWarningCount() const;
    
    void printDiagnostics() const;
    void printDiagnosticWithContext(const DiagnosticMessage& diagnostic, const std::string& source_content) const;
    void clear();
    
    const std::vector<DiagnosticMessage>& getDiagnostics() const { return diagnostics; }
};

} // namespace pangea
