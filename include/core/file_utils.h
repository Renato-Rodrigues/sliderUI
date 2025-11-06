#pragma once
#ifndef SLIDERUI_CORE_FILE_UTILS_H
#define SLIDERUI_CORE_FILE_UTILS_H

#include <string>
#include <cstdint>

namespace file_utils {

/**
 * Atomically write 'contents' to 'path'.
 * Implementation: create a secure temp file in the same directory, write, fsync, close, rename -> path.
 * Returns true on success, false on failure (no partial/truncated final file left).
 */
bool atomic_write(const std::string &path, const std::string &contents);

/**
 * Return true if file exists and is accessible (F_OK).
 */
bool file_exists(const std::string &path);

/**
 * Return file modification time (seconds since epoch) as uint64_t.
 * On error returns 0 and does not throw.
 */
uint64_t file_mtime(const std::string &path);

} // namespace file_utils

#endif // SLIDERUI_CORE_FILE_UTILS_H
