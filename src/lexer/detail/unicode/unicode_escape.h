#pragma once

/*
 String escape helpers for lexer string literals.

 Supports the usual single-char escapes, \e, \xHH, \uHHHH, and
 \UHHHHHHHH. Unicode escapes are re-encoded as UTF-8. On malformed
 input unescape_string returns the StringEscapeError; on success
 it returns std::nullopt. Callers translate errors to a diagnostic
 via message_for().
*/

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pangea {

enum class StringEscapeError : std::uint8_t {
    TRAILING_BACKSLASH,
    X_REQUIRES_HEX,
    U_REQUIRES_4_HEX,
    U_REQUIRES_8_HEX,
    U_SURROGATE,
    U_OVERFLOW,
    UNKNOWN_ESCAPE,
};

// Human-readable message for the given error.
std::string_view message_for(StringEscapeError err) noexcept;

/*
 Decode raw literal contents into out. out is cleared first, so
 existing capacity is reused across calls; pass a long-lived buffer
 to avoid an allocation per literal. Returns std::nullopt on success
 or the StringEscapeError on malformed input. raw must not include
 quotes.
*/
[[nodiscard]] std::optional<StringEscapeError>
unescape_string(std::string_view raw, std::string &out);

// Render decoded bytes back into a readable escaped form.
std::string escape_string(std::string_view decoded);

} // namespace pangea
