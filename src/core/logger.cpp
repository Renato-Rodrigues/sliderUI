#include "core/logger.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <vector>
#include <cstdio>
#include <iostream>

namespace fs = std::filesystem;

namespace core {

struct LoggerState {
  std::string dir;
  size_t max_files = 5;
  size_t buffer_threshold = 1024;
  std::string buffer; // in-memory buffered log lines
  std::mutex mtx;
  bool initialized = false;
};

static LoggerState &state() {
  static LoggerState s;
  return s;
}

Logger& Logger::instance() {
  static Logger inst;
  return inst;
}

Logger::Logger() {}
Logger::~Logger() {
  // attempt to flush buffered logs on destruction
  try {
    rotate_and_flush();
  } catch (...) { /* swallow */ }
}

bool Logger::init(const std::string &dir, size_t max_files, size_t buffer_threshold_bytes) {
  std::lock_guard<std::mutex> g(state().mtx);
  try {
    if (!dir.empty()) {
      fs::path p(dir);
      if (!fs::exists(p)) {
        fs::create_directories(p);
      } else if (!fs::is_directory(p)) {
        return false;
      }
    }
  } catch (...) {
    return false;
  }
  state().dir = dir;
  state().max_files = (max_files < 1) ? 1 : max_files;
  state().buffer_threshold = (buffer_threshold_bytes < 1) ? 1 : buffer_threshold_bytes;
  state().buffer.clear();
  state().initialized = true;
  return true;
}

// produce ISO8601 timestamp with seconds precision
static std::string now_iso8601() {
  using namespace std::chrono;
  auto t = system_clock::now();
  std::time_t tt = system_clock::to_time_t(t);
  auto ms = duration_cast<milliseconds>(t.time_since_epoch()) % 1000;
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &tt);
#else
  gmtime_r(&tt, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << "Z";
  return oss.str();
}

static void append_line_to_buffer(const std::string &level, const std::string &msg) {
  auto &st = state();
  std::ostringstream oss;
  oss << now_iso8601() << " [" << level << "] " << msg << "\n";
  st.buffer += oss.str();
  std::cerr << "[DEBUG] Logger: Appended to buffer. Current size: " << st.buffer.size() 
            << ", threshold: " << st.buffer_threshold << "\n";
}

// rotate files and write buffer to log.0, clear buffer. Must be called with lock held or will lock internally.
void Logger::rotate_and_flush() {
  // Acquire lock
  std::lock_guard<std::mutex> g(state().mtx);
  if (!state().initialized) return;

  // If nothing to write, still ensure rotation invariants (optional)
  if (state().buffer.empty()) {
    // no buffered content; nothing to flush
    return;
  }

  try {
    fs::path dirp(state().dir);
    // ensure directory exists
    if (!dirp.empty() && !fs::exists(dirp)) fs::create_directories(dirp);

    std::cerr << "[DEBUG] Logger: Writing to directory: " << state().dir << "\n";
    std::cerr << "[DEBUG] Logger: Buffer contents: " << state().buffer << "\n";

    // remove oldest if exists: log.(max_files-1)
    if (state().max_files > 0) {
      fs::path oldest = dirp / ("log." + std::to_string(state().max_files - 1));
      if (fs::exists(oldest)) {
        fs::remove(oldest);
      }
      // shift i<-i-1 for i=max_files-1..1
      for (size_t i = state().max_files - 1; i-- > 0; ) {
        fs::path from = dirp / ("log." + std::to_string(i));
        fs::path to = dirp / ("log." + std::to_string(i+1));
        if (fs::exists(from)) {
          // overwrite target if exists
          if (fs::exists(to)) fs::remove(to);
          fs::rename(from, to);
        }
        if (i == 0) break; // size_t underflow guard
      }
    }

    // write buffer to log.0
    fs::path outpath = dirp / "log.0";
    std::ofstream ofs(outpath, std::ios::binary | std::ios::trunc);
    if (!ofs) {
      // writing failed; drop buffer to avoid infinite loop
      state().buffer.clear();
      return;
    }
    ofs << state().buffer;
    // do not flush every write per requirement; we flush on rotation which is now
    ofs.flush();
    ofs.close();
  } catch (...) {
    // on any filesystem error, clear buffer (best effort) and return
    state().buffer.clear();
    return;
  }
  // clear buffer after successful write
  state().buffer.clear();
}

void Logger::info(const std::string &msg) {
  {
    std::lock_guard<std::mutex> g(state().mtx);
    if (!state().initialized) {
      std::cerr << "[DEBUG] Logger: Not initialized, skipping message: " << msg << "\n";
      return;
    }
    append_line_to_buffer("INFO", msg);
    if (state().buffer.size() >= state().buffer_threshold) {
      std::cerr << "[DEBUG] Logger: Buffer threshold reached, will flush\n";
      // release lock and rotate (rotate_and_flush will lock internally)
    } else {
      std::cerr << "[DEBUG] Logger: Buffer below threshold, deferring flush\n";
      return;
    }
  }
  // call rotate outside inner lock (rotate_and_flush locks itself)
  rotate_and_flush();
}

void Logger::error(const std::string &msg) {
  {
    std::lock_guard<std::mutex> g(state().mtx);
    if (!state().initialized) return;
    append_line_to_buffer("ERROR", msg);
    if (state().buffer.size() >= state().buffer_threshold) {
      // fall through to rotate after unlocking
    } else {
      return;
    }
  }
  rotate_and_flush();
}

} // namespace core
