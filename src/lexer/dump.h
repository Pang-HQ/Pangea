#pragma once

/*
 Debug-only token-stream dumper used by the driver's --tokens mode.
 Writes each token's name and payload to out on its own line. Not
 a stable on-disk format and not intended for any consumer past
 ad-hoc developer inspection.
*/

#include <cstdio>

namespace pangea {

struct LexerOutput;
class StringPool;

void dump_tokens(std::FILE *out,
                 const LexerOutput &lex,
                 const StringPool &strings);

} // namespace pangea
