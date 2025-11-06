#include "core/game_db.h"
#include "core/csv_parser.h"
#include "core/file_utils.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cerrno>
#include <cctype>

using core::Game;
using core::GameDB;

namespace core {

// Helper: parse integer safely; returns optional<int>
static std::optional<int> parse_int_field(const std::string &s) {
  if (s.empty()) return std::nullopt;
  try {
    size_t idx = 0;
    int v = std::stoi(s, &idx);
    if (idx != s.size()) return std::nullopt;
    return v;
  } catch (...) {
    return std::nullopt;
  }
}

// trim helpers
static inline std::string trim_copy(const std::string &s) {
  size_t l = 0;
  while (l < s.size() && std::isspace(static_cast<unsigned char>(s[l]))) ++l;
  if (l == s.size()) return std::string();
  size_t r = s.size() - 1;
  while (r > l && std::isspace(static_cast<unsigned char>(s[r]))) --r;
  return s.substr(l, r - l + 1);
}

// remove parenthesis content and trim, e.g. "Name (core)" -> "Name"
static std::string remove_parenthesis_and_trim(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  bool in_paren = false;
  for (char c : s) {
    if (c == '(') { in_paren = true; continue; }
    if (c == ')') { in_paren = false; continue; }
    if (!in_paren) out.push_back(c);
  }
  return trim_copy(out);
}

// extract content inside first pair of parentheses (without parentheses), trimmed
static std::optional<std::string> extract_parenthesis_content(const std::string &s) {
  size_t open = s.find('(');
  if (open == std::string::npos) return std::nullopt;
  size_t close = s.find(')', open + 1);
  if (close == std::string::npos) return std::nullopt;
  std::string inner = s.substr(open + 1, close - (open + 1));
  inner = trim_copy(inner);
  if (inner.empty()) return std::nullopt;
  return inner;
}

// Given a path string, return the last folder name that contains the file.
// Example: "/mnt/SDCARD/platformName/game.rom" -> "platformName"
static std::string extract_last_folder_name(const std::string &path) {
  if (path.empty()) return std::string();
  std::string p = path;
  // remove trailing slashes
  while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();
  if (p.empty()) return std::string();
  // find last separator for file
  size_t last_sep = p.find_last_of("/\\");
  if (last_sep == std::string::npos) return std::string();
  // folder path is everything before last_sep
  std::string folder = p.substr(0, last_sep);
  // take last component of folder
  size_t last_folder_sep = folder.find_last_of("/\\");
  std::string candidate = (last_folder_sep == std::string::npos) ? folder : folder.substr(last_folder_sep + 1);
  return trim_copy(candidate);
}

bool GameDB::load(const std::string &csv_path) {
  csv_path_ = csv_path;
  CSVReader reader(';', true);
  if (!reader.load(csv_path)) {
    // treat as error: clear games_ and return false
    games_.clear();
    return false;
  }
  auto rows = reader.rows();
  games_.clear();
  if (rows.empty()) return true;

  // If first row looks like a header (contains "gamePath" or "path"), skip it.
  size_t start = 0;
  if (!rows.empty() && rows[0].size() > 0) {
    std::string h0 = rows[0][0];
    std::string low;
    low.resize(h0.size());
    std::transform(h0.begin(), h0.end(), low.begin(), [](unsigned char c){ return std::tolower(c); });
    if (low.find("path") != std::string::npos || low.find("gamepath") != std::string::npos) {
      start = 1;
    }
  }

  for (size_t r = start; r < rows.size(); ++r) {
    const auto &row = rows[r];
    // expected: gamePath; order; gameName; release
    if (row.size() < 1) continue; // skip empty row
    Game g;
    g.path = row.size() > 0 ? trim_copy(row[0]) : "";
    // order
    if (row.size() > 1) {
      auto maybe = parse_int_field(trim_copy(row[1]));
      g.order = maybe.has_value() ? *maybe : -1;
    } else {
      g.order = -1;
    }
    // name: remove parenthesis content and trim
    if (row.size() > 2) g.name = remove_parenthesis_and_trim(row[2]);
    else g.name.clear();
    // release
    if (row.size() > 3 && !trim_copy(row[3]).empty()) g.release_iso = trim_copy(row[3]);
    else g.release_iso = std::nullopt;

    // platform_id and platform_core extraction
    std::string folder_name = extract_last_folder_name(g.path); // e.g. "PlatformName (core)"
    g.platform_id = remove_parenthesis_and_trim(folder_name);
    g.platform_core = extract_parenthesis_content(folder_name);

    games_.push_back(std::move(g));
  }

  return true;
}

const std::vector<Game>& GameDB::games() const noexcept {
  return games_;
}

void GameDB::ensure_orders_assigned() {
  int max_order = -1;
  for (const auto &g : games_) {
    if (g.order >= 0 && g.order > max_order) max_order = g.order;
  }
  // Assign missing orders in current in-memory order
  for (auto &g : games_) {
    if (g.order < 0) {
      ++max_order;
      g.order = max_order;
    }
  }
  // Normalize to contiguous 0..N-1 according to current vector order
  normalize_orders();
}

void GameDB::normalize_orders() {
  for (size_t i = 0; i < games_.size(); ++i) {
    games_[i].order = static_cast<int>(i);
  }
}

void GameDB::move_up(std::size_t index) {
  if (index == 0 || index >= games_.size()) return;
  std::swap(games_[index], games_[index - 1]);
  normalize_orders();
}

void GameDB::move_down(std::size_t index) {
  if (index >= games_.size() || index + 1 >= games_.size()) return;
  std::swap(games_[index], games_[index + 1]);
  normalize_orders();
}

bool GameDB::remove(std::size_t index) {
  if (index >= games_.size()) return false;
  games_.erase(games_.begin() + index);
  normalize_orders();
  return true;
}

std::size_t GameDB::find_by_path(const std::string &path) const noexcept {
  for (size_t i = 0; i < games_.size(); ++i) {
    if (games_[i].path == path) return i;
  }
  return games_.size();
}

bool GameDB::commit() {
  if (csv_path_.empty()) return false;
  // build rows: header + rows
  std::vector<std::vector<std::string>> rows;
  rows.push_back(std::vector<std::string>{"gamePath", "order", "gameName", "release"});
  for (const auto &g : games_) {
    std::vector<std::string> row;
    row.push_back(g.path);
    row.push_back(std::to_string(g.order));
    row.push_back(g.name);
    row.push_back(g.release_iso.has_value() ? *g.release_iso : std::string());
    rows.push_back(std::move(row));
  }
  CSVReader writer(';', true);
  return writer.save(csv_path_, rows);
}

} // namespace core
