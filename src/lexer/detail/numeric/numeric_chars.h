#pragma once

#include <utility>

#include "../core/classifiers.h"
#include "numeric_segments.h"

namespace pangea::detail {

inline int digit_value_for_base(char c, NumericBase base) noexcept {
    switch (base) {
        case NumericBase::DECIMAL:
            if (is_decimal_digit(c)) {
                return c - '0';
            }
            return -1;

        case NumericBase::HEX:
            if (!is_hex_digit(c)) {
                return -1;
            }
            if (c <= '9') {
                return c - '0';
            }
            if (c <= 'F') {
                return 10 + (c - 'A');
            }
            return 10 + (c - 'a');

        case NumericBase::BINARY:
            if (c == '0' || c == '1') {
                return c - '0';
            }
            return -1;

        case NumericBase::OCTAL:
            if (c >= '0' && c <= '7') {
                return c - '0';
            }
            return -1;
    }

    std::unreachable();
}

inline bool is_digit_for_base(char c, NumericBase base) noexcept {
    return digit_value_for_base(c, base) >= 0;
}

} // namespace pangea::detail
