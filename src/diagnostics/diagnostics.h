#pragma once

#include "diagnostic_codes.h"
#include "../source/source_location.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace pangea {

/*
 Diagnostics is a stage-agnostic store of diagnostic messages with
 in-process deduplication. The lexer, parser, and downstream stages
 all funnel through one instance.

 No colour, no I/O, no source-context rendering. Rendering lives in
 Renderer; the driver wires them together.
*/

/*
 location is optional: stage-level errors (missing file, environment
 failures surfaced as diagnostics) have no source position to point
 at. For those cases, the message must embed the distinguishing
 detail (path, module name) or dedup will collapse every instance
 into one.
*/
struct DiagnosticMessage {
    ErrorLevel level;
    DiagnosticCode code;
    std::optional<SourceLocation> location;
    std::string message;
};

// 64-bit hashed dedup key across code+level+location+message.
using DiagnosticFingerprint = std::uint64_t;

class Diagnostics {
public:
    Diagnostics() = default;

    /*
     Pinned identity: the driver holds one Diagnostics per compile and
     every stage takes a reference to it. Allowing copies or moves
     would let callers accidentally split the instance and leave
     stale refs pointing at an emptied husk, so both are deleted.
    */
    Diagnostics(const Diagnostics &) = delete;
    Diagnostics &operator=(const Diagnostics &) = delete;
    Diagnostics(Diagnostics &&) = delete;
    Diagnostics &operator=(Diagnostics &&) = delete;

    /*
     Record a diagnostic. Returns false if an equivalent one (same code,
     level, location, message) has already been recorded.

     The no-location overload is for stage-level errors (e.g. missing
     file) where no source position applies. The message must embed
     any distinguishing detail so instances do not dedup together.
    */
    bool report(DiagnosticCode code,
                ErrorLevel level,
                const SourceLocation &location,
                std::string_view message);

    bool report(DiagnosticCode code,
                ErrorLevel level,
                std::string_view message);

    std::size_t error_count() const;
    std::size_t warning_count() const;

    /*
     Iteration surface. Diagnostics is iterable so callers never see
     the concrete backing container and iterators stay invalidation-
     tracked under _GLIBCXX_DEBUG. Any mutation (report, clear)
     invalidates existing iterators - do not cache them across one.
    */
    auto begin() const noexcept { return entries_.begin(); }
    auto end() const noexcept { return entries_.end(); }
    std::size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }

    void clear();

private:
    std::vector<DiagnosticMessage> entries_;
    std::unordered_set<DiagnosticFingerprint> seen_;
};

} // namespace pangea
