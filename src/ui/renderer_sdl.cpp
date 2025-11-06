// src/ui/renderer_sdl.cpp
// SDL 1.2 renderer for sliderUI — rounded selector pill + correct text colors
// Fixed: draw_solid_pill draws both semicircles, and top highlight clipped to pill.

#include "core/global.h"
#include "ui/renderer.h"
#include "core/logger.h"
#include "core/image_cache.h"
#include "core/game_db.h"
#include "core/config_manager.h"

#include <SDL/SDL.h>
#ifdef HAVE_SDL_TTF
#include <SDL/SDL_ttf.h>
#endif
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <iostream>

using namespace ui;
using core::ImageCache;
using core::Game;
using core::Logger;

struct ui::Renderer::Impl {
    SDL_Surface *screen = nullptr;
#ifdef HAVE_SDL_TTF
    TTF_Font *font = nullptr;
#endif
    int width = 800;
    int height = 480;
    bool sprite_layer_mode = false;
    SDL_Surface *sprite_atlas = nullptr;
    std::unordered_map<std::string, SDL_Rect> atlas_map;
};

static std::string basename_from_path(const std::string &p) {
    if (p.empty()) return std::string();
    auto pos = p.find_last_of("/\\");
    if (pos == std::string::npos) return p;
    return p.substr(pos + 1);
}

static std::string find_art_for_game(const Game &g) {
    std::string label = g.name.empty() ? basename_from_path(g.path) : g.name;

    // sanitize spaces etc. if your filesystem needs it (optional)
    // std::replace(label.begin(), label.end(), ' ', '_');

    const std::vector<std::string> exts = {".png", ".jpg", ".jpeg", ".webp"};
    std::string base = global::g_exe_dir + "assets/img/" + label;

    for (const auto &ext : exts) {
        std::string full = base + ext;
        FILE *f = fopen(full.c_str(), "rb");
        if (f) { fclose(f); return full; }
    }

    return std::string(); // not found
}

namespace ui {
    
    Input poll_input() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: return Input::UP;
                    case SDLK_DOWN: return Input::DOWN;
                    case SDLK_LEFT: return Input::LEFT;
                    case SDLK_RIGHT: return Input::RIGHT;
                    case SDLK_j: return Input::A;
                    case SDLK_k: return Input::B;
                    case SDLK_u: return Input::X;
                    case SDLK_i: return Input::Y;
                    case SDLK_m: return Input::MENU;
                    case SDLK_q: return Input::START;
                    case SDLK_w: return Input::SELECT;
                    default: return Input::NONE;
                }
            } else if (event.type == SDL_QUIT) {
                exit(0);
            }
        }
        return Input::NONE;
    }
}

// Update the set_config method implementation:
void Renderer::set_config(core::ConfigManager *cfg) {
    config_ = cfg;
}

Renderer::Renderer() : pimpl(new Impl()) {}
Renderer::~Renderer() { shutdown(); delete pimpl; }

