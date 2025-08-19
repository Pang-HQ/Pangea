#include "source_location.h"
#include <sstream>

namespace pangea {

std::string SourceLocation::toString() const {
    std::ostringstream oss;
    if (!filename.empty()) {
        oss << filename << ":";
    }
    oss << line << ":" << column;
    return oss.str();
}

} // namespace pangea
