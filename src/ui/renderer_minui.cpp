#include "ui/renderer.h"
#include "core/game_db.h"
#include "core/image_cache.h"
#include "core/logger.h"

// MinUI specific headers
extern "C" {
#include "platform.h"           // MinUI platform functions
#include "hardware.h"          // Hardware specific definitions
#include "api.h"              // MinUI API functions
}

#include <memory>
#include <array>
#include <cstring>
#include <cmath>

using core::Game;
using core::ImageCache;

namespace {

// Pre-allocated buffers for rendering
constexpr size_t MAX_TEXT_LENGTH = 256;
std::array<char, MAX_TEXT_LENGTH> text_buffer;

// Screen dimensions (from MinUI platform.h)
// Screen dimensions are defined in hardware.h

// UI Constants
constexpr int CAROUSEL_SPACING = 20;  // Pixels between carousel items
constexpr int CAROUSEL_Y_OFFSET = 140; // Y position of carousel
constexpr int CAROUSEL_ITEM_WIDTH = 240;
constexpr int CAROUSEL_ITEM_HEIGHT = 160;

// Pre-allocated memory for image scaling
std::unique_ptr<uint16_t[]> scale_buffer;

// MinUI color constants
constexpr uint16_t COLOR_WHITE = 0xFFFF;
constexpr uint16_t COLOR_BLACK = 0x0000;
constexpr uint16_t COLOR_HIGHLIGHT = 0xF800; // Red for highlight

// Input mapping from MinUI to our Input enum
std::array<std::pair<uint32_t, ui::Input>, 9> input_map = {{
    {BTN_UP, ui::Input::UP},
    {BTN_DOWN, ui::Input::DOWN},
    {BTN_LEFT, ui::Input::LEFT},
    {BTN_RIGHT, ui::Input::RIGHT},
    {BTN_A, ui::Input::A},
    {BTN_B, ui::Input::B},
    {BTN_X, ui::Input::X},
    {BTN_Y, ui::Input::Y},
    {BTN_MENU, ui::Input::MENU}
}};

} // namespace

