// src/ui/renderer_stub.cpp
#include "ui/renderer.h"
#include "core/game_db.h"
#include "core/image_cache.h"

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/select.h>

using core::Game;
using core::ImageCache;

namespace ui {

// Helper: non-blocking single-char read from stdin. Returns 0 if no char available.
static int stdin_peek_char_nonblock() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int rv = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
    if (rv > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
        // read single byte
        unsigned char c = 0;
        ssize_t r = read(STDIN_FILENO, &c, 1);
        if (r == 1) return static_cast<int>(c);
    }
    return 0;
}

Input poll_input() {
    int c = stdin_peek_char_nonblock();
    if (c == 0) return Input::NONE;
    // Map keys:
    // 'w' => UP, 's'=> DOWN, 'a' => LEFT, 'd' => RIGHT,
    // 'j' => A, 'k' => B, 'u' => X, 'i' => Y, 'm' => MENU
    switch (c) {
        case 'w': case 'W': return Input::UP;
        case 's': case 'S': return Input::DOWN;
        case 'a': case 'A': return Input::LEFT;
        case 'd': case 'D': return Input::RIGHT;
        case 'j': case 'J': return Input::A;
        case 'k': case 'K': return Input::B;
        case 'u': case 'U': return Input::X;
        case 'i': case 'I': return Input::Y;
        case 'm': case 'M': return Input::MENU;
        default: return Input::NONE;
    }
}

/* Renderer methods: headless/no-op but log actions. */

bool Renderer::init() {
    std::cout << "[renderer] init (stub)\n";
    std::cout << "[renderer] stdin keys: w/up, s/down, a/left, d/right, j=A, k=B, u=X, i=Y, m=MENU\n";
    return true;
}

void Renderer::shutdown() {
    std::cout << "[renderer] shutdown (stub)\n";
}

void Renderer::clear() {
    std::cout << "[renderer] clear()\n";
}

void Renderer::present() {
    std::cout << "[renderer] present()\n";
}

void Renderer::draw_text(int x, int y, const std::string &s, bool highlight) {
    std::cout << "[renderer] draw_text (" << x << "," << y << ")"
              << (highlight ? " [HIGHLIGHT]" : "") << ": " << s << "\n";
}

int Renderer::get_text_width(const std::string &s) {
    // Stub implementation - return approximate width
    return static_cast<int>(s.length() * 7);
}

void Renderer::draw_background(const std::string &path) {
    std::cout << "[renderer] draw_background: " << path << "\n";
}

void Renderer::draw_game_carousel(const std::vector<Game> &view, std::size_t active, ImageCache *cache) {
    std::cout << "[renderer] draw_game_carousel (size=" << view.size() << ", active=" << active << ")\n";
    for (std::size_t i = 0; i < view.size(); ++i) {
        const Game &g = view[i];
        std::string name = !g.name.empty() ? g.name : g.path;
        std::cout << "  [" << (i == active ? ">" : " ") << "] " << name
                  << " (order=" << g.order << ", platform=" << g.platform_id;
        if (g.platform_core.has_value()) std::cout << " core=" << *g.platform_core;
        std::cout << ")";
        // If cache provided, indicate if texture is present
        if (cache) {
            auto tex = cache->get(g.path);
            std::cout << " tex=" << (tex.has_value() ? "cached" : "miss");
        }
        std::cout << "\n";
    }
}

void Renderer::draw_overlay(const std::string &msg) {
    std::cout << "[renderer] overlay: " << msg << "\n";
}

} // namespace ui
