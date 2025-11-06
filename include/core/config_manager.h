#pragma once
#ifndef SLIDERUI_CORE_CONFIG_MANAGER_H
#define SLIDERUI_CORE_CONFIG_MANAGER_H

#include <string>
#include <nlohmann/json.hpp>

namespace core {

using json = nlohmann::json;

/**
 * ConfigManager
 *
 * Loads and saves a JSON configuration file, merging with built-in defaults.
 * Provides typed getters/setters via dotted-key notation (e.g. "ui.game_image.width").
 *
 * Template get/set implemented inline in this header.
 */
class ConfigManager {
public:
  ConfigManager() = default;

  // Load configuration from `path`. If file missing -> populate defaults and return true.
  // If file present but corrupted -> fallback to defaults and return false.
  bool load(const std::string &path);

  // Save current configuration to `path`. Should use atomic write semantics.
  bool save(const std::string &path) const;

  // Get a typed value using dotted key (e.g. "ui.game_image.width").
  // Returns fallback if key missing or type mismatch.
  template<typename T>
  T get(const std::string &key, const T &fallback) const;

  // Set a value using dotted key notation. Creates intermediate objects as needed.
  template<typename T>
  void set(const std::string &key, const T &value);

  // Set default path used by this manager (optional).
  void set_path(const std::string &path) { cfg_path_ = path; }

  // Access raw JSON
  const json& raw() const noexcept { return cfg_; }

private:
  json cfg_;               // in-memory config
  std::string cfg_path_;   // last loaded/saved path

  // Defaults provider and validator/patcher (implemented in .cpp)
  static const json& defaults();
  bool validate_and_patch();

  // Helpers for dotted-key traversal (inline for template usage)
  static const json* json_get_ptr_const(const json &root, const std::string &dotted) {
    const json *cur = &root;
    size_t pos = 0;
    std::string s = dotted;
    while ((pos = s.find('.')) != std::string::npos) {
      std::string token = s.substr(0,pos);
      if (!cur->contains(token)) return nullptr;
      cur = &((*cur)[token]);
      s.erase(0,pos+1);
    }
    if (s.empty()) return cur;
    if (!cur->contains(s)) return nullptr;
    return &((*cur)[s]);
  }

  static json* json_get_ptr(json &root, const std::string &dotted) {
    json *cur = &root;
    size_t pos = 0;
    std::string s = dotted;
    while ((pos = s.find('.')) != std::string::npos) {
      std::string token = s.substr(0,pos);
      if (!cur->contains(token) || !(*cur)[token].is_object()) {
        (*cur)[token] = json::object();
      }
      cur = &((*cur)[token]);
      s.erase(0,pos+1);
    }
    if (s.empty()) return cur;
    if (!cur->contains(s)) {
      (*cur)[s] = nullptr;
    }
    return &((*cur)[s]);
  }
};

// Inline template implementations

template<typename T>
T ConfigManager::get(const std::string &key, const T &fallback) const {
  const json *node = json_get_ptr_const(cfg_, key);
  if (!node) return fallback;
  try {
    return node->get<T>();
  } catch (...) {
    return fallback;
  }
}

template<typename T>
void ConfigManager::set(const std::string &key, const T &value) {
  json *node = json_get_ptr(cfg_, key);
  *node = value;
}

} // namespace core

#endif // SLIDERUI_CORE_CONFIG_MANAGER_H
