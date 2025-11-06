#pragma once
#ifndef SLIDERUI_UI_GAMES_LIST_H
#define SLIDERUI_UI_GAMES_LIST_H

#include <vector>
#include <string>
#include "core/game_db.h"
#include "ui/renderer.h"

namespace ui {

struct GameListState {
    const core::GameDB& game_db;  // Reference to the game database
    size_t selected_index = 0;
    size_t scroll_offset = 0;
    static constexpr size_t VISIBLE_ITEMS = 7;  // Number of visible items in the list

    // Constructor requires GameDB reference
    explicit GameListState(const core::GameDB& db) : game_db(db) {}
};

/**
 * Show the games list menu with the specified design.
 * Returns true if a game was selected, false if cancelled.
 */
bool show_games_list(Renderer& renderer, GameListState& state);

} // namespace ui

#endif // SLIDERUI_UI_GAMES_LIST_H