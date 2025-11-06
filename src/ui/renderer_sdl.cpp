#include "ui/renderer.h"
#include "core/game_db.h"
#include "core/image_cache.h"
#include "core/logger.h"

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>

#include <memory>
#include <array>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>

using core::Game;
using core::ImageCache;

namespace {
    // Screen surface (SDL 1.2 uses surfaces instead of window+renderer)
    SDL_Surface* screen = nullptr;
    TTF_Font* font = nullptr;

    // Screen dimensions (matching MinUI)
    constexpr int SCREEN_WIDTH = 640;
    constexpr int SCREEN_HEIGHT = 480;

    // UI Constants (matching MinUI implementation)
    constexpr int CAROUSEL_SPACING = 40;
    constexpr int CAROUSEL_Y_OFFSET = 160;
    constexpr int CAROUSEL_ITEM_WIDTH = 280;
    constexpr int CAROUSEL_ITEM_HEIGHT = 200;

    // Input state
    std::array<SDLKey, 9> key_map = {
        SDLK_UP,    // UP
        SDLK_DOWN,  // DOWN
        SDLK_LEFT,  // LEFT
        SDLK_RIGHT, // RIGHT
        SDLK_j,     // A (j key)
        SDLK_k,     // B (k key)
        SDLK_u,     // X (u key)
        SDLK_i,     // Y (i key)
        SDLK_m      // MENU (m key)
    };

    // Helper to get text width using TTF
    int measure_text_width(TTF_Font* font, const std::string& text) {
        int w = 0, h = 0;
        TTF_SizeText(font, text.c_str(), &w, &h);
        return w;
    }

    // Helper to draw text with shadow
    void draw_text_with_shadow(SDL_Surface* surface, TTF_Font* font, 
                             const std::string& text, int x, int y, 
                             SDL_Color color, bool highlight) {
        // Draw shadow
        SDL_Color shadow_color;
        shadow_color.r = 0;
        shadow_color.g = 0;
        shadow_color.b = 0;
        shadow_color.unused = 0;
        SDL_Surface* shadow_surface = TTF_RenderText_Blended(font, text.c_str(), shadow_color);
        if (shadow_surface) {
            SDL_Rect shadow_rect = {Sint16(x + 2), Sint16(y + 2), Uint16(shadow_surface->w), Uint16(shadow_surface->h)};
            SDL_BlitSurface(shadow_surface, nullptr, surface, &shadow_rect);
            SDL_FreeSurface(shadow_surface);
        }

        // Draw main text
        if (highlight) {
            color.r = 255;
            color.g = 255;
            color.b = 255;
            color.unused = 0;
        }
        SDL_Surface* text_surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (text_surface) {
            SDL_Rect text_rect = {Sint16(x), Sint16(y), Uint16(text_surface->w), Uint16(text_surface->h)};
            SDL_BlitSurface(text_surface, nullptr, surface, &text_rect);
            SDL_FreeSurface(text_surface);
        }
    }

}

namespace ui {

Input poll_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            // Map SDL keys to our Input enum
            for (size_t i = 0; i < key_map.size(); ++i) {
                if (event.key.keysym.sym == key_map[i]) {
                    return static_cast<Input>(i + 1); // +1 because NONE is 0
                }
            }
        }
        else if (event.type == SDL_QUIT) {
            // Handle window close
            exit(0);
        }
    }
    return Input::NONE;
}

