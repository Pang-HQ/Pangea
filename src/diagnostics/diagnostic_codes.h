#pragma once

/*
 Diagnostic code and severity enums, split out so widely-included
 headers (lexer.h, parser.h, ...) can refer to them without pulling
 in the full Diagnostics class and its STL container dependencies.

 diagnostics.h re-exports these via include.
*/

namespace pangea {

enum class ErrorLevel {
    INFO,
    WARNING,
    ERROR,
    FATAL
};

/*
 DiagnosticCode covers frontend and semantic stages only - anything
 a user can reasonably act on by editing their source. Backend
 failures (bad IR, LLVM screaming, link errors, OOM) are ICE or
 environment errors, not diagnostics. Do not add backend codes here.
*/
enum class DiagnosticCode {
    MISSING_FILE,
    MODULE_CIRCULAR_DEPENDENCY,
    PARSE_UNEXPECTED_TOKEN,
    PARSE_UNEXPECTED_EOF,
    PARSE_STATEMENT_TERMINATOR_EXPECTED,
    PARSE_EXTRA_SEMICOLON,
    PARSE_OPTIONAL_INVALID_CONSTRUCTOR_SYNTAX,
    PARSE_OPTIONAL_NULL_AMBIGUOUS,
    PARSE_CONCEPT_METHOD_BODY,
    LEXER_UNEXPECTED_CHARACTER,
    LEXER_UNTERMINATED_BLOCK_COMMENT,
    LEXER_UNTERMINATED_STRING,
    LEXER_STRING_ESCAPE_ERROR,
    LEXER_INVALID_NUMBER_FORMAT,
    LEXER_NUMBER_OVERFLOW,
    LEXER_INVALID_TYPE_SUFFIX,
    LEXER_UNBALANCED_BRACKET,
    LEXER_STRING_POOL_EXHAUSTED,
    LEXER_SOURCE_TOO_LARGE,
    LEXER_EMBEDDED_NUL,
    SEMANTIC_INVALID_CONDITION,
    SEMANTIC_TYPE_MISMATCH,
    SEMANTIC_INVALID_CONSTRUCTOR_ARGUMENT,
    SEMANTIC_INVALID_VARIABLE_INITIALISER,
    SEMANTIC_USELESS_EXPRESSION_STATEMENT,
    SEMANTIC_INVALID_RETURN
};

} // namespace pangea
