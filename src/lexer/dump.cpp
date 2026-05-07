#include "dump.h"

#include "output.h"
#include "token.h"
#include "../source/string_pool.h"

#include <cstdio>
#include <string_view>

namespace pangea {

namespace {

void write_view(std::FILE *out, std::string_view s) {
    std::fwrite(s.data(), 1, s.size(), out);
}

void write_payload(std::FILE *out,
                   const Token &tok,
                   const LexerOutput &lex,
                   const StringPool &strings) {
    switch (tok.type) {
        case TokenType::LITERAL_STRING:
        case TokenType::LITERAL_C_STRING:
            std::fputc('[', out);
            write_view(out, strings.view(symbol_of(tok)));
            std::fputc(']', out);
            return;

        case TokenType::LITERAL_INTEGER:
            std::fprintf(out, "[%llu]",
                static_cast<unsigned long long>(int_value_of(tok, lex)));
            return;

        case TokenType::LITERAL_FLOAT:
            std::fprintf(out, "[%g]", float_value_of(tok, lex));
            return;

        case TokenType::SPECIAL_NEWLINE:
            std::fprintf(out, "[%u]", newlines_of(tok));
            return;

        default:
            // No payload to print.
            return;
    }
}

} // namespace

void dump_tokens(std::FILE *out,
                 const LexerOutput &lex,
                 const StringPool &strings) {
    for (const Token &tok : lex.tokens) {
        write_view(out, name_of(tok.type));
        write_payload(out, tok, lex, strings);
        std::fputc('\n', out);
    }
}

} // namespace pangea
