#include "numeric_segments.h"
#include "numeric_chars.h"

#include <cassert>
#include <cmath>
#include <variant>

namespace pangea::detail {

#ifndef NDEBUG
namespace {

void audit_span(std::string_view text, const Span &span) {
    assert(span.begin <= span.end);
    assert(span.end <= text.size());
}

void audit_prefix(const NumericSegments &segs) {
    if (!segs.prefix.has_value()) {
        return;
    }

    const ExplicitPrefix &prefix = *segs.prefix;
    assert(prefix.marker.begin == 0);
    assert(prefix.marker.end == 2);
    assert(segs.text.size() >= 2);
    assert(segs.text[0] == '0');

    const char ch = segs.text[1];
    if (prefix.base == NumericBase::HEX) {
        assert(ch == 'x' || ch == 'X');
        return;
    }
    if (prefix.base == NumericBase::BINARY) {
        assert(ch == 'b' || ch == 'B');
        return;
    }
    assert(prefix.base == NumericBase::OCTAL);
    assert(ch == 'o' || ch == 'O');
}

void audit_digit_run_chars(const NumericSegments &segs,
                           const DigitRun &run,
                           NumericBase base) {
    bool prev_digit = false;
    std::size_t digits = 0;

    for (std::size_t i = run.span.begin; i < run.span.end; ++i) {
        const char c = segs.text[i];
        if (c == '_') {
            assert(run.saw_separator);
            if (!run.bad_separator) {
                assert(prev_digit);
                assert(i + 1 < run.span.end);
                assert(is_digit_for_base(segs.text[i + 1], base));
            }
            prev_digit = false;
            continue;
        }

        if (!run.bad_digit) {
            assert(is_digit_for_base(c, base));
        }
        if (is_digit_for_base(c, base)) {
            ++digits;
            prev_digit = true;
        }
    }

    assert(digits == run.digits || run.bad_digit || run.bad_separator);
    if (!run.malformed()) {
        assert(run.ended_after_digit == (run.digits > 0 && prev_digit));
    }
}

void audit_digit_run(const NumericSegments &segs,
                     const DigitRun &run,
                     NumericBase base) {
    audit_span(segs.text, run.span);
    audit_digit_run_chars(segs, run, base);
}

void audit_integer_attachment(const NumericSegments &segs) {
    const std::size_t expected_begin =
        segs.prefix.has_value() ? segs.prefix->marker.end : 0;
    assert(segs.integer.span.begin == expected_begin);
}

void audit_fraction(const NumericSegments &segs,
                    const FractionPart &fraction) {
    assert(fraction.dot < segs.numeric_end);
    assert(segs.text[fraction.dot] == '.');
    assert(fraction.digits.span.begin == fraction.dot + 1);
    audit_digit_run(segs, fraction.digits, NumericBase::DECIMAL);
}

void audit_exponent(const NumericSegments &segs,
                    const ExponentPart &exponent) {
    assert(exponent.marker < segs.numeric_end);
    assert(segs.text[exponent.marker] == 'e' ||
           segs.text[exponent.marker] == 'E');

    if (exponent.sign.has_value()) {
        assert(*exponent.sign == exponent.marker + 1);
        assert(segs.text[*exponent.sign] == '+' ||
               segs.text[*exponent.sign] == '-');
    }

    audit_digit_run(segs, exponent.digits, NumericBase::DECIMAL);
}

void audit_float_tail(const NumericSegments &segs) {
    const FloatTail &tail = segs.float_tail;

    if (!tail.present()) {
        assert(segs.kind() == NumericKind::INTEGER);
        return;
    }

    assert(segs.base() == NumericBase::DECIMAL);
    assert(segs.kind() == NumericKind::FLOAT);

    if (tail.fraction.has_value()) {
        audit_fraction(segs, *tail.fraction);
    }
    if (tail.exponent.has_value()) {
        audit_exponent(segs, *tail.exponent);
    }
}

void audit_suffix(const NumericSegments &segs) {
    if (!segs.suffix.has_value()) {
        return;
    }

    const SuffixInfo &suffix = *segs.suffix;
    audit_span(segs.text, suffix.span);

    assert(suffix.span.begin == segs.numeric_end);
    assert(suffix.span.end == segs.consumed);
    assert(suffix.span.begin < suffix.span.end);
    assert(is_ident_start(segs.text[suffix.span.begin]));

    for (std::size_t i = suffix.span.begin; i < suffix.span.end; ++i) {
        assert(is_ident_continue(segs.text[i]));
    }
}

void audit_success(const NumericSegments &segs, const NumericValue &value) {
    assert(!segs.has_malformed_core());
    assert(!segs.suffix.has_value() || segs.suffix->valid);

    if (segs.kind() == NumericKind::INTEGER) {
        assert(std::holds_alternative<std::uint64_t>(value));
        return;
    }

    assert(segs.kind() == NumericKind::FLOAT);
    assert(std::holds_alternative<double>(value));
    assert(std::isfinite(std::get<double>(value)));
}

void audit_error(const NumericSegments &segs, NumericParseError err) {
    switch (err) {
        case NumericParseError::NO_DIGITS:
            assert(segs.prefix.has_value());
            assert(segs.integer.digits == 0);
            assert(!segs.integer.bad_separator);
            assert(!segs.integer.bad_digit);
            return;

        case NumericParseError::MALFORMED:
            assert(segs.has_malformed_core());
            return;

        case NumericParseError::INVALID_SUFFIX:
            assert(!segs.has_malformed_core());
            assert(segs.suffix.has_value());
            assert(!segs.suffix->valid);
            return;

        case NumericParseError::INTEGER_OVERFLOW:
            assert(segs.kind() == NumericKind::INTEGER);
            assert(!segs.has_malformed_core());
            return;

        case NumericParseError::FLOAT_OVERFLOW:
            assert(segs.kind() == NumericKind::FLOAT);
            assert(!segs.has_malformed_core());
            return;
    }
}

} // namespace

void audit_numeric_segments(const NumericSegments &segs) {
    assert(!segs.text.empty());
    assert(is_decimal_digit(segs.text.front()));
    assert(segs.numeric_end <= segs.consumed);
    assert(segs.consumed <= segs.text.size());

    audit_prefix(segs);
    audit_integer_attachment(segs);
    audit_digit_run(segs, segs.integer, segs.base());
    audit_float_tail(segs);
    audit_suffix(segs);
}

void audit_numeric_outcome(const NumericSegments &segs,
                           const NumericOutcome &outcome) {
    assert(outcome.consumed == segs.consumed);

    if (outcome.result) {
        audit_success(segs, outcome.result.value());
        return;
    }

    audit_error(segs, outcome.result.error());
}
#endif

} // namespace pangea::detail