bool Renderer::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        Logger::instance().error(std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    pimpl->width = 640;   // match old behavior
    pimpl->height = 480;

    pimpl->screen = SDL_SetVideoMode(pimpl->width, pimpl->height, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
    if (!pimpl->screen) {
        Logger::instance().error(std::string("SDL_SetVideoMode failed: ") + SDL_GetError());
        return false;
    }

#ifdef HAVE_SDL_TTF
if (TTF_Init() == -1) {
    Logger::instance().error(std::string("TTF_Init failed: ") + TTF_GetError());
    pimpl->font = nullptr;
} else {

    std::vector<std::string> try_paths = {
        global::g_exe_dir + "/assets/font/BPreplayBold-unhinted.otf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/DejaVuSans.ttf",
        "/mnt/SDCARD/.tmp_update/fonts/DejaVuSans.ttf",
        "DejaVuSans.ttf"
    };

    for (const auto& path : try_paths) {
        pimpl->font = TTF_OpenFont(path.c_str(), 28);
        if (pimpl->font) {
            Logger::instance().info("Loaded font: " + path);
            break;
        }
    }

    if (!pimpl->font) {
        Logger::instance().error("Could not load any font. Text will be invisible.");
    }
}
#endif

    return true;
}

void Renderer::shutdown() {
#ifdef HAVE_SDL_TTF
    if (pimpl->font) { TTF_CloseFont(pimpl->font); pimpl->font = nullptr; }
    TTF_Quit();
#endif
    if (pimpl->sprite_atlas) {
        SDL_FreeSurface(pimpl->sprite_atlas);
        pimpl->sprite_atlas = nullptr;
        pimpl->atlas_map.clear();
    }
    if (pimpl->screen) {
        SDL_FreeSurface(pimpl->screen);
        pimpl->screen = nullptr;
    }
    SDL_Quit();
}

void Renderer::clear() {
    if (!pimpl->screen) return;
    SDL_FillRect(pimpl->screen, nullptr, SDL_MapRGB(pimpl->screen->format, 10, 10, 10));
}

void Renderer::present() {
    if (!pimpl->screen) return;
    SDL_Flip(pimpl->screen);
}

void Renderer::draw_background(const std::string &background_image) {
    (void)background_image;
}

// ---------- helpers ----------

// Low-level fill rect (used internally) with clipping to surface bounds.
static void fill_rect(SDL_Surface *dst, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
    if (!dst || w <= 0 || h <= 0) return;

    SDL_Rect sr;
    sr.x = static_cast<Sint16>(x);
    sr.y = static_cast<Sint16>(y);
    sr.w = static_cast<Uint16>(w);
    sr.h = static_cast<Uint16>(h);

    // Clip to surface bounds
    if (sr.x < 0) { sr.w += sr.x; sr.x = 0; }
    if (sr.y < 0) { sr.h += sr.y; sr.y = 0; }
    if (sr.x >= dst->w || sr.y >= dst->h) return;
    if (sr.x + sr.w > dst->w) sr.w = dst->w - sr.x;
    if (sr.y + sr.h > dst->h) sr.h = dst->h - sr.y;
    if (sr.w <= 0 || sr.h <= 0) return;

    Uint32 color = SDL_MapRGB(dst->format, r, g, b);
    SDL_FillRect(dst, &sr, color);
}

// Compatibility wrapper: some code calls draw_filled_rect — keep that symbol.
static void draw_filled_rect(SDL_Surface *dst, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
    fill_rect(dst, x, y, w, h, r, g, b);
}

// draw a solid rounded pill by horizontal spans (central rect + both semicircles)
static void draw_solid_pill(SDL_Surface *dst, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
    if (!dst || w <= 0 || h <= 0) return;
    int radius = h / 2;
    if (radius < 1) radius = 1;

    // central rect
    int inner_x = x + radius;
    int inner_w = w - 2 * radius;
    if (inner_w > 0) {
        fill_rect(dst, inner_x, y, inner_w, h, r, g, b);
    }

    int cy = y + h / 2;

    // left semicircle
    int cx_left = x + radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        int absdy = std::abs(dy);
        if (absdy > radius) continue;
        int dx = static_cast<int>(std::floor(std::sqrt(double(radius*radius - absdy*absdy))));
        int sx = cx_left - dx;
        int span = dx * 2 + 1;
        fill_rect(dst, sx, cy + dy, span, 1, r, g, b);
    }

    // right semicircle
    int cx_right = x + w - radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        int absdy = std::abs(dy);
        if (absdy > radius) continue;
        int dx = static_cast<int>(std::floor(std::sqrt(double(radius*radius - absdy*absdy))));
        int sx = cx_right - dx;
        int span = dx * 2 + 1;
        fill_rect(dst, sx, cy + dy, span, 1, r, g, b);
    }
}

