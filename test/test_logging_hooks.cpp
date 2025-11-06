// test/test_logging_hooks.cpp
#include "core/logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
#include <unistd.h>

namespace fs = std::filesystem;
using core::Logger;

// forward declare helpers implemented in menu_ui.cpp and slider_ui.cpp
namespace ui {
  void menu_log_actions_for_test(const std::string &config_path);
  void slider_log_actions_for_test(const std::string &config_path, const std::string &csv_path);
}

static std::string read_file(const fs::path &p) {
  std::ifstream f(p);
  std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  return s;
}

int main() {
  std::cout << "[test] logging_hooks: running\n";
  std::string tmpdir = "/tmp/sliderui_logging_test_" + std::to_string(getpid());
  fs::remove_all(tmpdir);
  fs::create_directories(tmpdir);

  // Test initial log message
  Logger &lg_initial = Logger::instance();
  lg_initial.init(tmpdir, 3, 512); // Use larger buffer to prevent too-frequent rotation
  lg_initial.info("test: initial log message");
  lg_initial.rotate_and_flush();

  // Verify the initial message
  fs::path test_log = fs::path(tmpdir) / "log.0";
  if (!fs::exists(test_log)) {
    std::cerr << "[DEBUG] Initial test log does not exist!\n";
  } else {
    std::cerr << "[DEBUG] Initial test log contents:\n" << read_file(test_log) << "\n";
  }

  // Init logger is now done by the initial test above
  std::cerr << "[DEBUG] Logger already initialized\n";

  // create sample config json
  fs::path cfg = fs::path(tmpdir) / "sliderUI_cfg.json";
  {
    std::ofstream out(cfg);
    out << "{\n"
           "  \"behavior\": {\n"
           "    \"sort_mode\": \"alphabetical\",\n"
           "    \"start_game\": \"last_played\",\n"
           "    \"kids_mode_enabled\": false\n"
           "  }\n"
           "}\n";
  }

  // create sample gameList.csv
  fs::path csv = fs::path(tmpdir) / "gameList.csv";
  {
    std::ofstream out(csv);
    out << "/tmp/games/foo;0;Foo;1999\n";
    out << "/tmp/games/bar;1;Bar;2001\n";
  }

  // Call menu helper (should log load, sort change, kids toggle)
  ui::menu_log_actions_for_test(cfg.string());
  // Call slider helper (should log load csv, ensure orders, sort change, remove/commit)
  ui::slider_log_actions_for_test(cfg.string(), csv.string());

  // Force flush/rotate to ensure logs are on disk
  Logger::instance().rotate_and_flush();

  // Read all log files (log.0, log.1, log.2) to get complete history
  std::string content;
  for (int i = 0; i < 3; i++) {
    fs::path log = fs::path(tmpdir) / ("log." + std::to_string(i));
    if (fs::exists(log)) {
      std::cerr << "[DEBUG] Reading log." << i << ":\n" << read_file(log) << "\n";
      content += read_file(log);
    }
  }
  
  if (content.empty()) {
    std::cerr << "[FAIL] No log files found\n";
    return 2;
  }

  // Check for evidence of each logged action
  auto contains = [&](const std::string &needle)->bool {
    return content.find(needle) != std::string::npos;
  };

  if (!contains("menu: loaded config")) { std::cerr << "[FAIL] missing menu: loaded config\n"; return 3; }
  if (!contains("menu: sort change")) { std::cerr << "[FAIL] missing menu: sort change\n"; return 4; }
  if (!contains("menu: kids_mode")) { std::cerr << "[FAIL] missing menu: kids_mode\n"; return 5; }

  if (!contains("slider: loaded csv")) { std::cerr << "[FAIL] missing slider: loaded csv\n"; return 6; }
  if (!contains("slider: ensured orders assigned")) { std::cerr << "[FAIL] missing slider: ensured orders assigned\n"; return 7; }
  if (!contains("slider: sort change")) { std::cerr << "[FAIL] missing slider: sort change\n"; return 8; }
  // either commit success or remove message should appear
  if (!contains("slider: removed") && !contains("slider: no games to remove")) {
    std::cerr << "[FAIL] missing slider: removed or no games message\n"; return 9;
  }

  // cleanup
  fs::remove_all(tmpdir);

  std::cout << "[OK] logging_hooks tests passed\n";
  return 0;
}