bool Renderer::init() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        core::Logger::instance().error("SDL could not initialize: " + std::string(SDL_GetError()));
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        core::Logger::instance().error("SDL_ttf could not initialize: " + std::string(TTF_GetError()));
        SDL_Quit();
        return false;
    }

    // Initialize SDL_image
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        core::Logger::instance().error("SDL_image could not initialize: " + std::string(IMG_GetError()));
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // Create screen surface (SDL 1.2 uses SetVideoMode instead of CreateWindow+CreateRenderer)
    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
    if (!screen) {
        core::Logger::instance().error("Screen could not be created: " + std::string(SDL_GetError()));
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    SDL_WM_SetCaption("SliderUI Preview", nullptr);

    // Load BPreplay font
    font = TTF_OpenFont("res/BPreplayBold-unhinted.otf", 28);  // Increased font size, using correct path
    if (!font) {
        core::Logger::instance().error("Failed to load BPreplay font: " + std::string(TTF_GetError()));
        SDL_FreeSurface(screen);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}

void Renderer::shutdown() {
    if (font) TTF_CloseFont(font);
    if (screen) SDL_FreeSurface(screen);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void Renderer::clear() {
    SDL_FillRect(screen, nullptr, SDL_MapRGB(screen->format, 0, 0, 0));
}

void Renderer::present() {
    SDL_Flip(screen);
    // Cap at 60 FPS
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

void Renderer::draw_text(int x, int y, const std::string &s, bool highlight) {
    SDL_Color color;
    if (highlight) {
        color.r = 255;
        color.g = 255;
        color.b = 255;
    } else {
        color.r = 200;
        color.g = 200;
        color.b = 200;
    }
    color.unused = 0;
    draw_text_with_shadow(screen, font, s, x, y, color, highlight);
}

int Renderer::get_text_width(const std::string &s) {
    if (!font || s.empty()) return 0;
    return measure_text_width(font, s);
}

void Renderer::draw_background(const std::string &path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        core::Logger::instance().error("Could not load background: " + path);
        return;
    }

    // Scale surface to screen size if needed
    if (surface->w != SCREEN_WIDTH || surface->h != SCREEN_HEIGHT) {
        SDL_Surface* scaled = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 
                                                   screen->format->BitsPerPixel,
                                                   screen->format->Rmask, screen->format->Gmask,
                                                   screen->format->Bmask, screen->format->Amask);
        if (scaled) {
            SDL_SoftStretch(surface, nullptr, scaled, nullptr);
            SDL_FreeSurface(surface);
            surface = scaled;
        }
    }

    // Convert to screen format (SDL 1.2 - always convert for optimal blitting)
    SDL_Surface* converted = SDL_DisplayFormat(surface);
    if (converted) {
        SDL_FreeSurface(surface);
        surface = converted;
    }

    SDL_BlitSurface(surface, nullptr, screen, nullptr);
    SDL_FreeSurface(surface);
}

void Renderer::draw_game_carousel(const std::vector<Game> &view, std::size_t active, [[maybe_unused]] ImageCache *cache) {
    if (view.empty() || active >= view.size()) return;

    const int center_x = SCREEN_WIDTH / 2 - CAROUSEL_ITEM_WIDTH / 2;
    const int left_x = center_x - CAROUSEL_ITEM_WIDTH - CAROUSEL_SPACING;
    const int right_x = center_x + CAROUSEL_ITEM_WIDTH + CAROUSEL_SPACING;

    // Draw each visible game (up to 3)
    for (size_t i = 0; i < std::min(view.size(), size_t(3)); ++i) {
        const Game &game = view[i];
        int x = (i == 0) ? left_x : (i == 1) ? center_x : right_x;
        
        // Try to load game image
        SDL_Surface* surface = IMG_Load(game.path.c_str());
        if (surface) {
            // Scale to carousel item size if needed
            if (surface->w != CAROUSEL_ITEM_WIDTH || surface->h != CAROUSEL_ITEM_HEIGHT) {
                SDL_Surface* scaled = SDL_CreateRGBSurface(SDL_SWSURFACE, CAROUSEL_ITEM_WIDTH, CAROUSEL_ITEM_HEIGHT,
                                                           screen->format->BitsPerPixel,
                                                           screen->format->Rmask, screen->format->Gmask,
                                                           screen->format->Bmask, screen->format->Amask);
                if (scaled) {
                    SDL_SoftStretch(surface, nullptr, scaled, nullptr);
                    SDL_FreeSurface(surface);
                    surface = scaled;
                }
            }

            // Convert to screen format (SDL 1.2 - always convert for optimal blitting)
            SDL_Surface* converted = SDL_DisplayFormat(surface);
            if (converted) {
                SDL_FreeSurface(surface);
                surface = converted;
            }

            SDL_Rect dst = {Sint16(x), Sint16(CAROUSEL_Y_OFFSET), Uint16(CAROUSEL_ITEM_WIDTH), Uint16(CAROUSEL_ITEM_HEIGHT)};
            SDL_BlitSurface(surface, nullptr, screen, &dst);
            SDL_FreeSurface(surface);
        }

        // Draw selection border for active item
        if (i == active) {
            Uint32 white = SDL_MapRGB(screen->format, 255, 255, 255);
            SDL_Rect border = {Sint16(x - 2), Sint16(CAROUSEL_Y_OFFSET - 2), 
                             Uint16(CAROUSEL_ITEM_WIDTH + 4), Uint16(CAROUSEL_ITEM_HEIGHT + 4)};
            // Draw border lines manually (SDL 1.2 doesn't have SDL_RenderDrawRect)
            SDL_Rect top = {border.x, border.y, border.w, 1};
            SDL_Rect bottom = {border.x, Sint16(border.y + border.h - 1), border.w, 1};
            SDL_Rect left = {border.x, border.y, 1, border.h};
            SDL_Rect right = {Sint16(border.x + border.w - 1), border.y, 1, border.h};
            SDL_FillRect(screen, &top, white);
            SDL_FillRect(screen, &bottom, white);
            SDL_FillRect(screen, &left, white);
            SDL_FillRect(screen, &right, white);
        }
    }
}

void Renderer::draw_overlay(const std::string &msg) {
    // Semi-transparent overlay background
    // SDL 1.2 doesn't have alpha blending for FillRect, so we'll use a solid dark color
    // For a semi-transparent effect, we'd need to use SDL_SetAlpha or manually blend
    Uint32 dark_color = SDL_MapRGB(screen->format, 0, 0, 0);
    
    const int padding = 20;
    SDL_Rect overlay = {
        Sint16(SCREEN_WIDTH/4),
        Sint16(SCREEN_HEIGHT/2 - 30),
        Uint16(SCREEN_WIDTH/2),
        Uint16(60)
    };
    
    SDL_FillRect(screen, &overlay, dark_color);

    // Draw border
    Uint32 white = SDL_MapRGB(screen->format, 255, 255, 255);
    SDL_Rect border = overlay;
    SDL_Rect top = {border.x, border.y, border.w, 1};
    SDL_Rect bottom = {border.x, Sint16(border.y + border.h - 1), border.w, 1};
    SDL_Rect left = {border.x, border.y, 1, border.h};
    SDL_Rect right = {Sint16(border.x + border.w - 1), border.y, 1, border.h};
    SDL_FillRect(screen, &top, white);
    SDL_FillRect(screen, &bottom, white);
    SDL_FillRect(screen, &left, white);
    SDL_FillRect(screen, &right, white);

    // Draw message
    SDL_Color white_color;
    white_color.r = 255;
    white_color.g = 255;
    white_color.b = 255;
    white_color.unused = 0;
    draw_text_with_shadow(screen, font, msg,
                         overlay.x + padding,
                         overlay.y + (overlay.h - TTF_FontHeight(font))/2,
                         white_color, false);
}

void Renderer::draw_selector(int x, int y, int width, int height) {
    // Set up semi-transparent white for selector
    // SDL 1.2 doesn't have blend modes like SDL 2, so we'll use a lighter gray
    Uint32 selector_color = SDL_MapRGB(screen->format, 128, 128, 128);

    // Draw filled rounded rectangle
    const int radius = height / 2;
    
    // Draw main rectangle body
    SDL_Rect body = {Sint16(x + radius), Sint16(y), Uint16(width - 2 * radius), Uint16(height)};
    SDL_FillRect(screen, &body, selector_color);
    
    // Draw left semi-circle
    for (int dy = 0; dy < radius; dy++) {
        int dx = (int)sqrt((double)(radius * radius - dy * dy));
        SDL_Rect line = {Sint16(x + radius - dx), Sint16(y + radius - dy), Uint16(dx), Uint16(1)};
        SDL_FillRect(screen, &line, selector_color);
        line.y = Sint16(y + radius + dy);
        SDL_FillRect(screen, &line, selector_color);
    }
    
    // Draw right semi-circle
    for (int dy = 0; dy < radius; dy++) {
        int dx = (int)sqrt((double)(radius * radius - dy * dy));
        SDL_Rect line = {Sint16(x + width - radius), Sint16(y + radius - dy), Uint16(dx), Uint16(1)};
        SDL_FillRect(screen, &line, selector_color);
        line.y = Sint16(y + radius + dy);
        SDL_FillRect(screen, &line, selector_color);
    }
}

} // namespace ui
