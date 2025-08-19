#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace pangea {
namespace platform {

enum class FileMode {
    Read,
    Write,
    Append,
    ReadWrite
};

enum class FileError {
    NotFound,
    PermissionDenied,
    AlreadyExists,
    InvalidPath,
    DiskFull,
    Unknown
};

struct FileInfo {
    std::string name;
    std::string path;
    uint64_t size;
    bool is_directory;
    uint64_t modified_time;
    uint64_t created_time;
};

class FileSystem {
public:
    // File operations
    static std::optional<std::string> read_file(const std::string& path);
    static bool write_file(const std::string& path, const std::string& content);
    static bool append_file(const std::string& path, const std::string& content);
    static bool delete_file(const std::string& path);
    static bool copy_file(const std::string& source, const std::string& destination);
    static bool move_file(const std::string& source, const std::string& destination);
    
    // Directory operations
    static bool create_directory(const std::string& path);
    static bool delete_directory(const std::string& path, bool recursive = false);
    static std::vector<FileInfo> list_directory(const std::string& path);
    
    // Path operations
    static std::string get_current_directory();
    static bool set_current_directory(const std::string& path);
    static std::string get_absolute_path(const std::string& path);
    static std::string get_parent_directory(const std::string& path);
    static std::string join_paths(const std::string& path1, const std::string& path2);
    static std::string get_file_extension(const std::string& path);
    static std::string get_file_name(const std::string& path);
    
    // File info
    static bool file_exists(const std::string& path);
    static bool is_directory(const std::string& path);
    static bool is_file(const std::string& path);
    static std::optional<FileInfo> get_file_info(const std::string& path);
    static uint64_t get_file_size(const std::string& path);
    
    // Error handling
    static FileError get_last_error();
    static std::string error_to_string(FileError error);

private:
    static FileError last_error;
};

// File handle for streaming operations
class FileHandle {
public:
    FileHandle(const std::string& path, FileMode mode);
    ~FileHandle();
    
    // Disable copy, enable move
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&& other) noexcept;
    FileHandle& operator=(FileHandle&& other) noexcept;
    
    bool is_open() const;
    void close();
    
    // Reading
    std::optional<std::string> read_line();
    std::optional<std::string> read_all();
    std::optional<std::vector<uint8_t>> read_bytes(size_t count);
    
    // Writing
    bool write(const std::string& data);
    bool write_line(const std::string& line);
    bool write_bytes(const std::vector<uint8_t>& data);
    bool flush();
    
    // Position
    bool seek(uint64_t position);
    uint64_t tell() const;
    uint64_t size() const;
    
    FileError get_last_error() const;

private:
    void* handle;  // Platform-specific file handle
    FileMode mode;
    FileError last_error;
    bool is_open_flag;
};

} // namespace platform
} // namespace pangea
