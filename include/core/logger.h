#pragma once
#ifndef SLIDERUI_CORE_LOGGER_H
#define SLIDERUI_CORE_LOGGER_H

#include <string>
#include <memory>

namespace core {

/**
 * Logger - simple singleton logger with rotation.
 *
 * Usage:
 *   Logger::instance().init("/tmp/mylogs", 5);
 *   Logger::instance().info("started");
 *   Logger::instance().error("failed");
 *
 * Notes:
 * - init() should be called once early; calling again will reinitialize.
 * - info()/error() buffer entries in memory; buffer flushes to disk only when rotation triggers.
 * - On rotation the logger writes buffered data to 'log.0' and shifts older files to log.1...log.N-1.
 */
class Logger {
public:
  // Get singleton
  static Logger& instance();

  // Initialize logger: directory, max files to keep, optional buffer threshold bytes to trigger rotation.
  // Returns true on success (e.g., directory created or writable).
  bool init(const std::string &dir, size_t max_files, size_t buffer_threshold_bytes = 1024);

  // Append info-level message
  void info(const std::string &msg);

  // Append error-level message
  void error(const std::string &msg);

  // Force flush/rotation (testing or graceful shutdown)
  void rotate_and_flush();

private:
  Logger();
  ~Logger();

  // non-copyable
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  // private impl details in cpp
};

} // namespace core

#endif // SLIDERUI_CORE_LOGGER_H
