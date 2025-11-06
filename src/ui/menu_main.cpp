// src/menu_main.cpp
#include "core/global.h"
#include <iostream>
#include <string>
#include "ui/menu_ui.h"
#include "core/file_utils.h"

int main(int argc, char **argv) {

    global::g_exe_dir = file_utils::get_exe_dir();

    std::string config_path;
    if (argc > 1) config_path = argv[1];
    else config_path = global::g_exe_dir + "cfg/sliderUI_cfg.json"; // fallback for local testing

    std::cout << "[menu_main] using config: " << config_path << "\n";
    return ui::menu_main(config_path);
}
