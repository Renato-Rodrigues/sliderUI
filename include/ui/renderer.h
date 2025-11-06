// include/ui/renderer.h
#pragma once

#include <string>
#include <vector>

namespace core { struct Game; }
namespace core { class ImageCache; }
namespace core { class ConfigManager; }

namespace ui {

// Simple input enumeration used across UI code. This mirrors the project's expected values.
enum class Input {
    NONE = 0,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    A,
    B,
    Y,
    X,
    MENU,
    START,
    SELECT
};

// Poll input - forward declaration so menu_ui.cpp can call it.
// The real implementation already exists in your codebase (this declaration just makes it visible).
Input poll_input();

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init();
    void shutdown();

    // Frame lifecycle
    void clear();
    void present();

    // Drawing helpers
    void draw_background(const std::string &background_image);
    void draw_game_carousel(const std::vector<core::Game> &games, size_t active_index, core::ImageCache *cache);
    void draw_text(int x, int y, const std::string &s, bool highlight = false);
    void draw_overlay(const std::string &message);

    // Methods required by menu_ui.cpp
    void draw_selector(int x, int y, int w, int h);
    int get_text_width(const std::string &s);

    // Sprite-layer mode API
    void set_sprite_layer_mode(bool enabled);

    // Configuration support - allows renderer to read aesthetics from config
    void set_config(core::ConfigManager *cfg);

private:
    struct Impl;
    Impl *pimpl;
    core::ConfigManager *config_ = nullptr;
};

} // namespace ui