#include "file_system.h"
#include <fstream>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
    #define PATH_SEPARATOR '\\'
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #include <fcntl.h>
    #define PATH_SEPARATOR '/'
#endif

namespace pangea {
namespace platform {

FileError FileSystem::last_error = FileError::Unknown;

// File operations
std::optional<std::string> FileSystem::read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        last_error = FileError::NotFound;
        return std::nullopt;
    }
    
    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    
    content.assign((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
    
    return content;
}

bool FileSystem::write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        last_error = FileError::PermissionDenied;
        return false;
    }
    
    file << content;
    return file.good();
}

bool FileSystem::append_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        last_error = FileError::PermissionDenied;
        return false;
    }
    
    file << content;
    return file.good();
}

bool FileSystem::delete_file(const std::string& path) {
    try {
        return std::filesystem::remove(path);
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::PermissionDenied;
        return false;
    }
}

bool FileSystem::copy_file(const std::string& source, const std::string& destination) {
    try {
        std::filesystem::copy_file(source, destination);
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::PermissionDenied;
        return false;
    }
}

bool FileSystem::move_file(const std::string& source, const std::string& destination) {
    try {
        std::filesystem::rename(source, destination);
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::PermissionDenied;
        return false;
    }
}

// Directory operations
bool FileSystem::create_directory(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::PermissionDenied;
        return false;
    }
}

bool FileSystem::delete_directory(const std::string& path, bool recursive) {
    try {
        if (recursive) {
            return std::filesystem::remove_all(path) > 0;
        } else {
            return std::filesystem::remove(path);
        }
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::PermissionDenied;
        return false;
    }
}

std::vector<FileInfo> FileSystem::list_directory(const std::string& path) {
    std::vector<FileInfo> files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            FileInfo info;
            info.name = entry.path().filename().string();
            info.path = entry.path().string();
            info.is_directory = entry.is_directory();
            
            if (entry.is_regular_file()) {
                info.size = entry.file_size();
            } else {
                info.size = 0;
            }
            
            auto time = entry.last_write_time();
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            info.modified_time = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();
            info.created_time = info.modified_time; // Simplified
            
            files.push_back(info);
        }
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::PermissionDenied;
    }
    
    return files;
}

// Path operations
std::string FileSystem::get_current_directory() {
    try {
        return std::filesystem::current_path().string();
    } catch (const std::filesystem::filesystem_error&) {
        return "";
    }
}

bool FileSystem::set_current_directory(const std::string& path) {
    try {
        std::filesystem::current_path(path);
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::InvalidPath;
        return false;
    }
}

std::string FileSystem::get_absolute_path(const std::string& path) {
    try {
        return std::filesystem::absolute(path).string();
    } catch (const std::filesystem::filesystem_error&) {
        return path;
    }
}

std::string FileSystem::get_parent_directory(const std::string& path) {
    try {
        return std::filesystem::path(path).parent_path().string();
    } catch (const std::filesystem::filesystem_error&) {
        return "";
    }
}

std::string FileSystem::join_paths(const std::string& path1, const std::string& path2) {
    return (std::filesystem::path(path1) / path2).string();
}

std::string FileSystem::get_file_extension(const std::string& path) {
    return std::filesystem::path(path).extension().string();
}

std::string FileSystem::get_file_name(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

// File info
bool FileSystem::file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool FileSystem::is_directory(const std::string& path) {
    return std::filesystem::is_directory(path);
}

bool FileSystem::is_file(const std::string& path) {
    return std::filesystem::is_regular_file(path);
}

std::optional<FileInfo> FileSystem::get_file_info(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            last_error = FileError::NotFound;
            return std::nullopt;
        }
        
        FileInfo info;
        std::filesystem::path fs_path(path);
        info.name = fs_path.filename().string();
        info.path = fs_path.string();
        info.is_directory = std::filesystem::is_directory(path);
        
        if (std::filesystem::is_regular_file(path)) {
            info.size = std::filesystem::file_size(path);
        } else {
            info.size = 0;
        }
        
        auto time = std::filesystem::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        info.modified_time = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();
        info.created_time = info.modified_time; // Simplified
        
        return info;
    } catch (const std::filesystem::filesystem_error&) {
        last_error = FileError::Unknown;
        return std::nullopt;
    }
}

