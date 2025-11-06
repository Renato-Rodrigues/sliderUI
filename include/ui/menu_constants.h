#pragma once
#ifndef SLIDERUI_UI_MENU_CONSTANTS_H
#define SLIDERUI_UI_MENU_CONSTANTS_H

#include "ui/menu_config.h"

namespace ui {
namespace menu {

// Screen dimensions - now loaded from sliderUI_cfg.json at runtime
inline int SCREEN_WIDTH() { return MenuConfig::SCREEN_WIDTH(); }
inline int SCREEN_HEIGHT() { return MenuConfig::SCREEN_HEIGHT(); }

// Layout constants for menu UI - now loaded from sliderUI_cfg.json at runtime
inline int TITLE_X() { return MenuConfig::TITLE_X(); }
inline int TITLE_Y() { return MenuConfig::TITLE_Y(); }
inline int MENU_START_X() { return MenuConfig::MENU_START_X(); }
inline int MENU_START_Y() { return MenuConfig::MENU_START_Y(); }
inline int MENU_ITEM_SPACING() { return MenuConfig::MENU_ITEM_SPACING(); }
inline int MENU_VALUE_OFFSET_X() { return MenuConfig::MENU_VALUE_OFFSET_X(); }
inline int SELECTOR_HEIGHT() { return MenuConfig::SELECTOR_HEIGHT(); }

// Visual styling - now loaded from sliderUI_cfg.json at runtime
inline int SELECTOR_HEIGHT_LIST() { return MenuConfig::SELECTOR_HEIGHT_LIST(); }
inline int TEXT_HEIGHT() { return MenuConfig::TEXT_HEIGHT(); }
inline int HELP_X() { return MenuConfig::HELP_X(); }
inline int HELP_Y() { return MenuConfig::HELP_Y(); }

// Game list constants - now loaded from sliderUI_cfg.json at runtime
inline int LIST_START_X() { return MenuConfig::LIST_START_X(); }
inline int LIST_START_Y() { return MenuConfig::LIST_START_Y(); }
inline int ITEM_HEIGHT() { return MenuConfig::ITEM_HEIGHT(); }
inline int ARROW_PADDING() { return MenuConfig::ARROW_PADDING(); }
inline int RIGHT_MARGIN() { return MenuConfig::RIGHT_MARGIN(); }

// Menu value alignment - now loaded from sliderUI_cfg.json at runtime
inline int MENU_RIGHT_MARGIN() { return MenuConfig::MENU_RIGHT_MARGIN(); }

// Icons - now loaded from sliderUI_cfg.json at runtime
inline int ICON_SIZE() { return MenuConfig::ICON_SIZE(); }
inline int ICON_RIGHT_MARGIN() { return MenuConfig::ICON_RIGHT_MARGIN(); }

// UI animation timings (ms) - now loaded from sliderUI_cfg.json at runtime
inline int SELECTOR_MOVE_TIME() { return MenuConfig::SELECTOR_MOVE_TIME(); }
inline int VALUE_CHANGE_TIME() { return MenuConfig::VALUE_CHANGE_TIME(); }

} // namespace menu
} // namespace ui

#endif // SLIDERUI_UI_MENU_CONSTANTS_H