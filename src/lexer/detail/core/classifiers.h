#pragma once

/*
 ASCII byte classifiers shared between scan_one and the per-token
 scan functions. Identifiers are ASCII per the language spec; see
 docs/user/modules/foreign.md.
*/

namespace pangea::detail {

inline bool is_decimal_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

inline bool is_hex_digit(char c) noexcept {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

inline bool is_ident_start(char c) noexcept {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

inline bool is_ident_continue(char c) noexcept {
    return is_ident_start(c) || is_decimal_digit(c);
}

} // namespace pangea::detail
