// src/slider_main.cpp
#include "core/global.h"
#include <string>
#include <iostream>
#include "ui/renderer.h"
#include "core/file_utils.h"

// forward declare slider_main in ui namespace
namespace ui { int slider_main(const std::string &config_path, const std::string &csv_path, const std::string &mode, const std::string &exit_mode_flag); }

int main(int argc, char **argv) {

  global::g_exe_dir = file_utils::get_exe_dir();

  std::string config_path = global::g_exe_dir + "cfg/sliderUI_cfg.json";
  std::string csv_path = global::g_exe_dir + "gameList.csv";
  std::string mode = ""; // e.g., "kidsmode"
  std::string exit_mode_flag = ""; // e.g., "konami"

  // Simple arg parsing: accept --mode <val> --path <gameFolder> --exit <flag> and two positional args
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--mode" && i + 1 < argc) {
      mode = argv[++i];
    } else if (a == "--path" && i + 1 < argc) {
      csv_path = argv[++i];
    } else if (a == "--config" && i + 1 < argc) {
      config_path = argv[++i];
    } else if (a == "--exit" && i + 1 < argc) {
      exit_mode_flag = argv[++i];
    } else {
      // ignore unknown args
    }
  }

  std::cout << "[slider_main] config=" << config_path << " csv=" << csv_path << " mode=" << mode << " exit=" << exit_mode_flag << "\n";

  return ui::slider_main(config_path, csv_path, mode, exit_mode_flag);
}
