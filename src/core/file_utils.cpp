#include "core/file_utils.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <vector>

namespace file_utils {

static std::string make_tmp_template(const std::string &path) {
    // Create a template for mkstemp in the same directory as 'path'.
    // Objective: /some/dir/filename -> /some/dir/filename.tmp.XXXXXX
    std::string tmp = path + ".tmp.XXXXXX";
    return tmp;
}

bool atomic_write(const std::string &path, const std::string &contents) {
    if (path.empty()) return false;

    std::string tmp_template = make_tmp_template(path);
    // mkstemp modifies the buffer; create writable copy
    std::vector<char> buf(tmp_template.begin(), tmp_template.end());
    buf.push_back('\0');

    int fd = mkstemp(buf.data());
    if (fd < 0) {
        // mkstemp failed
        return false;
    }

    // Write contents fully
    const char *data = contents.data();
    size_t to_write = contents.size();
    ssize_t written = 0;
    size_t offset = 0;
    bool ok = true;

    while (to_write > 0) {
        written = ::write(fd, data + offset, to_write);
        if (written < 0) {
            ok = false;
            break;
        }
        offset += static_cast<size_t>(written);
        to_write -= static_cast<size_t>(written);
    }

    // Ensure data is on stable storage
    if (ok) {
        if (fsync(fd) != 0) {
            ok = false;
        }
    }

    // Close file descriptor
    if (close(fd) != 0) {
        ok = ok && true; // close errors are non-fatal for our purposes, but we keep ok state
    }

    const char *tmp_path = buf.data();

    if (!ok) {
        // Remove temp file if it exists
        unlink(tmp_path);
        return false;
    }

    // Atomically rename temp file to destination path
    if (rename(tmp_path, path.c_str()) != 0) {
        // Rename failed; try to remove temp file
        unlink(tmp_path);
        return false;
    }

    return true;
}

bool file_exists(const std::string &path) {
    if (path.empty()) return false;
    return (access(path.c_str(), F_OK) == 0);
}

uint64_t file_mtime(const std::string &path) {
    if (path.empty()) return 0;
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return 0;
    }
#if defined(__APPLE__)
    // macOS has st_mtimespec.tv_sec
    return static_cast<uint64_t>(st.st_mtimespec.tv_sec);
#else
    return static_cast<uint64_t>(st.st_mtime);
#endif
}

} // namespace file_utils
