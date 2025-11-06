// src/ui/menu_ui.h
#pragma once

#include <string>

namespace ui {

/**
 * menu_main
 *
 * Main entry point for the sliderUI menu. Loads config, handles
 * menu navigation (sort, start_game, kids_mode, games list), and
 * saves config atomically on changes.
 *
 * @param config_path Path to JSON config file
 * @return 0 on normal exit
 */
int menu_main(const std::string &config_path);

/**
 * menu_log_actions_for_test
 *
 * Optional helper function to trigger menu actions for automated testing.
 * Can log events without starting the full interactive menu.
 *
 * @param config_path Path to JSON config file
 */
void menu_log_actions_for_test(const std::string &config_path);

} // namespace ui
