#pragma once
#ifndef SLIDERUI_CORE_SORT_H
#define SLIDERUI_CORE_SORT_H

#include <vector>
#include "core/game_db.h"

namespace core {

enum class SortMode { ALPHA, RELEASE, CUSTOM };

/**
 * Sort games according to mode. Uses stable sort.
 * - ALPHA: case-insensitive compare of display name (name if present, else basename of path).
 * - RELEASE: parse release_iso (YYYY[-MM[-DD]]). Valid dates sort before unknown.
 *            Order (ascending/descending) is oldest-first (ascending) or newest-first (descending).
 *            Default behavior is ascending unless config specifies "descending".
 * - CUSTOM: sort by game.order ascending (lower -> earlier).
 *
 * Overloads:
 *   sort_games(g, mode)                -> uses default behavior (ascending for release).
 *   sort_games(g, mode, cfg)           -> if cfg != nullptr, reads behavior.release_order.
 */
void sort_games(std::vector<Game> &g, SortMode mode);

/**
 * Overload that consults ConfigManager for behavior.release_order ("ascending" or "descending").
 * Pass nullptr to use default (ascending).
 */
class ConfigManager; // forward-declare to avoid include in header
void sort_games(std::vector<Game> &g, SortMode mode, const ConfigManager *cfg);

} // namespace core

#endif // SLIDERUI_CORE_SORT_H
