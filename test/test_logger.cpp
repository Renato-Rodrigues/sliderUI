#include "core/logger.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using core::Logger;

static bool file_contains(const fs::path &p, const std::string &needle) {
  std::ifstream f(p);
  if (!f) return false;
  std::string s;
  std::string all;
  while (std::getline(f, s)) { all += s; all.push_back('\n'); }
  return all.find(needle) != std::string::npos;
}

int main() {
  std::cout << "[test] logger: running tests\n";
  std::string tmpdir = "/tmp/sliderui_logger_test";
  // clean
  try { fs::remove_all(tmpdir); } catch (...) {}
  fs::create_directories(tmpdir);

  Logger &lg = Logger::instance();
  bool ok = lg.init(tmpdir, 3, 100); // small threshold to force rotations
  if (!ok) {
    std::cerr << "[FAIL] logger init failed\n";
    return 1;
  }

  // write many messages to trigger multiple rotations
  for (int i = 0; i < 15; ++i) {
    lg.info("message number " + std::to_string(i));
  }
  // final flush just to ensure buffer written
  lg.rotate_and_flush();

  // check number of files present <= max_files
  int count = 0;
  for (int i = 0; i < 10; ++i) {
    fs::path p = fs::path(tmpdir) / ("log." + std::to_string(i));
    if (fs::exists(p)) ++count;
  }
  if (count == 0) {
    std::cerr << "[FAIL] no log files created\n";
    return 2;
  }
  if (count > 3) {
    std::cerr << "[FAIL] too many log files: " << count << "\n";
    return 3;
  }

  // log.0 must exist and contain last messages
  fs::path newest = fs::path(tmpdir) / "log.0";
  if (!fs::exists(newest)) {
    std::cerr << "[FAIL] log.0 missing\n";
    return 4;
  }
  if (!file_contains(newest, "message number")) {
    std::cerr << "[FAIL] content missing in log.0\n";
    return 5;
  }

  // cleanup
  try { fs::remove_all(tmpdir); } catch (...) {}

  std::cout << "[OK] logger tests passed\n";
  return 0;
}
