// src/ui/renderer_minui.cpp
#include "ui/renderer.h"
#include "core/logger.h"
#include "core/game_db.h"
#include "core/image_cache.h"
#include <string>
#include <vector>

using namespace ui;
using core::Game;
using core::ImageCache;
using core::Logger;

struct ui::Renderer::Impl {
    int dummy = 0;
};

Renderer::Renderer() : pimpl(new Impl()) {}
Renderer::~Renderer() { shutdown(); delete pimpl; }

bool Renderer::init() { return true; }
void Renderer::shutdown() {}
void Renderer::clear() {}
void Renderer::present() {}

void Renderer::draw_background(const std::string &background_image) {
    Logger::instance().info(std::string("[minui] draw_background ") + background_image);
}

void Renderer::draw_text(int x, int y, const std::string &s, bool highlight) {
    (void)x; (void)y; (void)highlight;
    Logger::instance().info(std::string("[minui text] ") + s);
}

void Renderer::draw_overlay(const std::string &message) {
    Logger::instance().info(std::string("[minui overlay] ") + message);
}

void Renderer::set_sprite_layer_mode(bool enabled) {
    (void)enabled;
}

void Renderer::draw_selector(int x, int y, int w, int h) {
    Logger::instance().info(std::string("[minui selector] x=") + std::to_string(x) +
                            " y=" + std::to_string(y) + " w=" + std::to_string(w) +
                            " h=" + std::to_string(h));
}

int Renderer::get_text_width(const std::string &s) {
    // approximate: 8 px per char
    return int(s.size() * 8);
}

void Renderer::draw_game_carousel(const std::vector<Game> &games, size_t active_index, ImageCache *cache) {
    (void)cache;
    for (size_t i = 0; i < games.size(); ++i) {
        if (i == active_index) Logger::instance().info(std::string("[minui active] ") + games[i].title);
        else Logger::instance().info(std::string("[minui slot]   ") + games[i].title);
    }
}
