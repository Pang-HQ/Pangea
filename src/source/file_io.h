#pragma once

/*
 Tiny wrapper around C stdio for reading a whole source file into
 memory. Lives next to SourceBuffer because a SourceBuffer is the
 only thing the driver does with the result; if more file I/O ever
 lands in the codebase, this is the natural seam to grow.
*/

#include <optional>
#include <string>

namespace pangea {

/*
 Read the contents of path into a string. Returns nullopt if the
 file cannot be opened or read; an empty file reads as an empty
 string, which is a valid result.
*/
[[nodiscard]]
std::optional<std::string> read_file(const std::string &path);

} // namespace pangea
