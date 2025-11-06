#include "ui/menu_config.h"
#include "core/logger.h"
#include <iostream>

namespace ui {
namespace menu {

core::ConfigManager MenuConfig::cfg_;
std::string MenuConfig::config_path_ = "sliderUI_cfg.json";
bool MenuConfig::initialized_ = false;
bool MenuConfig::hot_reload_enabled_ = false;
bool MenuConfig::hot_reload_checked_ = false;

bool MenuConfig::init(const std::string& config_path) {
    config_path_ = config_path;
    return reload();
}

bool MenuConfig::reload() {
    if (!cfg_.load(config_path_)) {
        core::Logger::instance().error("Failed to load menu config: " + config_path_);
        return false;
    }
    initialized_ = true;
    // Re-check hot reload setting after reload
    hot_reload_checked_ = false;
    return true;
}

bool MenuConfig::is_hot_reload_enabled() {
    if (!initialized_) {
        // Auto-initialize if not already done
        if (!init()) {
            return false;  // Default to disabled if can't load config
        }
    }
    // Cache the value to avoid repeated file I/O
    if (!hot_reload_checked_) {
        hot_reload_enabled_ = cfg_.get<bool>("hot_reload", false);  // Default to false for performance
        hot_reload_checked_ = true;
    }
    return hot_reload_enabled_;
}

void MenuConfig::reload_if_enabled() {
    if (is_hot_reload_enabled()) {
        reload();
    }
}

int MenuConfig::get_int(const std::string& key, int fallback) {
    if (!initialized_) {
        // Auto-initialize if not already done
        if (!init()) {
            return fallback;
        }
    }
    return cfg_.get<int>(key, fallback);
}

// Screen dimensions
int MenuConfig::SCREEN_WIDTH() {
    return get_int("screen.width", 640);
}

int MenuConfig::SCREEN_HEIGHT() {
    return get_int("screen.height", 480);
}

// Layout constants for menu UI
int MenuConfig::TITLE_X() {
    return get_int("menu.title_x", 60);
}

int MenuConfig::TITLE_Y() {
    return get_int("menu.title_y", 30);
}

int MenuConfig::MENU_START_X() {
    return get_int("menu.start_x", 60);
}

int MenuConfig::MENU_START_Y() {
    return get_int("menu.start_y", 100);
}

int MenuConfig::MENU_ITEM_SPACING() {
    return get_int("menu.item_spacing", 50);
}

int MenuConfig::MENU_VALUE_OFFSET_X() {
    return get_int("menu.value_offset_x", 550);
}

int MenuConfig::SELECTOR_HEIGHT() {
    return get_int("menu.selector_height", 40);
}

// Visual styling
int MenuConfig::SELECTOR_HEIGHT_LIST() {
    return get_int("menu.selector_height_list", 30);
}

int MenuConfig::TEXT_HEIGHT() {
    return get_int("menu.text_height", 22);
}

int MenuConfig::HELP_X() {
    return get_int("menu.help_x", 60);
}

int MenuConfig::HELP_Y() {
    return get_int("menu.help_y", 420);
}

// Game list constants
int MenuConfig::LIST_START_X() {
    return get_int("game_list.start_x", 40);
}

int MenuConfig::LIST_START_Y() {
    return get_int("game_list.start_y", 90);
}

int MenuConfig::ITEM_HEIGHT() {
    return get_int("game_list.item_height", 40);
}

int MenuConfig::ARROW_PADDING() {
    return get_int("game_list.arrow_padding", 20);
}

int MenuConfig::RIGHT_MARGIN() {
    return LIST_START_X();  // Same as left margin
}

// Menu value alignment
int MenuConfig::MENU_RIGHT_MARGIN() {
    return MENU_START_X();  // Same as left margin
}

// Icons
int MenuConfig::ICON_SIZE() {
    return get_int("menu.icon_size", 32);
}

int MenuConfig::ICON_RIGHT_MARGIN() {
    return get_int("menu.icon_right_margin", 20);
}

// UI animation timings (ms)
int MenuConfig::SELECTOR_MOVE_TIME() {
    return get_int("menu.selector_move_time", 150);
}

int MenuConfig::VALUE_CHANGE_TIME() {
    return get_int("menu.value_change_time", 200);
}

} // namespace menu
} // namespace ui

