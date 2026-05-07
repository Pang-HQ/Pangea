#include "unicode_escape.h"

#include <cassert>
#include <cstdint>

namespace pangea {

static constexpr int8_t hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') return static_cast<int8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<int8_t>(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return static_cast<int8_t>(10 + (c - 'A'));
    return -1;
}

static constexpr bool is_hex(char c) noexcept {
    return hex_value(c) >= 0;
}

static void encode_utf8(uint32_t cp, std::string &out) {
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
                           size_t digits, uint32_t &out) {
    if (raw.size() - pos < digits) return false;
    uint32_t cp = 0;
    for (size_t i = 0; i < digits; ++i) {
        const int8_t v = hex_value(raw[pos + i]);
        if (v < 0) return false;
        cp = (cp << 4) | static_cast<uint32_t>(v);
    }
    pos += digits;
    out = cp;
    return true;
}

std::string_view message_for(StringEscapeError err) noexcept {
    switch (err) {
        case StringEscapeError::TrailingBackslash:
            return "trailing backslash in string literal";
        case StringEscapeError::XRequiresHex:
            return "\\x requires at least one hex digit";
        case StringEscapeError::URequires4Hex:
            return "\\u requires exactly 4 hex digits";
        case StringEscapeError::URequires8Hex:
            return "\\U requires exactly 8 hex digits";
        case StringEscapeError::USurrogate:
            return "surrogate code point is not a valid Unicode scalar";
        case StringEscapeError::UOverflow:
            return "\\U code point exceeds U+10FFFF";
        case StringEscapeError::UnknownEscape:
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

    size_t i = 0;
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
            return StringEscapeError::TrailingBackslash;
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
                    return StringEscapeError::XRequiresHex;
                }
                uint32_t value = static_cast<uint32_t>(hex_value(raw[i]));
                ++i;
                if (i < raw.size() && is_hex(raw[i])) {
                    value = (value << 4) | static_cast<uint32_t>(hex_value(raw[i]));
                    ++i;
                }
                out.push_back(static_cast<char>(value & 0xFF));
                break;
            }

            case 'u': {
                // \u reads one BMP scalar and re-encodes it as UTF-8.
                uint32_t cp = 0;
                if (!read_fixed_hex(raw, i, 4, cp)) {
                    return StringEscapeError::URequires4Hex;
                }
                if (cp >= 0xD800 && cp <= 0xDFFF) {
                    return StringEscapeError::USurrogate;
                }
                encode_utf8(cp, out);
                break;
            }

            case 'U': {
                // \U reads one full Unicode scalar and re-encodes it as UTF-8.
                uint32_t cp = 0;
                if (!read_fixed_hex(raw, i, 8, cp)) {
                    return StringEscapeError::URequires8Hex;
                }
                if (cp > 0x10FFFF) {
                    return StringEscapeError::UOverflow;
                }
                if (cp >= 0xD800 && cp <= 0xDFFF) {
                    return StringEscapeError::USurrogate;
                }
                encode_utf8(cp, out);
                break;
            }

            default:
                return StringEscapeError::UnknownEscape;
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
