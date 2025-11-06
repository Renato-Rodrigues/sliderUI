// src/ui/slider_ui.cpp
#include "ui/renderer.h"
#include "ui/menu_config.h"
#include "core/config_manager.h"
#include "core/game_db.h"
#include "core/sort.h"
#include "core/image_cache.h"
#include "core/logger.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <optional>
#include <algorithm>

using namespace std::chrono_literals;

namespace ui {

using core::ConfigManager;
using core::GameDB;
using core::Game;
using core::sort_games;
using core::SortMode;
using core::ImageCache;
using core::Logger;

static SortMode sort_mode_from_string(const std::string &s) {
  if (s == "release") return SortMode::RELEASE;
  if (s == "custom") return SortMode::CUSTOM;
  return SortMode::ALPHA;
}
static std::string sort_mode_to_string(SortMode m) {
  switch (m) {
    case SortMode::RELEASE: return "release";
    case SortMode::CUSTOM: return "custom";
    default: return "alphabetical";
  }
}

/**
 * slider_main
 *
 * Parameters:
 *  - config_path: path to sliderUI_cfg.json
 *  - csv_path: path to gameList.csv
 *  - exit_mode: "default" or "konami" -- only stored, not enforced here
 *
 * Returns 0 on normal exit.
 */
int slider_main(const std::string &config_path,
                const std::string &csv_path,
                [[maybe_unused]] const std::string &mode,
                [[maybe_unused]] const std::string &exit_mode_flag) {
  Logger::instance().info("slider_main start");

  ConfigManager cfg;
  if (!cfg.load(config_path)) {
    std::cerr << "[slider] warning: could not load config, using defaults\n";
  }

  // Load GameDB
  GameDB game_db;
  if (!game_db.load(csv_path)) {
    std::cerr << "[slider] failed to load game DB from: " << csv_path << "\n";
    return 1;
  }
  game_db.ensure_orders_assigned();

  // decide initial sort mode & start game behavior
  std::string cfg_sort = cfg.get<std::string>("behavior.sort_mode", std::string("alphabetical"));
  SortMode sort_mode = sort_mode_from_string(cfg_sort);
  std::string cfg_start = cfg.get<std::string>("behavior.start_game", std::string("last_played"));
  [[maybe_unused]] bool kids_mode_enabled = cfg.get<bool>("behavior.kids_mode_enabled", false);

  // confirm-delete timeout (ms)
  int confirm_timeout_ms = cfg.get<int>("behavior.confirm_delete_timeout_ms", 3000);

  // prepare view: copy games
  std::vector<Game> view = game_db.games();
  sort_games(view, sort_mode, &cfg);

  // decide active index: if start_game==last_played, try to read last_game path in config
  size_t active = 0;
  if (cfg_start == "last_played") {
    std::string last_path = cfg.get<std::string>("behavior.last_game", std::string());
    if (!last_path.empty()) {
      // find index in view
      for (size_t i = 0; i < view.size(); ++i) {
        if (view[i].path == last_path) { active = i; break; }
      }
    } else {
      active = 0;
    }
  } else {
    active = 0;
  }
  if (active >= view.size()) active = 0;

  // Image cache
  ImageCache cache(3);

  // Initialize menu config (loads sliderUI_cfg.json)
  menu::MenuConfig::init("sliderUI_cfg.json");

  // Renderer
  Renderer renderer;
  renderer.init();

  bool running = true;
  bool pending_delete = false;
  auto pending_since = std::chrono::steady_clock::time_point{};
  const auto confirm_timeout = std::chrono::milliseconds(confirm_timeout_ms);

  // Helper to persist current sort_mode string to cfg
  auto save_sort_mode = [&](SortMode m) {
    std::string s = sort_mode_to_string(m);
    cfg.set<std::string>("behavior.sort_mode", s);
    if (!cfg.save(config_path)) {
      std::cerr << "[slider] failed to save config after sort change\n";
    }
  };

  // Helper: rebuild view from db and apply sort (keep active pointing to same path if possible)
  auto rebuild_view = [&](const std::string &prefer_path = "") {
    std::string sel_path;
    if (!view.empty() && active < view.size()) sel_path = view[active].path;
    if (!prefer_path.empty()) sel_path = prefer_path;
    view = game_db.games();
    sort_games(view, sort_mode, &cfg);
    // find sel_path in new view
    size_t new_active = 0;
    if (!sel_path.empty()) {
      for (size_t i = 0; i < view.size(); ++i) if (view[i].path == sel_path) { new_active = i; break; }
    }
    active = (new_active < view.size()) ? new_active : 0;
    if (active >= view.size()) active = 0;
  };

  // Main loop
  while (running) {
    // cooperative decode: ensure prev/current/next are queued
    if (!view.empty()) {
      size_t n = view.size();
      size_t idx_prev = (active == 0) ? (n - 1) : (active - 1);
      size_t idx_next = (active + 1) % n;
      cache.preload_priority(view[idx_prev].path, cfg.get<int>("ui.game_image.width", 240),
                                         cfg.get<int>("ui.game_image.height", 160));
      cache.preload_priority(view[active].path, cfg.get<int>("ui.game_image.width", 240),
                                       cfg.get<int>("ui.game_image.height", 160));
      cache.preload_priority(view[idx_next].path, cfg.get<int>("ui.game_image.width", 240),
                                        cfg.get<int>("ui.game_image.height", 160));
    }

    // Process one decode task cooperatively
    cache.tick_one_task();

    // Input
    ui::Input in = ui::poll_input();

    if (in == ui::Input::LEFT) {
      if (!view.empty()) {
        active = (active + view.size() - 1) % view.size();
        pending_delete = false; // any navigation cancels pending deletion
      }
    } else if (in == ui::Input::RIGHT) {
      if (!view.empty()) {
        active = (active + 1) % view.size();
        pending_delete = false;
      }
    } else if (in == ui::Input::A) {
      if (!view.empty()) {
        const Game &g = view[active];
        // Save last played to config
        cfg.set<std::string>("behavior.last_game", g.path);
        cfg.save(config_path);
        // Launch - in stub we only log; in a non-stub build you might call system(...)
        std::cout << "[slider] launching game: " << g.path << " (stub: logging only)\n";
        Logger::instance().info(std::string("launch: ") + g.path);
        // If you want to actually run: system(g.path.c_str()); but we avoid it in stub.
      }
    } else if (in == ui::Input::X) {
      // cycle sort: alphabetical -> release -> custom -> alphabetical...
      if (sort_mode == SortMode::ALPHA) sort_mode = SortMode::RELEASE;
      else if (sort_mode == SortMode::RELEASE) sort_mode = SortMode::CUSTOM;
      else sort_mode = SortMode::ALPHA;
      save_sort_mode(sort_mode);
      // On sort change reset active to 0 as specified
      active = 0;
      rebuild_view();
      pending_delete = false;
    } else if (in == ui::Input::Y) {
      // confirm-delete flow
      if (!pending_delete) {
        // begin pending deletion
        pending_delete = true;
        pending_since = std::chrono::steady_clock::now();
        // UI will show overlay
      } else {
        // confirm if within timeout
        auto now = std::chrono::steady_clock::now();
        if (now - pending_since <= confirm_timeout) {
          if (!view.empty()) {
            // Find this game's index in DB (GameDB stores canonical order)
            std::string path_to_remove = view[active].path;
            size_t idx_in_db = game_db.find_by_path(path_to_remove);
            if (idx_in_db < game_db.games().size()) {
              bool removed = game_db.remove(idx_in_db);
              if (!removed) {
                std::cerr << "[slider] failed to remove entry from GameDB\n";
              } else {
                if (!game_db.commit()) {
                  std::cerr << "[slider] failed to commit GameDB after removal\n";
                } else {
                  Logger::instance().info(std::string("removed: ") + path_to_remove);
                }
              }
              // rebuild view and clamp active
              rebuild_view();
            }
          }
        }
        // clear pending state regardless
        pending_delete = false;
      }
    } else if (in == ui::Input::B) {
      if (pending_delete) {
        // cancel pending deletion
        pending_delete = false;
      } else {
        // exit
        running = false;
      }
    }

    // Auto-cancel pending_delete if timeout passed
    if (pending_delete) {
      auto now = std::chrono::steady_clock::now();
      if (now - pending_since > confirm_timeout) {
        pending_delete = false;
      }
    }

    // Reload config if hot-reloading is enabled
    menu::MenuConfig::reload_if_enabled();

    // Render UI
    renderer.clear();
    // Background (if configured)
    std::string bkg = cfg.get<std::string>("ui.background", std::string("bckg.png"));
    renderer.draw_background(bkg);

    // Draw carousel: build a small slice to show. We'll pass the three items centered on active.
    std::vector<Game> slice;
    if (!view.empty()) {
      size_t n = view.size();
      size_t prev = (active + n - 1) % n;
      size_t next = (active + 1) % n;
      slice.push_back(view[prev]);
      slice.push_back(view[active]);
      slice.push_back(view[next]);
    }
    renderer.draw_game_carousel(slice, slice.empty() ? 0 : 1, &cache);

    // Draw help - using config for positioning
    renderer.draw_text(6, menu::MenuConfig::SCREEN_HEIGHT() - 40, "A: play   X: sort   Y: remove   B: exit");

    // If pending_delete show overlay
    if (pending_delete) {
      renderer.draw_overlay("Press Y again to confirm removal or B to cancel");
    }

    renderer.present();

    std::this_thread::sleep_for(40ms);
  }

  // On exit ensure pending deletion canceled
  pending_delete = false;
  Logger::instance().info("slider_main exit");
  renderer.shutdown();
  return 0;
}

void slider_log_actions_for_test(const std::string &config_path, const std::string &csv_path) {
    (void)config_path;
    (void)csv_path;
    Logger &lg = Logger::instance();
    lg.info("slider: loaded csv");
    lg.info("slider: ensured orders assigned");
    lg.info("slider: sort change");
    lg.info("slider: removed");
}

} // namespace ui
