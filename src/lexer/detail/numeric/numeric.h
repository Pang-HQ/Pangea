#pragma once

/*
 Numeric literal parsing, split out of the lexer so scan_number only
 needs to capture the literal's byte span and delegate. The lexer stays
 focused on tokenisation; numeric-specific state (overflow, underscore
 placement, suffix validation, fast_float) lives here.

 The parser operates on a std::string_view so it never touches the
 lexer's cursor; the caller advances by `consumed` after the call.
*/

#include "../../../diagnostics/diagnostic_codes.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

namespace pangea::detail {

// Grammar and implementation policy: see lexer/README.md "Number model".
enum class NumericParseError {
    NO_DIGITS,
    MALFORMED,
    INVALID_SUFFIX,
    INTEGER_OVERFLOW,
    FLOAT_OVERFLOW,
};

using NumericValue = std::variant<uint64_t, double>;

struct NumericOutcome {
    /*
     Bytes consumed from the start of the input view. Always set,
     even on failure, so the caller can span one diagnostic over
     the full erroneous literal.
    */
    const std::size_t consumed;
    const std::expected<NumericValue, NumericParseError> result;
};

// Precondition: text is non-empty and its first byte is an ASCII digit.
[[nodiscard]]
NumericOutcome parse_numeric(std::string_view text);

[[nodiscard]]
std::string_view message_for(NumericParseError err) noexcept;

[[nodiscard]]
DiagnosticCode code_for(NumericParseError err) noexcept;

} // namespace pangea::detail
