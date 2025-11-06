#include "ui/games_list.h"
#include "ui/renderer.h"
#include "ui/menu_constants.h"
#include "ui/menu_config.h"
#include "core/logger.h"

#include <algorithm>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using core::Logger;
using namespace ui::menu;

namespace ui {

bool show_games_list(Renderer& renderer, GameListState& state) {
    // Initialize menu config if not already done
    MenuConfig::init("sliderUI_cfg.json");
    
    bool running = true;
    bool game_selected = false;
    const auto& games = state.game_db.games();

    while (running) {
        // Reload config if hot-reloading is enabled
        MenuConfig::reload_if_enabled();
        // Handle input
        Input in = poll_input();
        switch (in) {
            case Input::UP:
                if (state.selected_index > 0) {
                    state.selected_index--;
                    // Adjust scroll if needed
                    if (state.selected_index < state.scroll_offset) {
                        state.scroll_offset = state.selected_index;
                    }
                }
                break;
            case Input::DOWN:
                if (state.selected_index < games.size() - 1) {
                    state.selected_index++;
                    // Adjust scroll if needed
                    if (state.selected_index >= state.scroll_offset + GameListState::VISIBLE_ITEMS) {
                        state.scroll_offset = state.selected_index - GameListState::VISIBLE_ITEMS + 1;
                    }
                }
                break;
            case Input::Y: {
                // Remove game from list
                if (!games.empty()) {
                    const auto& game = games[state.selected_index];
                    Logger::instance().info("Removing game: " + game.name);
                    if (const_cast<core::GameDB&>(state.game_db).remove(state.selected_index)) {
                        if (const_cast<core::GameDB&>(state.game_db).commit()) {
                            // Adjust indices if needed
                            if (state.selected_index >= games.size()) {
                                state.selected_index = games.empty() ? 0 : games.size() - 1;
                            }
                            if (state.scroll_offset > 0 && state.scroll_offset >= games.size()) {
                                state.scroll_offset = games.empty() ? 0 : games.size() - 1;
                            }
                        } else {
                            Logger::instance().error("Failed to commit game removal");
                        }
                    }
                }
                break;
            }
            case Input::A:
                if (!games.empty()) {
                    game_selected = true;
                    running = false;
                }
                break;
            case Input::B:
                running = false;
                break;
            default:
                break;
        }

        // Render
        renderer.clear();

        // Draw title
        renderer.draw_text(TITLE_X(), TITLE_Y(), "sliderUI game list", false);

        // Draw scroll arrows if needed
        if (state.scroll_offset > 0) {
            renderer.draw_text(SCREEN_WIDTH()/2, LIST_START_Y() - ARROW_PADDING(), "^", false);
        }
        if (state.scroll_offset + GameListState::VISIBLE_ITEMS < games.size()) {
            renderer.draw_text(SCREEN_WIDTH()/2, LIST_START_Y() + GameListState::VISIBLE_ITEMS * ITEM_HEIGHT(), "v", false);
        }

        // Draw visible items
        for (size_t i = 0; i < GameListState::VISIBLE_ITEMS && i + state.scroll_offset < games.size(); i++) {
            const int y = LIST_START_Y() + i * ITEM_HEIGHT();
            const size_t game_idx = i + state.scroll_offset;
            const bool highlight = (game_idx == state.selected_index);
            const auto& game = games[game_idx];

            // Calculate selector position - position below text
            const int selector_y = y + (SELECTOR_HEIGHT_LIST() - TEXT_HEIGHT()) / 2;
            
            if (highlight) {
                renderer.draw_selector(LIST_START_X() - 10, selector_y, SCREEN_WIDTH() - LIST_START_X() * 2 + 20, SELECTOR_HEIGHT_LIST());
            }

            // Use game name if available, otherwise use path basename
            std::string display_name = game.name;
            if (display_name.empty()) {
                size_t last_slash = game.path.find_last_of("/\\");
                size_t last_dot = game.path.find_last_of(".");
                if (last_slash == std::string::npos) {
                    last_slash = 0;
                } else {
                    last_slash++;
                }
                if (last_dot == std::string::npos) {
                    last_dot = game.path.length();
                }
                display_name = game.path.substr(last_slash, last_dot - last_slash);
            }

            renderer.draw_text(LIST_START_X(), y, display_name, highlight);

            // Draw platform info if available (right-aligned with same margin as left)
            if (!game.platform_id.empty()) {
                std::string platform_text = game.platform_id;
                if (game.platform_core) {
                    platform_text += " (" + *game.platform_core + ")";
                }
                // Calculate right-aligned position - measure actual width
                const int platform_width = renderer.get_text_width(platform_text);
                const int platform_x = SCREEN_WIDTH() - RIGHT_MARGIN() - platform_width;  // Right edge at margin
                renderer.draw_text(platform_x, y, platform_text, highlight);
            }
        }

        // Draw help text
        renderer.draw_text(HELP_X(), HELP_Y(), "B BACK     A SELECT     Y REMOVE", false);

        renderer.present();

        // Frame pacing
        std::this_thread::sleep_for(16ms);
    }
    return game_selected;
}

} // namespace ui