#include "numeric.h"

#include "numeric_chars.h"
#include "numeric_cursor.h"
#include "numeric_segments.h"

#include <cassert>
#include <cmath>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <system_error>

#include <fast_float.h>
#include <utility>

namespace pangea::detail {

namespace {

bool is_valid_int_suffix_token(TokenType type) noexcept {
    switch (type) {
#define DEFINE_TOKEN(name, spelling, category)
#define DEFINE_INT_TYPE(name, spelling) case TokenType::name:
#include "../../token_kinds.def"
#undef DEFINE_INT_TYPE
#undef DEFINE_TOKEN
            return true;
        default:
            return false;
    }
}

bool is_valid_float_suffix_token(TokenType type) noexcept {
    switch (type) {
#define DEFINE_TOKEN(name, spelling, category)
#define DEFINE_FLOAT_TYPE(name, spelling) case TokenType::name:
#include "../../token_kinds.def"
#undef DEFINE_FLOAT_TYPE
#undef DEFINE_TOKEN
            return true;
        default:
            return false;
    }
}

void consume_identifier_tail(NumericCursor &cur) noexcept {
    while (is_ident_continue(cur.peek())) {
        cur.advance();
    }
}

std::optional<ExplicitPrefix> scan_prefix(NumericCursor &cur) noexcept {
    if (cur.peek() != '0')
        return std::nullopt;

    NumericBase base;
    switch (cur.peek(1)) {
        case 'x':
        case 'X':
            base = NumericBase::HEX;
            break;

        case 'b':
        case 'B':
            base = NumericBase::BINARY;
            break;

        case 'o':
        case 'O':
            base = NumericBase::OCTAL;
            break;

        default:
            return std::nullopt;
    }

    const Span span{cur.pos, cur.pos + 2};

    cur.advance();
    cur.advance();

    return ExplicitPrefix{base, span};
}

void consume_malformed_prefixed_core(NumericCursor &cur) noexcept {
    cur.advance();
    consume_identifier_tail(cur);
}

DigitRun scan_digit_run(NumericCursor &cur, NumericBase base) noexcept {
    const std::size_t begin = cur.pos;
    std::size_t digits = 0;
    bool prev_digit = false;
    bool saw_separator = false;
    bool bad_separator = false;

    while (true) {
        const char c = cur.peek();
        if (is_digit_for_base(c, base)) {
            cur.advance();
            ++digits;
            prev_digit = true;
            continue;
        }

        if (c != '_')
            break;

        saw_separator = true;
        const bool next_is_digit = is_digit_for_base(cur.peek(1), base);
        if (!prev_digit || !next_is_digit) {
            bad_separator = true;
        }

        cur.advance();
        prev_digit = false;
    }

    return DigitRun{
        .span = {begin, cur.pos},
        .digits = digits,
        .ended_after_digit = prev_digit,
        .saw_separator = saw_separator,
        .bad_separator = bad_separator,
        .bad_digit = false,
    };
}

DigitRun empty_run_at(std::size_t pos,
                      bool bad_separator,
                      bool bad_digit) noexcept {
    return DigitRun{
        .span = {pos, pos},
        .digits = 0,
        .ended_after_digit = false,
        .saw_separator = bad_separator,
        .bad_separator = bad_separator,
        .bad_digit = bad_digit,
    };
}

DigitRun scan_integer_core(
    NumericCursor &cur, const std::optional<ExplicitPrefix> &prefix
) noexcept {
    const NumericBase base =
        prefix.has_value() ? prefix->base : NumericBase::DECIMAL;

    if (!prefix.has_value()) {
        return scan_digit_run(cur, base);
    }

    const std::size_t begin = cur.pos;
    const char first = cur.peek();

    if (first == '\0') {
        return empty_run_at(begin, false, false);
    }

    if (first == '_') {
        consume_malformed_prefixed_core(cur);
        return DigitRun{
            .span = {begin, cur.pos},
            .digits = 0,
            .ended_after_digit = false,
            .saw_separator = true,
            .bad_separator = true,
            .bad_digit = false,
        };
    }

    if (!is_digit_for_base(first, base)) {
        consume_malformed_prefixed_core(cur);
        return DigitRun{
            .span = {begin, cur.pos},
            .digits = 0,
            .ended_after_digit = false,
            .saw_separator = false,
            .bad_separator = false,
            .bad_digit = true,
        };
    }

    const DigitRun run = scan_digit_run(cur, base);
    const char after = cur.peek();
    if (!is_decimal_digit(after) || is_digit_for_base(after, base)) {
        return run;
    }

    cur.advance();
    consume_identifier_tail(cur);
    return DigitRun{
        .span = {run.span.begin, cur.pos},
        .digits = run.digits,
        .ended_after_digit = false,
        .saw_separator = run.saw_separator,
        .bad_separator = run.bad_separator,
        .bad_digit = true,
    };
}

std::optional<FractionPart> scan_fraction(NumericCursor &cur) noexcept {
    if (cur.peek() != '.') {
        return std::nullopt;
    }

    const std::size_t dot = cur.pos;
    cur.advance();

    return FractionPart{
        .dot = dot,
        .digits = scan_digit_run(cur, NumericBase::DECIMAL),
    };
}

std::optional<ExponentPart> scan_exponent(NumericCursor &cur) noexcept {
    const char marker_char = cur.peek();
    if (marker_char != 'e' && marker_char != 'E') {
        return std::nullopt;
    }

    const std::size_t marker = cur.pos;
    cur.advance();

    std::optional<std::size_t> sign;
    if (cur.peek() == '+' || cur.peek() == '-') {
        sign = cur.pos;
        cur.advance();
    }

    return ExponentPart{
        .marker = marker,
        .sign = sign,
        .digits = scan_digit_run(cur, NumericBase::DECIMAL),
    };
}

FloatTail scan_decimal_float_tail(NumericCursor &cur) noexcept {
    return FloatTail{
        .fraction = scan_fraction(cur),
        .exponent = scan_exponent(cur),
    };
}

std::optional<SuffixInfo> scan_suffix(NumericCursor &cur,
                                      NumericKind kind) noexcept {
    if (!is_ident_start(cur.peek())) {
        return std::nullopt;
    }

    const std::size_t begin = cur.pos;
    while (is_ident_continue(cur.peek())) {
        cur.advance();
    }

    const Span span{begin, cur.pos};
    const TokenType token =
        lookup_keyword(cur.text.substr(span.begin, span.length()));
    const bool valid = kind == NumericKind::FLOAT
        ? is_valid_float_suffix_token(token)
        : is_valid_int_suffix_token(token);

    return SuffixInfo{
        .span = span,
        .token = token,
        .valid = valid,
    };
}

std::expected<uint64_t, NumericParseError>
parse_integer_value(const NumericSegments &segs) noexcept {
    std::uint64_t value = 0;
    const auto base = std::to_underlying(segs.base());

    for (std::size_t i = segs.integer.span.begin;
         i < segs.integer.span.end; ++i) {
        const char c = segs.text[i];
        if (c == '_') {
            continue;
        }

        const int digit = digit_value_for_base(c, segs.base());
        assert(digit >= 0);
        const std::uint64_t digit_value = static_cast<std::uint64_t>(digit);

        if (value > std::numeric_limits<std::uint64_t>::max() / base) {
            return std::unexpected(NumericParseError::INTEGER_OVERFLOW);
        }
        value *= base;

        if (value > std::numeric_limits<std::uint64_t>::max() - digit_value) {
            return std::unexpected(NumericParseError::INTEGER_OVERFLOW);
        }
        value += digit_value;
    }

    return value;
}

bool numeric_core_has_separator(const NumericSegments &segs) noexcept {
    if (segs.integer.saw_separator) {
        return true;
    }
    if (segs.float_tail.fraction.has_value() &&
        segs.float_tail.fraction->digits.saw_separator) {
        return true;
    }
    if (segs.float_tail.exponent.has_value() &&
        segs.float_tail.exponent->digits.saw_separator) {
        return true;
    }
    return false;
}

std::expected<double, NumericParseError>
parse_float_value(const NumericSegments &segs) {
    std::string cleaned;
    const char *first = segs.text.data();
    const char *last = first + segs.numeric_end;

    if (numeric_core_has_separator(segs)) {
        cleaned.reserve(segs.numeric_end);
        for (std::size_t i = 0; i < segs.numeric_end; ++i) {
            const char ch = segs.text[i];
            if (ch != '_') {
                cleaned.push_back(ch);
            }
        }

        first = cleaned.data();
        last = first + cleaned.size();
    }

    double value = 0.0;
    const auto result = fast_float::from_chars(first, last, value);
    if (result.ec == std::errc::invalid_argument || result.ptr != last) {
        return std::unexpected(NumericParseError::MALFORMED);
    }

    if (result.ec == std::errc::result_out_of_range && !std::isfinite(value)) {
        return std::unexpected(NumericParseError::FLOAT_OVERFLOW);
    }

    return value;
}

NumericOutcome finish(const NumericSegments &segs, NumericOutcome outcome) {
#ifndef NDEBUG
    audit_numeric_segments(segs);
    audit_numeric_outcome(segs, outcome);
#else
    (void)segs;
#endif
    return outcome;
}

NumericOutcome finish_error(const NumericSegments &segs,
                            NumericParseError err) {
    return finish(segs, {segs.consumed, std::unexpected(err)});
}

NumericOutcome finish_integer(const NumericSegments &segs) {
    const auto value = parse_integer_value(segs);
    if (!value) {
        return finish_error(segs, value.error());
    }

    return finish(segs, {segs.consumed, NumericValue{value.value()}});
}

NumericOutcome finish_float(const NumericSegments &segs) {
    const auto value = parse_float_value(segs);
    if (!value) {
        return finish_error(segs, value.error());
    }

    return finish(segs, {segs.consumed, NumericValue{value.value()}});
}

} // namespace

NumericOutcome parse_numeric(std::string_view text) {
    assert(!text.empty() && is_decimal_digit(text.front()));

    NumericCursor cur{text};

    const auto prefix = scan_prefix(cur);
    const auto integer = scan_integer_core(cur, prefix);

    if (integer.digits == 0) {
        const NumericSegments segs{
            .text = text,
            .prefix = prefix,
            .integer = integer,
            .float_tail = FloatTail{},
            .numeric_end = cur.pos,
            .suffix = std::nullopt,
            .consumed = cur.pos,
        };
        return finish_error(segs, segs.has_malformed_core()
            ? NumericParseError::MALFORMED
            : NumericParseError::NO_DIGITS);
    }

    const auto base = prefix.has_value() ? prefix->base : NumericBase::DECIMAL;
    const auto float_tail = base == NumericBase::DECIMAL
        ? scan_decimal_float_tail(cur)
        : FloatTail{};

    const NumericSegments segs{
        .text = text,
        .prefix = prefix,
        .integer = integer,
        .float_tail = float_tail,
        .numeric_end = cur.pos,
        .suffix = scan_suffix(cur, float_tail.present()
            ? NumericKind::FLOAT
            : NumericKind::INTEGER),
        .consumed = cur.pos,
    };

    if (segs.has_malformed_core())
        return finish_error(segs, NumericParseError::MALFORMED);

    if (segs.suffix.has_value() && !segs.suffix->valid)
        return finish_error(segs, NumericParseError::INVALID_SUFFIX);

    if (segs.kind() == NumericKind::FLOAT)
        return finish_float(segs);

    return finish_integer(segs);
}

std::string_view message_for(NumericParseError err) noexcept {
    switch (err) {
        case NumericParseError::NO_DIGITS:
            return "expected digits after numeric base prefix";
        case NumericParseError::MALFORMED:
            return "malformed numeric literal";
        case NumericParseError::INVALID_SUFFIX:
            return "invalid numeric type suffix";
        case NumericParseError::INTEGER_OVERFLOW:
            return "integer literal exceeds u64 range";
        case NumericParseError::FLOAT_OVERFLOW:
            return "float literal exceeds f64 range";
    }
    std::unreachable();
}

DiagnosticCode code_for(NumericParseError err) noexcept {
    switch (err) {
        case NumericParseError::NO_DIGITS:
        case NumericParseError::MALFORMED:
            return DiagnosticCode::LEXER_INVALID_NUMBER_FORMAT;
        case NumericParseError::INVALID_SUFFIX:
            return DiagnosticCode::LEXER_INVALID_TYPE_SUFFIX;
        case NumericParseError::INTEGER_OVERFLOW:
        case NumericParseError::FLOAT_OVERFLOW:
            return DiagnosticCode::LEXER_NUMBER_OVERFLOW;
    }
    std::unreachable();
}

} // namespace pangea::detail
