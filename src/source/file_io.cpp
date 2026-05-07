#include "file_io.h"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <sys/stat.h>

namespace pangea {

namespace {

struct FileCloser {
    void operator()(std::FILE *fp) const noexcept {
        if (fp != nullptr) {
            std::fclose(fp);
        }
    }
};

using ScopedFile = std::unique_ptr<std::FILE, FileCloser>;

/*
 Reject directories, devices, FIFOs, and other non-regular paths up
 front. fseek/ftell give nonsensical answers on those, and the lexer
 should never have to defend against /dev/zero or a directory entry.
 S_IFMT and S_IFREG are defined on both POSIX and MSVC's <sys/stat.h>.
*/
bool is_regular_file(const std::string &path) noexcept {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) {
        return false;
    }
    return (st.st_mode & S_IFMT) == S_IFREG;
}

} // namespace

std::optional<std::string> read_file(const std::string &path) {
    if (!is_regular_file(path)) {
        return std::nullopt;
    }

    ScopedFile fp{std::fopen(path.c_str(), "rb")};
    if (!fp) {
        return std::nullopt;
    }

    /*
     Size up front via fseek/ftell so the result string is sized
     exactly and read in one fread call. Streaming chunk-by-chunk
     would incur reallocations for no benefit at compiler-input
     sizes.
    */
    if (std::fseek(fp.get(), 0, SEEK_END) != 0) {
        return std::nullopt;
    }
    const long size = std::ftell(fp.get());
    if (size < 0) {
        return std::nullopt;
    }
    std::rewind(fp.get());

    /*
     resize_and_overwrite (C++23) lets fread write straight into the
     string's storage with no zero-init pass. The lambda returns the
     bytes actually read, so a short read leaves out.size() < want
     and we surface that as a failure.
    */
    const std::size_t want = static_cast<std::size_t>(size);
    std::string out;
    out.resize_and_overwrite(want,
        [raw = fp.get()](char *data, std::size_t cap) noexcept -> std::size_t {
            return std::fread(data, 1, cap, raw);
        });

    if (out.size() != want) {
        return std::nullopt;
    }
    return out;
}

} // namespace pangea
