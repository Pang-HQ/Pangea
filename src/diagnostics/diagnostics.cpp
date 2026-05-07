#include "diagnostics.h"

#include <cstdint>
#include <algorithm>
#include <functional>

namespace pangea {

namespace {

/*
 Hash-combine step: a murmur3-style finalizer on the new payload,
 then a boost::hash_combine-style fold into the rolling seed. We only
 need a decent distribution over DiagnosticFingerprints, not crypto
 strength - no adversarial inputs are in play here.
*/
inline std::uint64_t mix64(std::uint64_t seed, std::uint64_t v) noexcept {
    v ^= v >> 33;
    v *= 0xff51afd7ed558ccdULL;
    v ^= v >> 33;
    v *= 0xc4ceb9fe1a85ec53ULL;
    v ^= v >> 33;

    return seed ^ (v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

/*
 Fold each payload into a 64-bit hash. Not cryptographic - only a
 distribution good enough to keep the dedup set's collision rate low
 over the handful of diagnostics a compile typically produces.

 location is nullable: locationless diagnostics skip the position
 mix, which is fine because code+level+message still distinguish
 them as long as callers embed the distinguishing detail in the
 message (see report() contract).
*/
DiagnosticFingerprint fingerprint_of(DiagnosticCode code,
                                     ErrorLevel level,
                                     const SourceLocation *location,
                                     std::string_view message) {
    std::uint64_t h = static_cast<std::uint64_t>(static_cast<std::uint32_t>(code));
    h = mix64(h, static_cast<std::uint32_t>(level));
    if (location != nullptr) {
        h = mix64(h, std::hash<std::string_view>{}(location->filename));
        h = mix64(h, location->line);
        h = mix64(h, location->column);
        h = mix64(h, location->length);
    }
    h = mix64(h, std::hash<std::string_view>{}(message));
    return h;
}

} // namespace

bool Diagnostics::report(DiagnosticCode code,
                         ErrorLevel level,
                         const SourceLocation &location,
                         std::string_view message) {
    // Drop exact duplicates so repeated recovery paths stay quiet.
    const DiagnosticFingerprint fp =
        fingerprint_of(code, level, &location, message);
    if (!seen_.insert(fp).second) return false;

    entries_.emplace_back(level, code, location, std::string(message));
    return true;
}

bool Diagnostics::report(DiagnosticCode code,
                         ErrorLevel level,
                         std::string_view message) {
    /*
     Locationless path: code/level/message alone drive the
     fingerprint. Dedup across two distinct instances (e.g. two
     different missing files) only works if message embeds the
     distinguishing detail - otherwise they collapse to one entry.
    */
    const DiagnosticFingerprint fp =
        fingerprint_of(code, level, nullptr, message);
    if (!seen_.insert(fp).second) return false;

    entries_.emplace_back(level, code, std::nullopt, std::string(message));
    return true;
}

std::size_t Diagnostics::error_count() const {
    return static_cast<std::size_t>(std::count_if(
        entries_.begin(),
        entries_.end(),
        [](const DiagnosticMessage &msg) {
            return msg.level == ErrorLevel::ERROR ||
                   msg.level == ErrorLevel::FATAL;
        }));
}

std::size_t Diagnostics::warning_count() const {
    return static_cast<std::size_t>(std::count_if(
        entries_.begin(),
        entries_.end(),
        [](const DiagnosticMessage &msg) {
            return msg.level == ErrorLevel::WARNING;
        }));
}

void Diagnostics::clear() {
    entries_.clear();
    seen_.clear();
}

} // namespace pangea
