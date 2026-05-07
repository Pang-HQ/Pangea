#pragma once

/*
 Free-function scanners called from Lexer::scan_one. Each function
 takes only the seams it needs; nothing here knows about the Lexer
 type itself.
*/

#include <cstdint>
#include <string>

namespace pangea {
class SourceBuffer;
class StringPool;
} // namespace pangea

namespace pangea::detail {

class BracketStack;
class Emitter;
class SourceCursor;

// Distinguishes plain "..." literals from c"..." C-string literals;
// see scan_string.cpp for the difference in pool storage.
enum class StringKind : uint8_t {
    PANGEA,
    C,
};

void scan_word(SourceCursor &cur, Emitter &emit, const SourceBuffer &buf);

void scan_number(SourceCursor &cur, Emitter &emit, const SourceBuffer &buf);

/*
 decode_buf is a caller-owned scratch buffer for unescape decoding;
 it is cleared on every call so existing capacity is reused.
*/
void scan_string(StringKind kind,
                 SourceCursor &cur,
                 Emitter &emit,
                 const SourceBuffer &buf,
                 StringPool &strings,
                 std::string &decode_buf);

void scan_symbol(SourceCursor &cur,
                 Emitter &emit,
                 BracketStack &brackets);

} // namespace pangea::detail
