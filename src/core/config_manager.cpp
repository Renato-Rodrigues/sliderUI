#include "core/config_manager.h"
#include "core/file_utils.h"

#include <fstream>
#include <sstream>
#include <iostream>

using core::ConfigManager;
using json = nlohmann::json;

namespace {

// Build default config
json make_defaults() {
  return json{
    {"version", 1},
    {"ui", {
      {"resolution", {640,480}},
      {"background", "bckg.png"},
      // Game title at top
      {"title", {
        {"x", 20},
        {"y", 28},
        {"size", 26},
        {"font", "default"},
        {"color", "#FFFFFF"},
        {"align", "left"}
      }},
      // Release year info
      {"release", {
        {"x", 20},
        {"y", 56},
        {"size", 14},
        {"font", "default"},
        {"color", "#CCCCCC"},
        {"align", "left"}
      }},
      // Platform info with icon
      {"platform", {
        {"x", 20},
        {"y", 84},
        {"size", 14},
        {"font", "default"},
        {"color", "#CCCCCC"},
        {"icon_size", 24},
        {"icon_margin", 8},
        {"align", "left"}
      }},
      // Game carousel
      {"game_image", {
        {"x", 320},              // Center image X
        {"y", 160},              // Center image Y
        {"width", 200},          // Image width
        {"height", 270},         // Image height
        {"margin", 28},          // Space between images
        {"scale", "fit"},        // Scaling mode: fit/fill
        {"side_scale", 0.78}      // Size multiplier for side images
      }},
      // Selected game highlight
      {"selected_contour", {
        {"stroke", 3},
        {"radius", 8},
        {"color", "#FFFFFF"},
        {"glow", true},
        {"glow_color", "#336699"}, 
        {"glow_alpha", 160}
      }},
      // Bottom control hints
      {"buttons", {
        {"x", 620},             // Right-aligned
        {"y", 440},             // Bottom position
        {"size", 14},           // Font size
        {"font", "default"},
        {"color", "#FFFFFF"},
        {"icon_size", 24},      // Button icon size
        {"icon_margin", 8},     // Space between icon and text
        {"background", "#000000AA"}, // Semi-transparent background
        {"padding", 8},         // Padding around text
        {"align", "right"}      // Right alignment
      }},
      // Loading spinner
      {"loading", {
        {"x", 320},            // Center of screen
        {"y", 240},
        {"size", 32},
        {"color", "#FFFFFF"}
      }}
    }},
    {"behavior", {
      {"sort_mode", "alphabetical"},
      {"start_game", "last_played"},
      {"kids_mode_enabled", false},
      {"exit_mode", "default"},
      {"confirm_delete_timeout_ms", 3000},
      {"release_order", "ascending"}
    }},
    {"platform", {
      {"icons_path", ""},
      {"image_formats", {"png","jpg","webp"}},
      {"image_max_dimensions", {640,480}}
    }},
    {"logging", {
      {"enabled", true},
      {"dir", "logs/"},
      {"max_files", 10}
    }}
  };
}

// Recursive merge: overlay `user` onto `target`.
// Rules: if both are objects => recurse; else target = user (override).
void merge_into_defaults(const json &user, json &target) {
  if (!user.is_object() || !target.is_object()) {
    // target replaced by user if user is not object: handled by caller
    return;
  }
  for (auto it = user.begin(); it != user.end(); ++it) {
    const std::string key = it.key();
    const json &uval = it.value();
    if (target.contains(key) && target[key].is_object() && uval.is_object()) {
      merge_into_defaults(uval, target[key]);
    } else {
      // override or set
      target[key] = uval;
    }
  }
}

} // namespace

const json& ConfigManager::defaults() {
  static json d = make_defaults();
  return d;
}

bool ConfigManager::validate_and_patch() {
  // Start with defaults, then overlay the user's loaded cfg_ into it recursively.
  json patched = defaults();
  if (cfg_.is_object()) {
    merge_into_defaults(cfg_, patched);
  } else {
    // If loaded cfg_ is not an object (corrupted), keep defaults
  }
  cfg_ = std::move(patched);
  return true;
}

bool ConfigManager::load(const std::string &path) {
  cfg_path_ = path;
  std::ifstream in(path);
  if (!in.good()) {
    // Missing file — use defaults, success.
    cfg_ = defaults();
    return true;
  }

  try {
    in >> cfg_;
  } catch (const std::exception &e) {
    // Corrupted JSON: fallback to defaults and return false to signal issue.
    cfg_ = defaults();
    return false;
  }

  validate_and_patch();
  return true;
}

bool ConfigManager::save(const std::string &path) const {
  try {
    std::string out = cfg_.dump(2);
    return file_utils::atomic_write(path, out);
  } catch (...) {
    return false;
  }
}