uint64_t FileSystem::get_file_size(const std::string& path) {
    try {
        return std::filesystem::file_size(path);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

// Error handling
FileError FileSystem::get_last_error() {
    return last_error;
}

std::string FileSystem::error_to_string(FileError error) {
    switch (error) {
        case FileError::NotFound: return "File not found";
        case FileError::PermissionDenied: return "Permission denied";
        case FileError::AlreadyExists: return "File already exists";
        case FileError::InvalidPath: return "Invalid path";
        case FileError::DiskFull: return "Disk full";
        case FileError::Unknown: return "Unknown error";
        default: return "Unknown error";
    }
}

// FileHandle implementation
FileHandle::FileHandle(const std::string& path, FileMode mode) 
    : handle(nullptr), mode(mode), last_error(FileError::Unknown), is_open_flag(false) {
    
    std::ios::openmode open_mode = std::ios::binary;
    
    switch (mode) {
        case FileMode::Read:
            open_mode |= std::ios::in;
            break;
        case FileMode::Write:
            open_mode |= std::ios::out | std::ios::trunc;
            break;
        case FileMode::Append:
            open_mode |= std::ios::out | std::ios::app;
            break;
        case FileMode::ReadWrite:
            open_mode |= std::ios::in | std::ios::out;
            break;
    }
    
    auto* file = new std::fstream(path, open_mode);
    if (file->is_open()) {
        handle = file;
        is_open_flag = true;
    } else {
        delete file;
        last_error = FileError::NotFound;
    }
}

FileHandle::~FileHandle() {
    close();
}

FileHandle::FileHandle(FileHandle&& other) noexcept
    : handle(other.handle), mode(other.mode), last_error(other.last_error), is_open_flag(other.is_open_flag) {
    other.handle = nullptr;
    other.is_open_flag = false;
}

FileHandle& FileHandle::operator=(FileHandle&& other) noexcept {
    if (this != &other) {
        close();
        handle = other.handle;
        mode = other.mode;
        last_error = other.last_error;
        is_open_flag = other.is_open_flag;
        other.handle = nullptr;
        other.is_open_flag = false;
    }
    return *this;
}

bool FileHandle::is_open() const {
    return is_open_flag;
}

void FileHandle::close() {
    if (handle) {
        delete static_cast<std::fstream*>(handle);
        handle = nullptr;
        is_open_flag = false;
    }
}

std::optional<std::string> FileHandle::read_line() {
    if (!is_open()) return std::nullopt;
    
    auto* file = static_cast<std::fstream*>(handle);
    std::string line;
    if (std::getline(*file, line)) {
        return line;
    }
    return std::nullopt;
}

std::optional<std::string> FileHandle::read_all() {
    if (!is_open()) return std::nullopt;
    
    auto* file = static_cast<std::fstream*>(handle);
    std::string content;
    
    file->seekg(0, std::ios::end);
    content.reserve(file->tellg());
    file->seekg(0, std::ios::beg);
    
    content.assign((std::istreambuf_iterator<char>(*file)),
                   std::istreambuf_iterator<char>());
    
    return content;
}

std::optional<std::vector<uint8_t>> FileHandle::read_bytes(size_t count) {
    if (!is_open()) return std::nullopt;
    
    auto* file = static_cast<std::fstream*>(handle);
    std::vector<uint8_t> buffer(count);
    
    file->read(reinterpret_cast<char*>(buffer.data()), count);
    buffer.resize(file->gcount());
    
    return buffer;
}

bool FileHandle::write(const std::string& data) {
    if (!is_open()) return false;
    
    auto* file = static_cast<std::fstream*>(handle);
    *file << data;
    return file->good();
}

bool FileHandle::write_line(const std::string& line) {
    return write(line + "\n");
}

bool FileHandle::write_bytes(const std::vector<uint8_t>& data) {
    if (!is_open()) return false;
    
    auto* file = static_cast<std::fstream*>(handle);
    file->write(reinterpret_cast<const char*>(data.data()), data.size());
    return file->good();
}

bool FileHandle::flush() {
    if (!is_open()) return false;
    
    auto* file = static_cast<std::fstream*>(handle);
    file->flush();
    return file->good();
}

bool FileHandle::seek(uint64_t position) {
    if (!is_open()) return false;
    
    auto* file = static_cast<std::fstream*>(handle);
    file->seekg(position);
    file->seekp(position);
    return file->good();
}

uint64_t FileHandle::tell() const {
    if (!is_open()) return 0;
    
    auto* file = static_cast<std::fstream*>(handle);
    return file->tellg();
}

uint64_t FileHandle::size() const {
    if (!is_open()) return 0;
    
    auto* file = static_cast<std::fstream*>(handle);
    auto current = file->tellg();
    file->seekg(0, std::ios::end);
    auto size = file->tellg();
    file->seekg(current);
    return size;
}

FileError FileHandle::get_last_error() const {
    return last_error;
}

} // namespace platform
} // namespace pangea