// Draw a horizontal 1px line that is clipped to the pill outline (so it won't show outside rounded corners)
// y_line is absolute y coordinate where the 1px line should be drawn
static void draw_pill_hline(SDL_Surface *dst, int x, int y, int w, int h, int y_line, Uint8 r, Uint8 g, Uint8 b) {
    if (!dst || w <= 0 || h <= 0) return;
    int radius = h / 2;
    if (radius < 1) radius = 1;
    int cy = y + h / 2;

    int inner_x = x + radius;
    int inner_w = w - 2 * radius;

    int dy = y_line - cy;
    if (std::abs(dy) > radius) return; // outside pill

    // draw central area if it exists
    if (inner_w > 0) {
        fill_rect(dst, inner_x, y_line, inner_w, 1, r, g, b);
    }

    // left cap
    int dx_left = static_cast<int>(std::floor(std::sqrt(double(radius*radius - dy*dy))));
    int lx = (x + radius) - dx_left;
    int lspan = dx_left * 2 + 1;
    // Clip to leftmost central boundary so we don't overdrawing overlap is harmless but we clip anyway.
    fill_rect(dst, lx, y_line, lspan, 1, r, g, b);

    // right cap
    int dx_right = dx_left; // symmetric
    int rx = (x + w - radius) - dx_right;
    int rspan = dx_right * 2 + 1;
    fill_rect(dst, rx, y_line, rspan, 1, r, g, b);
}

// Draw rounded pill by composing border (outer) and inner fill (inset by 1 px).
static void draw_rounded_pill(SDL_Surface *dst, int x, int y, int w, int h,
                              Uint8 r, Uint8 g, Uint8 b, Uint8 br, Uint8 bg, Uint8 bb) {
    if (!dst || w <= 0 || h <= 0) return;

    // Outer pill = border
    draw_solid_pill(dst, x, y, w, h, br, bg, bb);

    // Inner pill inset
    const int inset = 1;
    int ix = x + inset;
    int iy = y + inset;
    int iw = w - 2 * inset;
    int ih = h - 2 * inset;
    if (iw > 0 && ih > 0) {
        draw_solid_pill(dst, ix, iy, iw, ih, r, g, b);
    }
}

// ---------- rendering functions ----------

void Renderer::draw_text(int x, int y, const std::string &s, bool highlight) {
#ifdef HAVE_SDL_TTF
    if (!pimpl->font || !pimpl->screen) return;

    // Colors: selected -> white; unselected -> light gray
    SDL_Color fg;
    if (highlight) { fg.r = 255; fg.g = 255; fg.b = 255; fg.unused = 0; }
    else           { fg.r = 200; fg.g = 200; fg.b = 200; fg.unused = 0; }

    SDL_Color shadow;
    shadow.r = 0; shadow.g = 0; shadow.b = 0; shadow.unused = 0;

    // Shadow (offset 2,2)
    SDL_Surface *shadow_surf = TTF_RenderUTF8_Blended(pimpl->font, s.c_str(), shadow);
    if (shadow_surf) {
        SDL_Rect dst{ static_cast<Sint16>(x + 2), static_cast<Sint16>(y + 2),
                      static_cast<Uint16>(shadow_surf->w), static_cast<Uint16>(shadow_surf->h) };
        SDL_BlitSurface(shadow_surf, nullptr, pimpl->screen, &dst);
        SDL_FreeSurface(shadow_surf);
    }

    // Main text
    SDL_Surface *surf = TTF_RenderUTF8_Blended(pimpl->font, s.c_str(), fg);
    if (!surf) return;
    SDL_Rect dst{ static_cast<Sint16>(x), static_cast<Sint16>(y),
                  static_cast<Uint16>(surf->w), static_cast<Uint16>(surf->h) };

    SDL_BlitSurface(surf, nullptr, pimpl->screen, &dst);
    SDL_FreeSurface(surf);
#else
    (void)x; (void)y; (void)s; (void)highlight;
#endif
}

