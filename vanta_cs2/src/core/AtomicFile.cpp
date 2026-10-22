#include "core/AtomicFile.hpp"

#include <fstream>
#include <system_error>
#include <windows.h>

namespace vanta::io {
namespace fs = std::filesystem;

bool write_atomic(const fs::path& path, std::string_view contents) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);

    auto tmp = path;
    tmp += L".tmp." + std::to_wstring(::GetCurrentProcessId());

    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) return false;
        f.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        f.flush();
        if (!f.good()) {
            fs::remove(tmp, ec);
            return false;
        }
    }

    // Try to flush to disk explicitly.
    HANDLE h = ::CreateFileW(tmp.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        ::FlushFileBuffers(h);
        ::CloseHandle(h);
    }

    // MoveFileEx with REPLACE_EXISTING is atomic on NTFS.
    if (!::MoveFileExW(tmp.c_str(), path.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

bool read_file(const fs::path& path, std::string& out, std::size_t max_bytes) {
    std::error_code ec;
    auto sz = fs::file_size(path, ec);
    if (ec) return false;
    if (sz > max_bytes) return false;

    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return f.good() || f.eof();
}

} // namespace vanta::io
