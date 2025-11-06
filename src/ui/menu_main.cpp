// src/menu_main.cpp
#include <iostream>
#include <string>
#include "ui/menu_ui.h"
int main(int argc, char **argv) {
    std::string config_path;
    if (argc > 1) config_path = argv[1];
    else config_path = "./sliderUI_cfg.json"; // fallback for local testing

    std::cout << "[menu_main] using config: " << config_path << "\n";
    return ui::menu_main(config_path);
}