void Renderer::draw_overlay(const std::string &message) {
    if (!pimpl->screen) return;
    int w = pimpl->width - 100;
    int h = 160;
    int x = 50;
    int y = (pimpl->height - h) / 2;
    // dark background
    fill_rect(pimpl->screen, x, y, w, h, 12, 12, 12);
    draw_text(x + 12, y + 12, message, true);
}

void Renderer::draw_selector(int x, int y, int w, int h) {
    if (!pimpl->screen) return;
    // Gray pill fill + darker border
    Uint8 fill_r = 96;
    Uint8 fill_g = 96;
    Uint8 fill_b = 96;
    Uint8 border_r = 60;
    Uint8 border_g = 60;
    Uint8 border_b = 60;

    // Draw border + inner fill (no square border artifact)
    draw_rounded_pill(pimpl->screen, x, y, w, h, fill_r, fill_g, fill_b, border_r, border_g, border_b);

    // subtle top highlight inside inner area (inset so it follows rounded edge)
    int hi_inset = 2;
    int hi_y = y + hi_inset;
    Uint8 hr = static_cast<Uint8>(std::min<int>(255, fill_r + 20));
    Uint8 hg = static_cast<Uint8>(std::min<int>(255, fill_g + 20));
    Uint8 hb = static_cast<Uint8>(std::min<int>(255, fill_b + 20));
    draw_pill_hline(pimpl->screen, x + 1, y + 1, w - 2, h - 2, hi_y, hr, hg, hb);
}

int Renderer::get_text_width(const std::string &s) {
#ifdef HAVE_SDL_TTF
    if (!pimpl->font) return int(s.size() * 8);
    int w = 0, h = 0;
    TTF_SizeUTF8(pimpl->font, s.c_str(), &w, &h);
    return w;
#else
    return int(s.size() * 8);
#endif
}

void Renderer::set_sprite_layer_mode(bool enabled) {
    pimpl->sprite_layer_mode = enabled;
    if (!enabled && pimpl->sprite_atlas) {
        SDL_FreeSurface(pimpl->sprite_atlas);
        pimpl->sprite_atlas = nullptr;
        pimpl->atlas_map.clear();
    }
}

static SDL_Surface *build_atlas(SDL_Surface *screen, const std::vector<std::pair<std::string, SDL_Surface*>> &imgs,
                                std::unordered_map<std::string, SDL_Rect> &out_map) {
    if (imgs.empty() || !screen) return nullptr;
    int total_w = 0;
    int max_h = 0;
    for (auto &p : imgs) { if (p.second) { total_w += p.second->w; max_h = std::max(max_h, p.second->h); } }
    if (total_w <= 0 || max_h <= 0) return nullptr;

    SDL_Surface *atlas = SDL_CreateRGBSurface(SDL_SWSURFACE, total_w, max_h,
                                              screen->format->BitsPerPixel,
                                              screen->format->Rmask, screen->format->Gmask,
                                              screen->format->Bmask, screen->format->Amask);
    if (!atlas) return nullptr;

    int x = 0;
    for (auto &p : imgs) {
        SDL_Surface *s = p.second;
        SDL_Rect dst;
        dst.x = static_cast<Sint16>(x);
        dst.y = 0;
        dst.w = s ? s->w : 0;
        dst.h = s ? s->h : 0;
        if (s) SDL_BlitSurface(s, nullptr, atlas, &dst);
        out_map[p.first] = dst;
        x += dst.w;
    }
    return atlas;
}

