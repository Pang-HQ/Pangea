#include "dump.h"

#include "output.h"
#include "token.h"
#include "../source/string_pool.h"

#include <cstdint>
#include <cstdio>
#include <string_view>
#include <type_traits>
#include <variant>

namespace pangea {

namespace {

void write_view(std::FILE *out, std::string_view s) {
    std::fwrite(s.data(), 1, s.size(), out);
}

void write_payload(std::FILE *out,
                   const TokenPayload &payload,
                   const StringPool &strings) {
    std::visit([&](const auto &v) {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, SymbolID>) {
            std::fputc('[', out);
            write_view(out, strings.view(v));
            std::fputc(']', out);
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
            std::fprintf(out, "[%llu]", static_cast<unsigned long long>(v));
        } else if constexpr (std::is_same_v<T, double>) {
            std::fprintf(out, "[%g]", v);
        }
        // std::monostate prints nothing.
    }, payload);
}

} // namespace

void dump_tokens(std::FILE *out,
                 const LexerOutput &lex,
                 const StringPool &strings) {
    for (const Token &tok : lex.tokens) {
        write_view(out, name_of(tok.type));
        write_payload(out, tok.payload, strings);
        std::fputc('\n', out);
    }
}

} // namespace pangea
