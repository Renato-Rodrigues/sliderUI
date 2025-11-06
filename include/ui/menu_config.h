#pragma once
#ifndef SLIDERUI_UI_MENU_CONFIG_H
#define SLIDERUI_UI_MENU_CONFIG_H

#include "core/config_manager.h"
#include <string>

namespace ui {
namespace menu {

/**
 * MenuConfig - Runtime-configurable menu constants
 * 
 * Loads constants from sliderUI_cfg.json and allows hot-reloading without recompilation.
 * Provides getters for all menu UI constants that were previously compile-time constants.
 */
class MenuConfig {
public:
    // Initialize and load config from file
    static bool init(const std::string& config_path);
    
    // Reload config from file (useful for hot-reloading)
    static bool reload();
    
    // Check if hot reloading is enabled (reads from config)
    static bool is_hot_reload_enabled();
    
    // Reload config only if hot reload is enabled
    static void reload_if_enabled();
    
    // Get current config path
    static std::string get_config_path() { return config_path_; }
    
    // Screen dimensions
    static int SCREEN_WIDTH();
    static int SCREEN_HEIGHT();
    
    // Layout constants for menu UI
    static int TITLE_X();
    static int TITLE_Y();
    static int MENU_START_X();
    static int MENU_START_Y();
    static int MENU_ITEM_SPACING();
    static int MENU_VALUE_OFFSET_X();
    static int SELECTOR_HEIGHT();
    
    // Visual styling
    static int SELECTOR_HEIGHT_LIST();
    static int TEXT_HEIGHT();
    static int HELP_X();
    static int HELP_Y();
    
    // Game list constants
    static int LIST_START_X();
    static int LIST_START_Y();
    static int ITEM_HEIGHT();
    static int ARROW_PADDING();
    static int RIGHT_MARGIN();
    
    // Menu value alignment
    static int MENU_RIGHT_MARGIN();
    
    // Icons
    static int ICON_SIZE();
    static int ICON_RIGHT_MARGIN();
    
    // UI animation timings (ms)
    static int SELECTOR_MOVE_TIME();
    static int VALUE_CHANGE_TIME();

private:
    static core::ConfigManager cfg_;
    static std::string config_path_;
    static bool initialized_;
    static bool hot_reload_enabled_;
    static bool hot_reload_checked_;
    
    // Helper to get int value with fallback
    static int get_int(const std::string& key, int fallback);
};

} // namespace menu
} // namespace ui

#endif // SLIDERUI_UI_MENU_CONFIG_H