void Renderer::draw_game_carousel(const std::vector<Game> &games, size_t active_index, ImageCache *cache) {
    if (!pimpl->screen) return;
    
    // Default values (fallback if config not set)
    int center_x = pimpl->width / 2;
    int center_y = pimpl->height / 2 - 20;
    int active_w = 360;
    int active_h = 200;
    int side_w = 220;
    int side_h = 140;
    int spacing = 24;
    double side_scale = 0.78;
    
    // Load from config if available - use this->config_ member variable
    if (this->config_) {
        auto *cfg = this->config_;
        
        // Read game_image config section
        center_x = cfg->get<int>("ui.game_image.x", pimpl->width / 2);
        center_y = cfg->get<int>("ui.game_image.y", pimpl->height / 2 - 20);
        active_w = cfg->get<int>("ui.game_image.width", 360);
        active_h = cfg->get<int>("ui.game_image.height", 200);
        spacing = cfg->get<int>("ui.game_image.margin", 24);
        side_scale = cfg->get<double>("ui.game_image.side_scale", 0.78);
        
        // Calculate side dimensions from active dimensions and side_scale
        side_w = static_cast<int>(active_w * side_scale);
        side_h = static_cast<int>(active_h * side_scale);
    }

    if (pimpl->sprite_layer_mode) {
        std::vector<std::pair<std::string, SDL_Surface*>> entries;
        for (size_t i = 0; i < games.size(); ++i) {
            SDL_Surface *surf = nullptr;
            if (cache) surf = static_cast<SDL_Surface*>(cache->get_surface_for_path(games[i].path));
            std::string art_path = find_art_for_game(games[i]);
            if (!art_path.empty())
                entries.emplace_back(art_path, static_cast<SDL_Surface*>(cache->get_surface_for_path(art_path)));
            else
                entries.emplace_back(games[i].path, static_cast<SDL_Surface*>(cache->get_surface_for_path(games[i].path)));
        }
        if (pimpl->sprite_atlas) { SDL_FreeSurface(pimpl->sprite_atlas); pimpl->sprite_atlas = nullptr; pimpl->atlas_map.clear(); }
        pimpl->sprite_atlas = build_atlas(pimpl->screen, entries, pimpl->atlas_map);
    }

    for (size_t i = 0; i < games.size(); ++i) {
        int rel = int(i) - int(active_index);
        int w = (rel == 0) ? active_w : side_w;
        int h = (rel == 0) ? active_h : side_h;
        int x = center_x + rel * (w + spacing) - w / 2;
        int y = center_y - h / 2;

        // draw border
        draw_filled_rect(pimpl->screen, x - 6, y - 6, w + 12, h + 12, 30, 30, 30);

        SDL_Surface *art = nullptr;
        std::string art_path = find_art_for_game(games[i]);
        if (!art_path.empty()) {
            art = static_cast<SDL_Surface*>(cache->get_surface_for_path(art_path));
        } else {
            if (cache) art = static_cast<SDL_Surface*>(cache->get_surface_for_path(games[i].path));
        }

        // sprite atlas rendering
        if (pimpl->sprite_layer_mode && pimpl->sprite_atlas) {
            auto it = pimpl->atlas_map.find(games[i].path);
            if (it != pimpl->atlas_map.end()) {
                SDL_Rect src = it->second;
                SDL_Rect dst;
                dst.x = static_cast<Sint16>(x);
                dst.y = static_cast<Sint16>(y);
                dst.w = static_cast<Uint16>(w);
                dst.h = static_cast<Uint16>(h);

                if (static_cast<int>(src.w) == dst.w && static_cast<int>(src.h) == dst.h) {
                    SDL_BlitSurface(pimpl->sprite_atlas, &src, pimpl->screen, &dst);
                } else {
                    SDL_Surface *temp = SDL_CreateRGBSurface(SDL_SWSURFACE, dst.w, dst.h,
                                                             pimpl->screen->format->BitsPerPixel,
                                                             pimpl->screen->format->Rmask,
                                                             pimpl->screen->format->Gmask,
                                                             pimpl->screen->format->Bmask,
                                                             pimpl->screen->format->Amask);
                    if (temp) {
                        SDL_Rect crop = src;
                        int crop_w = static_cast<int>(crop.w);
                        int crop_h = static_cast<int>(crop.h);
                        if (crop_w > dst.w) { crop.x += (crop_w - dst.w) / 2; crop_w = std::min<int>(crop_w, static_cast<int>(dst.w)); crop.w = static_cast<Uint16>(crop_w); }
                        if (crop_h > dst.h) { crop.y += (crop_h - dst.h) / 2; crop_h = std::min<int>(crop_h, static_cast<int>(dst.h)); crop.h = static_cast<Uint16>(crop_h); }
                        SDL_Rect dest0; dest0.x = 0; dest0.y = 0; dest0.w = crop.w; dest0.h = crop.h;
                        SDL_BlitSurface(pimpl->sprite_atlas, &crop, temp, &dest0);
                        SDL_Rect dest_final = dst;
                        SDL_BlitSurface(temp, nullptr, pimpl->screen, &dest_final);
                        SDL_FreeSurface(temp);
                    } else {
                        SDL_Rect crop = src;
                        int crop_w = static_cast<int>(crop.w);
                        int crop_h = static_cast<int>(crop.h);
                        if (crop_w > w) { crop.x += (crop_w - w) / 2; crop_w = std::min(crop_w, w); crop.w = static_cast<Uint16>(crop_w); }
                        if (crop_h > h) { crop.y += (crop_h - h) / 2; crop_h = std::min(crop_h, h); crop.h = static_cast<Uint16>(crop_h); }
                        SDL_Rect dest;
                        dest.x = static_cast<Sint16>(x);
                        dest.y = static_cast<Sint16>(y);
                        dest.w = static_cast<Uint16>(crop.w);
                        dest.h = static_cast<Uint16>(crop.h);
                        SDL_BlitSurface(pimpl->sprite_atlas, &crop, pimpl->screen, &dest);
                    }
                }
            } else {
                if (rel == 0) draw_filled_rect(pimpl->screen, x, y, w, h, 100, 100, 140);
                else draw_filled_rect(pimpl->screen, x, y, w, h, 70, 70, 90);
            }
        } else {
            if (art) {
                if (art->w == w && art->h == h) {
                    SDL_Rect dst;
                    dst.x = static_cast<Sint16>(x);
                    dst.y = static_cast<Sint16>(y);
                    dst.w = static_cast<Uint16>(w);
                    dst.h = static_cast<Uint16>(h);
                    SDL_BlitSurface(art, nullptr, pimpl->screen, &dst);
                } else {
                    int cx = std::max(0, (art->w - w) / 2);
                    int cy = std::max(0, (art->h - h) / 2);
                    int use_w = std::min(art->w, w);
                    int use_h = std::min(art->h, h);
                    SDL_Rect crop;
                    crop.x = static_cast<Sint16>(cx);
                    crop.y = static_cast<Sint16>(cy);
                    crop.w = static_cast<Uint16>(use_w);
                    crop.h = static_cast<Uint16>(use_h);
                    SDL_Rect dest;
                    dest.x = static_cast<Sint16>(x);
                    dest.y = static_cast<Sint16>(y);
                    dest.w = static_cast<Uint16>(crop.w);
                    dest.h = static_cast<Uint16>(crop.h);
                    SDL_BlitSurface(art, &crop, pimpl->screen, &dest);
                }
            } else {
                if (rel == 0) draw_filled_rect(pimpl->screen, x, y, w, h, 100, 100, 140);
                else draw_filled_rect(pimpl->screen, x, y, w, h, 70, 70, 90);
            }
        }

        // draw title
        std::string label = games[i].name.empty() ? basename_from_path(games[i].path) : games[i].name;
        if (rel == 0) draw_text(center_x - 200, center_y + active_h / 2 + 8, label, true);
        else draw_text(x + 8, y + 8, label, false);
    }
}
