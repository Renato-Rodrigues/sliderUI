#pragma once
#ifndef SLIDERUI_CORE_GAME_DB_H
#define SLIDERUI_CORE_GAME_DB_H

#include <string>
#include <vector>
#include <optional>
#include <cstddef>

namespace core {

/**
 * Game
 *
 * Lightweight POD describing a single game row from gameList.csv.
 *
 * Fields:
 *  - path: required path to the game file (as stored in CSV).
 *  - order: integer used for "custom" ordering (lower => earlier). -1 means unassigned.
 *  - name: optional display name (may be empty; UI can fallback to basename).
 *  - release_iso: optional release date in tolerant ISO-ish formats (YYYY, YYYY-MM, YYYY-MM-DD).
 *  - platform_id: inferred platform identifier (last folder name after trimming, without parentheses).
 *  - platform_core: optional string inside parentheses in the platform folder name (if present).
 */
struct Game {
  std::string path;
  int order = -1;
  std::string name;
  std::optional<std::string> release_iso;
  std::string platform_id;
  std::optional<std::string> platform_core; // new: content inside parentheses if present
};

/**
 * GameDB
 *
 * Responsible for parsing, holding, mutating, and persisting the game list.
 *
 * Invariants and design notes:
 *  - games_ (the internal vector) is the canonical, in-memory representation and
 *    its vector order represents the current display/custom order.
 *  - All mutating operations (move_up, move_down, remove, ensure_orders_assigned)
 *    modify the in-memory vector only. Call commit() to persist changes to disk.
 *  - commit() must perform an atomic write (temp + rename) to avoid partial writes.
 *  - move_up/move_down swap entries with neighbors and then normalize ordering
 *    (i.e., reindex contiguous 0..N-1) when commit() is called or when explicitly requested.
 *  - remove(index) removes the entry at index from the in-memory vector and returns true if successful.
 *  - Methods that accept an index assume 0 <= index < games().size(); callers should check bounds.
 *  - Game parsing should be robust: missing order -> order == -1 initially; use ensure_orders_assigned()
 *    to assign concrete integer orders (max_order+1).
 *
 * Thread-safety: not thread-safe. All calls must come from a single thread (UI thread).
 */
class GameDB {
public:
  GameDB() = default;

  /**
   * Load games from CSV at csv_path.
   * Returns true on success, false on I/O or parse error.
   * After load, games() is populated but orders may be unassigned (-1).
   */
  bool load(const std::string &csv_path);

  /**
   * Persist current in-memory games() to disk (atomic write).
   * Returns true on success, false on failure.
   * Should write CSV using same delimiter/format as read.
   */
  bool commit();

  /**
   * Read-only access to the in-memory games vector.
   * This vector order is canonical for display and custom ordering.
   */
  const std::vector<Game>& games() const noexcept;

  /**
   * Ensure every Game has a valid integer order.
   * Assigns missing orders as max_order + 1 for each missing entry (in current in-memory order).
   * Does not persist to disk; call commit() to save.
   */
  void ensure_orders_assigned();

  /**
   * Move the element at index up (swap with previous element).
   * If index == 0, does nothing.
   * Updates in-memory order; does NOT write to disk.
   */
  void move_up(std::size_t index);

  /**
   * Move the element at index down (swap with next element).
   * If index >= size-1, does nothing.
   * Updates in-memory order; does NOT write to disk.
   */
  void move_down(std::size_t index);

  /**
   * Remove the element at index from in-memory list.
   * Returns true if removed, false if index out of range.
   * Caller should call commit() to persist.
   */
  bool remove(std::size_t index);

  /**
   * Find the first game whose path equals the supplied path.
   * Returns index in games() if found, or size() (i.e., games().size()) if not found.
   */
  std::size_t find_by_path(const std::string &path) const noexcept;

private:
  std::string csv_path_;
  std::vector<Game> games_;

  /**
   * Re-normalize the order integers in games_ to contiguous values 0..N-1
   * following the current vector order. Called by implementations of move/remove
   * as needed (but not automatically persisted).
   */
  void normalize_orders();
};

} // namespace core

#endif // SLIDERUI_CORE_GAME_DB_H
