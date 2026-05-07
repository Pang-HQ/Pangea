#include "scanners.h"

#include "../core/classifiers.h"
#include "../core/cursor.h"
#include "../core/emitter.h"
#include "../numeric/numeric.h"
#include "../../token.h"
#include "../../../source/source_buffer.h"

#include <cassert>
#include <cstddef>
#include <variant>

namespace pangea::detail {

void scan_number(SourceCursor &cur, Emitter &emit, const SourceBuffer &buf) {
    assert(is_decimal_digit(cur.peek()));

    const SourceOffset start = cur.offset();
    assert(static_cast<std::size_t>(start) <= buf.size());

    const SourceLength length = static_cast<SourceLength>(
        buf.size() - static_cast<std::size_t>(start));

    const NumericOutcome outcome = parse_numeric(buf.slice(start, length));

    assert(outcome.consumed <= static_cast<std::size_t>(
        std::numeric_limits<SourceLength>::max()));

    for (std::size_t i = 0; i < outcome.consumed; ++i) {
        cur.advance();
    }

    const SourceRange range = {
        start,
        static_cast<SourceLength>(outcome.consumed),
    };

    if (!outcome.result) {
        const NumericParseError err = outcome.result.error();
        emit.emit_error(range, code_for(err), message_for(err));
        return;
    }

    const NumericValue &value = outcome.result.value();
    if (const double *floating = std::get_if<double>(&value)) {
        emit.emit(TokenType::LITERAL_FLOAT, range, *floating);
        return;
    }

    emit.emit(TokenType::LITERAL_INTEGER, range, std::get<uint64_t>(value));
}

} // namespace pangea::detail
