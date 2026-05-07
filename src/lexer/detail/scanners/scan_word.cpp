#include "scanners.h"

#include "../core/classifiers.h"
#include "../core/cursor.h"
#include "../core/emitter.h"
#include "../../token.h"
#include "../../../source/source_buffer.h"

#include <cassert>
#include <string_view>

namespace pangea::detail {

void scan_word(SourceCursor &cur, Emitter &emit, const SourceBuffer &buf) {
    assert(is_ident_start(cur.peek()));

    const SourceOffset start = cur.offset();
    cur.advance();
    while (is_ident_continue(cur.peek())) {
        cur.advance();
    }
    const SourceOffset end = cur.offset();

    const std::string_view spelling = buf.slice(start, end - start);
    const TokenType type = lookup_keyword(spelling);
    emit.emit(type, {start, end - start});
}

} // namespace pangea::detail
