#pragma once

/*
 Numeric parsing records source spans into the string_view passed to
 parse_numeric(). These are not SourceOffset values; the lexer converts
 the final consumed length back to SourceRange.
*/

#include "numeric.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace pangea::detail {

struct Span {
    const std::size_t begin;
    const std::size_t end;

    std::size_t length() const noexcept {
        return end - begin;
    }
};

enum class NumericKind : std::uint8_t {
    INTEGER,
    FLOAT,
};

enum class NumericBase : std::uint8_t {
    BINARY = 2,
    OCTAL = 8,
    DECIMAL = 10,
    HEX = 16,
};

/*
 Explicit base prefix ("0x", "0b", "0o"). Only constructed for non-
 decimal literals; absence in NumericSegments::prefix means decimal.
*/
struct ExplicitPrefix {
    const NumericBase base;
    const Span marker;
};

/*
 A run of digits with optional underscore separators in some base.
 Only constructed by scan_digit_run, so span is always meaningful.
*/
struct DigitRun {
    const Span span;
    const std::size_t digits;
    const bool ended_after_digit;
    const bool saw_separator;
    const bool bad_separator;
    const bool bad_digit;

    bool malformed() const noexcept {
        return bad_separator || bad_digit;
    }
};

// '.' followed by fractional digits. digits.span.begin == dot + 1.
struct FractionPart {
    const std::size_t dot;
    const DigitRun digits;

    bool malformed() const noexcept {
        if (digits.malformed()) {
            return true;
        }
        if (digits.digits > 0 && !digits.ended_after_digit) {
            return true;
        }
        return false;
    }
};

// 'e' or 'E', optional '+'/'-', then digits.
struct ExponentPart {
    const std::size_t marker;
    const std::optional<std::size_t> sign;
    const DigitRun digits;

    bool malformed() const noexcept {
        if (digits.malformed()) {
            return true;
        }
        if (digits.digits == 0 || !digits.ended_after_digit) {
            return true;
        }
        return false;
    }
};

/*
 Decimal float tail. Both fields nullopt means the literal is an
 integer.
*/
struct FloatTail {
    const std::optional<FractionPart> fraction;
    const std::optional<ExponentPart> exponent;

    bool present() const noexcept {
        return fraction.has_value() || exponent.has_value();
    }

    bool malformed() const noexcept {
        if (fraction.has_value() && fraction->malformed()) {
            return true;
        }
        if (exponent.has_value() && exponent->malformed()) {
            return true;
        }
        return false;
    }
};

/*
 Trailing type suffix. Wrapped in std::optional at the parent so no
 field is readable when the suffix is absent.
*/
struct SuffixInfo {
    const Span span;
    const TokenType token;
    const bool valid;
};

struct NumericSegments {
    const std::string_view text;
    const std::optional<ExplicitPrefix> prefix;
    const DigitRun integer;
    const FloatTail float_tail;
    const std::size_t numeric_end;
    const std::optional<SuffixInfo> suffix;
    const std::size_t consumed;

    NumericBase base() const noexcept {
        return prefix.has_value() ? prefix->base : NumericBase::DECIMAL;
    }

    NumericKind kind() const noexcept {
        return float_tail.present() ? NumericKind::FLOAT
                                    : NumericKind::INTEGER;
    }

    bool has_malformed_core() const noexcept {
        return integer.malformed() || float_tail.malformed();
    }
};

#ifndef NDEBUG
void audit_numeric_segments(const NumericSegments &segs);
void audit_numeric_outcome(const NumericSegments &segs,
                           const NumericOutcome &outcome);
#endif

} // namespace pangea::detail