namespace ui {

Input poll_input() {
    // Get raw input from MinUI
    uint32_t raw_input = platform_poll_input();
    if (raw_input == 0) return Input::NONE;
    
    // Map to our input enum
    for (const auto& [btn, input] : input_map) {
        if (raw_input & btn) return input;
    }
    
    return Input::NONE;
}

bool Renderer::init() {
    // Initialize MinUI platform
    if (!platform_init()) {
        core::Logger::instance().error("Failed to initialize MinUI platform");
        return false;
    }

    // Initialize display
    if (!display_init()) {
        core::Logger::instance().error("Failed to initialize display");
        platform_quit();
        return false;
    }

    // Note: Font initialization is handled by MinUI platform layer
    // No explicit font_init call needed

    // Allocate scaling buffer for images
    scale_buffer = std::make_unique<uint16_t[]>(SCREEN_WIDTH * SCREEN_HEIGHT);
    if (!scale_buffer) {
        core::Logger::instance().error("Failed to allocate scaling buffer");
        display_quit();
        platform_quit();
        return false;
    }

    return true;
}

void Renderer::shutdown() {
    scale_buffer.reset();
    display_quit();
    platform_quit();
}

void Renderer::clear() {
    // Clear screen buffer to black
    display_fill(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK);
}

void Renderer::present() {
    // Flip display buffer
    display_flip();
}

void Renderer::draw_text(int x, int y, const std::string &s, bool highlight) {
    if (s.empty()) return;
    
    // Safely copy string to our pre-allocated buffer
    size_t len = std::min(s.length(), MAX_TEXT_LENGTH - 1);
    std::memcpy(text_buffer.data(), s.c_str(), len);
    text_buffer[len] = '\0';
    
    // Draw text using MinUI font functions
    display_text(x, y, text_buffer.data(), highlight ? COLOR_HIGHLIGHT : COLOR_WHITE);
}

int Renderer::get_text_width(const std::string &s) {
    // Estimate width for MinUI (approximate: ~6-8 pixels per character)
    // MinUI doesn't expose text measurement, so we use approximation
    return static_cast<int>(s.length() * 7);
}

void Renderer::draw_background(const std::string &path) {
    // Load and draw background image using MinUI's image loading
    Image bg;
    if (!image_load(&bg, path.c_str())) {
        core::Logger::instance().error("Failed to load background: " + path);
        return;
    }
    
    // Scale and display
    display_image(&bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    image_free(&bg);
}

void Renderer::draw_game_carousel(const std::vector<Game> &view, std::size_t active, ImageCache *cache) {
    if (view.empty() || active >= view.size()) return;

    // Calculate positions for 3-item carousel
    const int center_x = SCREEN_WIDTH / 2 - CAROUSEL_ITEM_WIDTH / 2;
    const int left_x = center_x - CAROUSEL_ITEM_WIDTH - CAROUSEL_SPACING;
    const int right_x = center_x + CAROUSEL_ITEM_WIDTH + CAROUSEL_SPACING;

    // Draw items (up to 3)
    for (size_t i = 0; i < std::min(view.size(), size_t(3)); ++i) {
        const Game &game = view[i];
        int x = (i == 0) ? left_x : (i == 1) ? center_x : right_x;
        
        // Try to get cached texture
        if (cache) {
            auto tex = cache->get(game.path);
            if (tex) {
                // Draw cached texture
                display_blit(tex->pixels.data(), x, CAROUSEL_Y_OFFSET, 
                           tex->width, tex->height);
                continue;
            }
        }
        
        // Fallback: try to load image directly
        Image img;
        if (image_load(&img, game.path.c_str())) {
            display_image(&img, x, CAROUSEL_Y_OFFSET, 
                         CAROUSEL_ITEM_WIDTH, CAROUSEL_ITEM_HEIGHT);
            image_free(&img);
        }
        
        // Draw selection border for active item
        if (i == active) {
            display_rect(x - 2, CAROUSEL_Y_OFFSET - 2,
                        CAROUSEL_ITEM_WIDTH + 4, CAROUSEL_ITEM_HEIGHT + 4,
                        COLOR_HIGHLIGHT);
        }
    }
}

void Renderer::draw_overlay(const std::string &msg) {
    if (msg.empty()) return;

    // Create semi-transparent overlay
    const int padding = 10;
    const int overlay_width = SCREEN_WIDTH / 2;
    const int overlay_height = 40;
    const int overlay_x = (SCREEN_WIDTH - overlay_width) / 2;
    const int overlay_y = (SCREEN_HEIGHT - overlay_height) / 2;

    // Draw overlay background
    display_fill(overlay_x, overlay_y, overlay_width, overlay_height, 
                COLOR_BLACK);
    display_rect(overlay_x, overlay_y, overlay_width, overlay_height, 
                COLOR_WHITE);

    // Draw message
    size_t len = std::min(msg.length(), MAX_TEXT_LENGTH - 1);
    std::memcpy(text_buffer.data(), msg.c_str(), len);
    text_buffer[len] = '\0';

    int text_x = overlay_x + padding;
    int text_y = overlay_y + (overlay_height - 12) / 2; // Assuming 12px font height
    display_text(text_x, text_y, text_buffer.data(), COLOR_WHITE);
}

void Renderer::draw_selector(int x, int y, int width, int height) {
    const int radius = height / 2;
    const uint16_t selector_color = 0x8410; // Semi-transparent white

    // Draw main rectangle body
    display_fill(x + radius, y, width - 2 * radius, height, selector_color);
    
    // Draw left rounded edge
    for (int dy = 0; dy < radius; dy++) {
        int dx = (int)((float)radius * sqrt(1.0f - (dy * dy) / (float)(radius * radius)));
        display_fill(x + radius - dx, y + radius - dy, dx, 1, selector_color);
        display_fill(x + radius - dx, y + radius + dy, dx, 1, selector_color);
    }
    
    // Draw right rounded edge
    for (int dy = 0; dy < radius; dy++) {
        int dx = (int)((float)radius * sqrt(1.0f - (dy * dy) / (float)(radius * radius)));
        display_fill(x + width - radius, y + radius - dy, dx, 1, selector_color);
        display_fill(x + width - radius, y + radius + dy, dx, 1, selector_color);
    }
}

} // namespace ui