#include "unicode_escape.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace pangea {

static constexpr std::int8_t hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<std::int8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<std::int8_t>(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return static_cast<std::int8_t>(10 + (c - 'A'));
    return -1;
}

static constexpr bool is_hex(char c) noexcept {
    return hex_value(c) >= 0;
}

static void encode_utf8(std::uint32_t cp, std::string &out) {
    assert(cp <= 0x10FFFF);
    assert(cp < 0xD800 || cp > 0xDFFF);

    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
        return;
    }
    if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return;
    }
    if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return;
    }
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
}

static bool read_fixed_hex(std::string_view raw, size_t &pos,
                           size_t digits, std::uint32_t &out) {
    if (raw.size() - pos < digits) return false;
    std::uint32_t cp = 0;
    for (size_t i = 0; i < digits; ++i) {
        const std::int8_t v = hex_value(raw[pos + i]);
        if (v < 0) return false;
        cp = (cp << 4) | static_cast<std::uint32_t>(v);
    }
    pos += digits;
    out = cp;
    return true;
}

std::string_view message_for(StringEscapeError err) noexcept {
    switch (err) {
        case StringEscapeError::TRAILING_BACKSLASH:
            return "trailing backslash in string literal";
        case StringEscapeError::X_REQUIRES_HEX:
            return "\\x requires at least one hex digit";
        case StringEscapeError::U_REQUIRES_4_HEX:
            return "\\u requires exactly 4 hex digits";
        case StringEscapeError::U_REQUIRES_8_HEX:
            return "\\U requires exactly 8 hex digits";
        case StringEscapeError::U_SURROGATE:
            return "surrogate code point is not a valid Unicode scalar";
        case StringEscapeError::U_OVERFLOW:
            return "\\U code point exceeds U+10FFFF";
        case StringEscapeError::UNKNOWN_ESCAPE:
            return "unknown escape sequence";
    }
    return "unknown escape error";
}

std::optional<StringEscapeError>
unescape_string(std::string_view raw, std::string &out) {
    /*
     clear() preserves capacity; the caller's buffer keeps growing
     across calls until it stops needing to reallocate.
    */
    out.clear();
    out.reserve(raw.size());

    std::size_t i = 0;
    while (i < raw.size()) {
        const char c = raw[i];

        if (c != '\\') {
            out.push_back(c);
            ++i;
            continue;
        }

        // A trailing backslash is always invalid.
        ++i;
        if (i >= raw.size()) {
            return StringEscapeError::TRAILING_BACKSLASH;
        }

        const char e = raw[i];
        ++i;
        switch (e) {
            case 'a': out.push_back('\a'); break;
            case 'b': out.push_back('\b'); break;
            case 'e': out.push_back('\x1b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'v': out.push_back('\v'); break;
            case '\\': out.push_back('\\'); break;
            case '\'': out.push_back('\''); break;
            case '\"': out.push_back('\"'); break;
            case '0': out.push_back('\0'); break;
            case '?': out.push_back('?'); break;

            case 'x': {
                // \x reads one or two hex digits and emits one byte.
                if (i >= raw.size() || !is_hex(raw[i])) {
                    return StringEscapeError::X_REQUIRES_HEX;
                }
                std::uint32_t value = static_cast<std::uint32_t>(hex_value(raw[i]));
                ++i;
                if (i < raw.size() && is_hex(raw[i])) {
                    value = (value << 4) | static_cast<std::uint32_t>(hex_value(raw[i]));
                    ++i;
                }
                out.push_back(static_cast<char>(value & 0xFF));
                break;
            }

            case 'u': {
                // \u reads one BMP scalar and re-encodes it as UTF-8.
                std::uint32_t cp = 0;
                if (!read_fixed_hex(raw, i, 4, cp)) {
                    return StringEscapeError::U_REQUIRES_4_HEX;
                }
                if (cp >= 0xD800 && cp <= 0xDFFF) {
                    return StringEscapeError::U_SURROGATE;
                }
                encode_utf8(cp, out);
                break;
            }

            case 'U': {
                // \U reads one full Unicode scalar and re-encodes it as UTF-8.
                std::uint32_t cp = 0;
                if (!read_fixed_hex(raw, i, 8, cp)) {
                    return StringEscapeError::U_REQUIRES_8_HEX;
                }
                if (cp > 0x10FFFF) {
                    return StringEscapeError::U_OVERFLOW;
                }
                if (cp >= 0xD800 && cp <= 0xDFFF) {
                    return StringEscapeError::U_SURROGATE;
                }
                encode_utf8(cp, out);
                break;
            }

            default:
                return StringEscapeError::UNKNOWN_ESCAPE;
        }
    }

    return std::nullopt;
}

std::string escape_string(std::string_view decoded) {
    std::string out;
    /*
     Worst case every byte could become a 4-character \xNN escape,
     or 2-character short escape.
    */
    out.reserve(decoded.size() * 4);

    static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";

    for (char b : decoded) {
        const auto ub = static_cast<unsigned char>(b);  // Prevents any sign issues.
        switch (ub) {
            case '\a': out += "\\a"; break;
            case '\b': out += "\\b"; break;
            case '\x1b': out += "\\e"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\v': out += "\\v"; break;
            case '\\': out += "\\\\"; break;
            case '\'': out += "\\'"; break;
            case '\"': out += "\\\""; break;
            default: {
                if (ub < 0x20 || ub == 0x7F) {
                    out += "\\x";
                    out.push_back(HEX_DIGITS[(ub >> 4) & 0xF]);
                    out.push_back(HEX_DIGITS[ub & 0xF]);
                    break;
                }
                /*
                 Preserve raw non-ASCII bytes as-is; only control
                 bytes are hex-escaped.
                */
                out.push_back(b);
                break;
            }
        }
    }

    return out;
}

} // namespace pangea
