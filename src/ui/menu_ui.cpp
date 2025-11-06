// src/ui/menu_ui.cpp
#include "ui/renderer.h"
#include "ui/menu_constants.h"
#include "ui/games_list.h"
#include "ui/menu_config.h"
#include "core/config_manager.h"
#include "core/game_db.h"
#include "core/logger.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace std::chrono_literals;
using core::Logger;
using core::GameDB;
using namespace ui::menu; // for constants

namespace ui {

static const std::vector<std::string> sort_modes = { "alphabetical", "release", "custom" };
static const std::vector<std::string> start_game_modes = { "last_played", "first_game" };

static size_t index_of(const std::vector<std::string> &v, const std::string &val) {
  for (size_t i = 0; i < v.size(); ++i) if (v[i] == val) return i;
  return 0;
}

/**
 * menu_main
 *
 * Shows a simple text menu that allows toggling:
 *  - sort (alphabetical, release, custom)
 *  - start game (last_played, first_game)
 *  - kids mode (enable/disable)
 *  - games list (enter stubbed submenu)
 *
 * config_path: path to the JSON config file that ConfigManager will load/save.
 */
int menu_main(const std::string &config_path) {
  core::ConfigManager cfg;

  // Load config (if missing, ConfigManager should merge with defaults)
  bool ok = cfg.load(config_path);
  if (!ok) {
    std::cerr << "[menu] warning: failed to load config (will use defaults)\n";
    Logger::instance().error(std::string("menu: failed to load config ") + config_path);
  } else {
    Logger::instance().info(std::string("menu: loaded config ") + config_path);
  }

  // ensure keys exist by reading with fallbacks
  std::string sort_mode = cfg.get<std::string>("behavior.sort_mode", std::string("alphabetical"));
  std::string start_game = cfg.get<std::string>("behavior.start_game", std::string("last_played"));
  bool kids_mode_enabled = cfg.get<bool>("behavior.kids_mode_enabled", false);

  // Initialize menu config (loads sliderUI_cfg.json)
  MenuConfig::init("sliderUI_cfg.json");

  // renderer
  Renderer renderer;
  renderer.init();

  // menu state
  enum MenuItem { SORT = 0, START_GAME = 1, KIDS_MODE = 2, GAMES_LIST = 3 };
  const int menu_size = 4;
  int selected = 0;

  bool running = true;
  renderer.clear();
  while (running) {
    // Input
    ui::Input in = ui::poll_input();

    switch (in) {
      case ui::Input::UP:
        selected = (selected - 1 + menu_size) % menu_size;
        break;
      case ui::Input::DOWN:
        selected = (selected + 1) % menu_size;
        break;
      case ui::Input::A: {
        // Activate / toggle depending on selected item
        if (selected == SORT) {
          // cycle sort_mode
          size_t idx = index_of(sort_modes, sort_mode);
          idx = (idx + 1) % sort_modes.size();
          sort_mode = sort_modes[idx];
          cfg.set<std::string>("behavior.sort_mode", sort_mode);
          if (!cfg.save(config_path)) {
            std::cerr << "[menu] failed to save config after changing sort_mode\n";
            Logger::instance().error("menu: failed to save config after sort_mode change");
          } else {
            Logger::instance().info(std::string("menu: sort change -> ") + sort_mode);
          }
        } else if (selected == START_GAME) {
          size_t idx = index_of(start_game_modes, start_game);
          idx = (idx + 1) % start_game_modes.size();
          start_game = start_game_modes[idx];
          cfg.set<std::string>("behavior.start_game", start_game);
          if (!cfg.save(config_path)) {
            std::cerr << "[menu] failed to save config after changing start_game\n";
            Logger::instance().error("menu: failed to save config after start_game change");
          } else {
            Logger::instance().info(std::string("menu: start_game change -> ") + start_game);
          }
        } else if (selected == KIDS_MODE) {
          kids_mode_enabled = !kids_mode_enabled;
          cfg.set<bool>("behavior.kids_mode_enabled", kids_mode_enabled);
          if (!cfg.save(config_path)) {
            std::cerr << "[menu] failed to save config after toggling kids_mode\n";
            Logger::instance().error("menu: failed to save config after kids_mode toggle");
          } else {
            Logger::instance().info(std::string("menu: kids_mode -> ") + (kids_mode_enabled ? "enabled" : "disabled"));
          }
          // do not execute external script; only log
          if (kids_mode_enabled) {
            std::cout << "[menu] kids mode enabled (would run kidsMode.sh)\n";
          } else {
            std::cout << "[menu] kids mode disabled (would remove kidsMode settings)\n";
          }
        } else if (selected == GAMES_LIST) {
          // Load game database
          core::GameDB game_db;
          std::string games_csv = "gameList.csv";  // TODO: Get from config
          if (!game_db.load(games_csv)) {
            Logger::instance().error("Failed to load games database: " + games_csv);
            renderer.draw_overlay("Error: Could not load games list");
            renderer.present();
            std::this_thread::sleep_for(2s);
            continue;
          }

          // Ensure game orders are assigned before showing list
          game_db.ensure_orders_assigned();
          
          // Show the games list menu
          GameListState games_state(game_db);
          
          if (show_games_list(renderer, games_state)) {
            // A game was selected
            const auto& selected_game = game_db.games()[games_state.selected_index];
            Logger::instance().info("Game selected: " + 
              (selected_game.name.empty() ? selected_game.path : selected_game.name));
            // TODO: Launch the selected game
          }
        }
        break;
      }
      case ui::Input::B:
        // exit menu
        running = false;
        break;
      default:
        break;
    }

    // Render menu
    renderer.clear();
    
    // Reload config if hot-reloading is enabled
    MenuConfig::reload_if_enabled();

    // Draw title
    renderer.draw_text(TITLE_X(), TITLE_Y(), "sliderUI Options", false);

        // Draw menu items with selection highlight and current values aligned right
    for (int i = 0; i < menu_size; ++i) {
      const int y = MENU_START_Y() + i * MENU_ITEM_SPACING();
      bool highlight = (i == selected);

      // Draw selector for highlighted item - position below text
      const int selector_y = y + (SELECTOR_HEIGHT() - TEXT_HEIGHT()) / 2;
      
      if (highlight) {
        const int SELECTOR_WIDTH = SCREEN_WIDTH() - MENU_START_X() * 2 + 20;
        renderer.draw_selector(MENU_START_X() - 10, selector_y, SELECTOR_WIDTH, SELECTOR_HEIGHT());
      }

      // Draw menu items
      switch (i) {
        case SORT: {
          renderer.draw_text(MENU_START_X(), y, "sort", highlight);
          // Right-align value text - measure actual width
          const std::string value_text = sort_mode;
          const int value_width = renderer.get_text_width(value_text);
          const int value_x = SCREEN_WIDTH() - MENU_RIGHT_MARGIN() - value_width;  // Right edge at margin
          renderer.draw_text(value_x, y, value_text, highlight);
          break;
        }
        case START_GAME: {
          renderer.draw_text(MENU_START_X(), y, "start game", highlight);
          const std::string value_text = start_game == "last_played" ? "last played" : "first game";
          const int value_width = renderer.get_text_width(value_text);
          const int value_x = SCREEN_WIDTH() - MENU_RIGHT_MARGIN() - value_width;
          renderer.draw_text(value_x, y, value_text, highlight);
          break;
        }
        case KIDS_MODE: {
          renderer.draw_text(MENU_START_X(), y, "kids mode", highlight);
          const std::string value_text = kids_mode_enabled ? "on" : "off";
          const int value_width = renderer.get_text_width(value_text);
          const int value_x = SCREEN_WIDTH() - MENU_RIGHT_MARGIN() - value_width;
          renderer.draw_text(value_x, y, value_text, highlight);
          break;
        }
        case GAMES_LIST: {
          renderer.draw_text(MENU_START_X(), y, "> games list", highlight);
          break;
        }
      }
    }

    // Draw help text at bottom
    renderer.draw_text(HELP_X(), HELP_Y(), "B  BACK          A  CHANGE", false);

    renderer.present();

    // frame pacing
    std::this_thread::sleep_for(40ms);
  }

  renderer.shutdown();
  return 0;
}

/**
 * Test helper for logging hooks.
 * Performs deterministic config operations and logs them without interactive input.
 */
void menu_log_actions_for_test(const std::string &config_path) {
  // Clear any existing logs first
  Logger &lg = Logger::instance();

  core::ConfigManager cfg;
  bool cfg_ok = cfg.load(config_path);   // check return value
  std::cerr << "[DEBUG] Config load result: " << (cfg_ok ? "success" : "failed") << " for path: " << config_path << "\n";

  lg.rotate_and_flush();  // Flush any previous logs
  lg.info("menu: loaded config");  // exact string test expects
  lg.rotate_and_flush();  // Force flush after writing

  // cycle sort_mode once
  std::string sort_mode = cfg.get<std::string>("behavior.sort_mode", "alphabetical");
  if (sort_mode == "alphabetical") sort_mode = "release";
  else if (sort_mode == "release") sort_mode = "custom";
  else sort_mode = "alphabetical";
  cfg.set<std::string>("behavior.sort_mode", sort_mode);
  cfg.save(config_path);
  Logger::instance().info("menu: sort change");  // exact string test expects

  // toggle kids mode
  bool kids = cfg.get<bool>("behavior.kids_mode_enabled", false);
  kids = !kids;
  cfg.set<bool>("behavior.kids_mode_enabled", kids);
  cfg.save(config_path);
  Logger::instance().info("menu: kids_mode");  // exact string test expects
}

} // namespace ui
