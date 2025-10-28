#include "slider.hpp"
#include "reflection_cache.hpp"
#include "dat_cache.h"
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>

namespace fs = std::filesystem;

static const SDL_Color COLOR_TEXT = {240,240,240,0};
static const SDL_Color COLOR_WHITE = {255,255,255,0};
static const SDL_Color COLOR_ORANGE = {255,140,0,0};
static const int BOX_W = 200;
static const int BOX_H = 260;

SliderUI::SliderUI(SDL_Surface* scr,
                   const std::string& icons_dir_,
                   const std::string& base_cache_dir_,
                   const std::string& dat_cache_file_,
                   const std::string& refl_cache_dir_,
                   int lazy_radius,
                   bool boxart_transparency)
: screen(scr), fontBig(nullptr), fontSmall(nullptr), selectedIndex(0),
  icons_dir(icons_dir_), base_cache_dir(base_cache_dir_),
  dat_cache_file(dat_cache_file_), refl_cache_dir(refl_cache_dir_),
  lazyRadius(lazy_radius), boxartTransparency(boxart_transparency),
  konamiIndex(0), lastKonamiTime(0), lastInputTime(0)
{
    konamiSeq = { KA_UP, KA_UP, KA_DOWN, KA_DOWN, KA_LEFT, KA_RIGHT, KA_LEFT, KA_RIGHT, KA_B, KA_A };
}

SliderUI::~SliderUI() {
    for (auto &g : games) unloadGameAssets(g);
    if (fontBig) TTF_CloseFont(fontBig);
    if (fontSmall) TTF_CloseFont(fontSmall);
}

bool SliderUI::init(const std::string& slider_games_path) {
    // fonts
    std::string font_path = icons_dir + "/../fonts/default.ttf";
    fontBig = TTF_OpenFont(font_path.c_str(), 28);
    fontSmall = TTF_OpenFont(font_path.c_str(), 18);
    
    if (!fontBig || !fontSmall) {
        std::cerr << "Failed to load fonts from: " << font_path << "\n";
        return false;
    }
    
    // create cache dirs
    fs::create_directories(base_cache_dir);
    fs::create_directories(refl_cache_dir);

    // load dat cache
    DatCache datCache(dat_cache_file);
    datCache.load();

    if (!loadGamesList(slider_games_path)) return false;

    // initial lazy load window centered at index 0
    loadAssetsAround(selectedIndex);

    // save dat cache if new entries were parsed
    datCache.save();

    return true;
}

bool SliderUI::loadGamesList(const std::string& slider_games_path) {
    std::ifstream f(slider_games_path);
    if (!f.is_open()) {
        std::cerr << "Failed to open slider games list: " << slider_games_path << "\n";
        return false;
    }
    
    DatCache datCache(dat_cache_file);
    datCache.load();
    
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;
        size_t sep = line.find(';');
        if (sep == std::string::npos) continue;
        GameEntry g;
        g.systemFolder = line.substr(0, sep);
        g.rawGameFile = line.substr(sep + 1);
        
        // extract system code
        size_t open = g.systemFolder.find('(');
        size_t close = g.systemFolder.find(')');
        if (open != std::string::npos && close != std::string::npos && close > open+1)
            g.systemCode = g.systemFolder.substr(open+1, close - open - 1);
        else g.systemCode = "UNK";
        g.romPath = "/mnt/SDCARD/Roms/" + g.systemFolder + "/" + g.rawGameFile;

        // Use dat_cache to populate displayName and year if possible
        std::string datName;
        size_t p = g.systemFolder.find(" (");
        if (p != std::string::npos) datName = g.systemFolder.substr(0,p);
        else datName = g.systemFolder;
        std::string datPath = "/mnt/SDCARD/Roms/" + g.systemFolder + "/" + datName + ".dat";
        DatMetadata meta = datCache.getMetadata(g.systemCode, fs::path(g.rawGameFile).stem().string(), datPath);
        if (!meta.description.empty()) g.displayName = meta.description;
        if (meta.year != 0) g.year = meta.year;

        games.push_back(g);
    }
    return !games.empty();
}

SDL_Surface* SliderUI::scaleSurface(SDL_Surface* src, int newW, int newH) {
    if (!src) return nullptr;
    
    SDL_Surface* scaled = SDL_CreateRGBSurface(0, newW, newH, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!scaled) return nullptr;
    
    // Simple nearest-neighbor scaling
    SDL_LockSurface(src);
    SDL_LockSurface(scaled);
    
    Uint32* srcPixels = (Uint32*)src->pixels;
    Uint32* dstPixels = (Uint32*)scaled->pixels;
    
    float xRatio = (float)src->w / newW;
    float yRatio = (float)src->h / newH;
    
    for (int y = 0; y < newH; y++) {
        for (int x = 0; x < newW; x++) {
            int srcX = (int)(x * xRatio);
            int srcY = (int)(y * yRatio);
            dstPixels[y * scaled->w + x] = srcPixels[srcY * src->w + srcX];
        }
    }
    
    SDL_UnlockSurface(scaled);
    SDL_UnlockSurface(src);
    
    return scaled;
}

void SliderUI::loadGameAssets(GameEntry& g) {
    if (g.assetsLoaded) return;
    
    // locate boxart
    std::string base = fs::path(g.rawGameFile).stem().string();
    std::string p1 = "/mnt/SDCARD/Roms/" + g.systemFolder + "/.res/" + base + ".png";
    std::string p2 = "/mnt/SDCARD/Roms/" + g.systemFolder + "/.res/" + base + ".jpg";
    if (fs::exists(p1)) g.boxartPath = p1;
    else if (fs::exists(p2)) g.boxartPath = p2;
    else g.boxartPath.clear();

    // system icon
    std::string ip = icons_dir + "/" + g.systemCode + ".png";
    std::string ij = icons_dir + "/" + g.systemCode + ".jpg";
    if (fs::exists(ip)) g.systemIconPath = ip;
    else if (fs::exists(ij)) g.systemIconPath = ij;
    else g.systemIconPath.clear();

    // load boxart surface
    if (!g.boxartPath.empty()) {
        g.boxart = IMG_Load(g.boxartPath.c_str());
        if (g.boxart) {
            // Convert to display format for faster blitting
            SDL_Surface* temp = SDL_DisplayFormatAlpha(g.boxart);
            if (temp) {
                SDL_FreeSurface(g.boxart);
                g.boxart = temp;
            }
        }
        // reflection (uses cache)
        g.reflection = loadOrGenerateReflection(g.boxartPath, refl_cache_dir);
    }

    // system icon
    if (!g.systemIconPath.empty()) {
        g.systemIcon = IMG_Load(g.systemIconPath.c_str());
        if (g.systemIcon) {
            SDL_Surface* temp = SDL_DisplayFormat(g.systemIcon);
            if (temp) {
                SDL_FreeSurface(g.systemIcon);
                g.systemIcon = temp;
            }
        }
    }

    g.assetsLoaded = true;
}

void SliderUI::unloadGameAssets(GameEntry& g) {
    if (!g.assetsLoaded) return;
    if (g.boxart) { SDL_FreeSurface(g.boxart); g.boxart = nullptr; }
    if (g.reflection) { SDL_FreeSurface(g.reflection); g.reflection = nullptr; }
    if (g.systemIcon) { SDL_FreeSurface(g.systemIcon); g.systemIcon = nullptr; }
    g.assetsLoaded = false;
}

void SliderUI::loadAssetsAround(int index) {
    if (games.empty()) return;
    int n = (int)games.size();
    int lo = std::max(0, index - lazyRadius);
    int hi = std::min(n-1, index + lazyRadius);
    
    // load inside window
    for (int i = lo; i <= hi; ++i) {
        if (!games[i].assetsLoaded) loadGameAssets(games[i]);
    }
    // unload outside window
    for (int i = 0; i < n; ++i) {
        if (i < lo || i > hi) {
            if (games[i].assetsLoaded) unloadGameAssets(games[i]);
        }
    }
}

void SliderUI::unloadAssetsOutside(int index) {
    loadAssetsAround(index);
}

void SliderUI::blitSurfaceAlpha(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect, Uint8 alpha) {
    if (!src || !dst) return;
    SDL_SetAlpha(src, SDL_SRCALPHA, alpha);
    SDL_BlitSurface(src, srcrect, dst, dstrect);
}

void SliderUI::run() {
    bool running = true;
    const int FPS = 30;
    const int frameDelay = 1000 / FPS;

    // Open joystick if available
    SDL_Joystick* joystick = nullptr;
    if (SDL_NumJoysticks() > 0) {
        joystick = SDL_JoystickOpen(0);
    }

    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            bool konami = handleEvent(e);
            if (konami) { running = false; break; }
        }
        draw();
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) SDL_Delay(frameDelay - frameTime);
    }
    
    if (joystick) SDL_JoystickClose(joystick);
}

void SliderUI::drawTextLeft(const std::string &txt, TTF_Font* font, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, txt.c_str(), color);
    if (!surf) return;
    SDL_Rect dst = { (Sint16)x, (Sint16)y, 0, 0 };
    SDL_BlitSurface(surf, nullptr, screen, &dst);
    SDL_FreeSurface(surf);
}

void SliderUI::drawTextCentered(const std::string &txt, TTF_Font* font, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, txt.c_str(), color);
    if (!surf) return;
    SDL_Rect dst = { (Sint16)(x - surf->w/2), (Sint16)(y - surf->h/2), 0, 0 };
    SDL_BlitSurface(surf, nullptr, screen, &dst);
    SDL_FreeSurface(surf);
}

void SliderUI::draw() {
    // Clear screen
    SDL_FillRect(screen, nullptr, SDL_MapRGB(screen->format, 15, 15, 20));

    if (games.empty()) {
        drawTextCentered("No games found", fontBig, 320, 240, COLOR_TEXT);
        SDL_Flip(screen);
        return;
    }

    // header
    int header_h = 56;
    const GameEntry &cur = games[selectedIndex];
    std::string title = cur.displayName.empty() ? cur.rawGameFile : cur.displayName;
    drawTextLeft(title, fontBig, 16, 12, COLOR_TEXT);

    SDL_Rect hdr_line = { 8, (Sint16)(header_h - 2), 640 - 16, 2 };
    SDL_FillRect(screen, &hdr_line, SDL_MapRGB(screen->format, 255, 255, 255));

    // center carousel
    int center_x = 640/2;
    int center_y = 480/2 - 24;
    int spacing = 240;
    
    for (int i = 0; i < (int)games.size(); ++i) {
        int rel = i - selectedIndex;
        int x = center_x + rel * spacing;
        float scale = (i == selectedIndex) ? 1.05f : 0.78f;
        int draw_w = int(BOX_W * scale);
        int draw_h = int(BOX_H * scale);

        if (!games[i].assetsLoaded || !games[i].boxart) {
            SDL_Rect ph = { (Sint16)(x - draw_w/2), (Sint16)(center_y - draw_h/2), (Uint16)draw_w, (Uint16)draw_h };
            SDL_FillRect(screen, &ph, SDL_MapRGB(screen->format, 50, 50, 60));
        } else {
            SDL_Surface* scaled = scaleSurface(games[i].boxart, draw_w, draw_h);
            if (scaled) {
                SDL_Rect dst = { (Sint16)(x - draw_w/2), (Sint16)(center_y - draw_h/2), 0, 0 };
                Uint8 alpha = (i == selectedIndex || !boxartTransparency) ? 255 : 128;
                blitSurfaceAlpha(scaled, nullptr, screen, &dst, alpha);
                
                // reflection
                if (games[i].reflection) {
                    SDL_Surface* scaledRefl = scaleSurface(games[i].reflection, draw_w, draw_h/2);
                    if (scaledRefl) {
                        SDL_Rect rdst = { (Sint16)(x - draw_w/2), (Sint16)(center_y + draw_h/2 + 6), 0, 0 };
                        blitSurfaceAlpha(scaledRefl, nullptr, screen, &rdst, 255);
                        SDL_FreeSurface(scaledRefl);
                    }
                }
                SDL_FreeSurface(scaled);
            }
        }
    }

    // orange outline around selected
    int sel_w = int(BOX_W * 1.05f);
    int sel_h = int(BOX_H * 1.05f);
    Uint32 orange = SDL_MapRGB(screen->format, COLOR_ORANGE.r, COLOR_ORANGE.g, COLOR_ORANGE.b);
    for (int i = 0; i < 3; i++) {
        SDL_Rect r = { 
            (Sint16)(center_x - sel_w/2 - i), 
            (Sint16)(center_y - sel_h/2 - i), 
            (Uint16)(sel_w + i*2), 
            (Uint16)(sel_h + i*2) 
        };
        // Draw rectangle outline
        SDL_Rect top = { r.x, r.y, r.w, 1 };
        SDL_Rect bottom = { r.x, (Sint16)(r.y + r.h - 1), r.w, 1 };
        SDL_Rect left = { r.x, r.y, 1, r.h };
        SDL_Rect right = { (Sint16)(r.x + r.w - 1), r.y, 1, r.h };
        SDL_FillRect(screen, &top, orange);
        SDL_FillRect(screen, &bottom, orange);
        SDL_FillRect(screen, &left, orange);
        SDL_FillRect(screen, &right, orange);
    }

    // separator
    int sep_y = center_y + sel_h/2 + 10;
    SDL_Rect sep = { 8, (Sint16)sep_y, 640 - 16, 2 };
    SDL_FillRect(screen, &sep, SDL_MapRGB(screen->format, 255, 255, 255));

    // footer left: system icon + text
    int footer_y = sep_y + 18;
    if (cur.systemIcon) {
        SDL_Rect sysIconDst = { 18, (Sint16)footer_y, 40, 24 };
        SDL_Surface* scaledIcon = scaleSurface(cur.systemIcon, 40, 24);
        if (scaledIcon) {
            SDL_BlitSurface(scaledIcon, nullptr, screen, &sysIconDst);
            SDL_FreeSurface(scaledIcon);
        }
    }
    std::string sysText = cur.systemFolder;
    if (cur.year) sysText += " - " + std::to_string(cur.year);
    drawTextLeft(sysText, fontSmall, 18 + 48, footer_y, COLOR_TEXT);

    // footer right: A OPEN
    int btnW = 56; int btnX = 640 - btnW - 18; int btnY = footer_y - 6;
    SDL_Rect rect = { (Sint16)btnX, (Sint16)btnY, (Uint16)btnW, (Uint16)btnW };
    SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 80, 80, 80));
    drawTextCentered("A", fontBig, btnX + btnW/2, btnY + btnW/2 - 6, COLOR_TEXT);
    drawTextLeft("OPEN", fontSmall, btnX - 72, btnY + 16, COLOR_WHITE);

    SDL_Flip(screen);
}

static SliderUI::KonamiAction toKonami(const SDL_Event &e) {
    if (e.type == SDL_KEYDOWN) {
        SDLKey key = e.key.keysym.sym;
        if (key == SDLK_UP) return SliderUI::KA_UP;
        if (key == SDLK_DOWN) return SliderUI::KA_DOWN;
        if (key == SDLK_LEFT) return SliderUI::KA_LEFT;
        if (key == SDLK_RIGHT) return SliderUI::KA_RIGHT;
        if (key == SDLK_a || key == SDLK_RETURN || key == SDLK_LCTRL) return SliderUI::KA_A;
        if (key == SDLK_b || key == SDLK_ESCAPE || key == SDLK_LALT) return SliderUI::KA_B;
    } else if (e.type == SDL_JOYHATMOTION) {
        if (e.jhat.value & SDL_HAT_UP) return SliderUI::KA_UP;
        if (e.jhat.value & SDL_HAT_DOWN) return SliderUI::KA_DOWN;
        if (e.jhat.value & SDL_HAT_LEFT) return SliderUI::KA_LEFT;
        if (e.jhat.value & SDL_HAT_RIGHT) return SliderUI::KA_RIGHT;
    } else if (e.type == SDL_JOYBUTTONDOWN) {
        // MinUI button mapping: A=0, B=1, X=2, Y=3
        if (e.jbutton.button == 0) return SliderUI::KA_A; // A button
        if (e.jbutton.button == 1) return SliderUI::KA_B; // B button
    }
    return SliderUI::KA_NONE;
}

bool SliderUI::handleEvent(SDL_Event& e) {
    SliderUI::KonamiAction action = toKonami(e);
    Uint32 now = SDL_GetTicks();
    
    if (action != KA_NONE) {
        if (now - lastInputTime >= KONAMI_DEBOUNCE_MS) {
            lastInputTime = now;
            if (konamiIndex > 0 && (now - lastKonamiTime > KONAMI_TIMEOUT_MS)) konamiIndex = 0;
            lastKonamiTime = now;
            if (action == konamiSeq[konamiIndex]) {
                konamiIndex++;
                if (konamiIndex >= (int)konamiSeq.size()) {
                    konamiIndex = 0;
                    showUnlockAnimation();
                    exit(42);
                }
            } else {
                konamiIndex = (action == konamiSeq[0]) ? 1 : 0;
            }
        }
    }

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_RIGHT) {
            selectedIndex = (selectedIndex + 1) % games.size();
            loadAssetsAround(selectedIndex);
        } else if (e.key.keysym.sym == SDLK_LEFT) {
            selectedIndex = (selectedIndex - 1 + games.size()) % games.size();
            loadAssetsAround(selectedIndex);
        } else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_LCTRL) {
            launchROM(games[selectedIndex].romPath);
        } else if (e.key.keysym.sym == SDLK_b || e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_LALT) {
            exit(0);
        }
    } else if (e.type == SDL_JOYBUTTONDOWN) {
        if (e.jbutton.button == 0) { // A button
            launchROM(games[selectedIndex].romPath);
        } else if (e.jbutton.button == 1) { // B button
            exit(0);
        }
    } else if (e.type == SDL_JOYHATMOTION) {
        if (e.jhat.value & SDL_HAT_RIGHT) {
            selectedIndex = (selectedIndex + 1) % games.size();
            loadAssetsAround(selectedIndex);
        } else if (e.jhat.value & SDL_HAT_LEFT) {
            selectedIndex = (selectedIndex - 1 + games.size()) % games.size();
            loadAssetsAround(selectedIndex);
        }
    }
    return false;
}

void SliderUI::launchROM(const std::string& rom) {
    if (rom.empty()) return;
    pid_t pid = fork();
    if (pid == 0) {
        execlp("/mnt/SDCARD/.minui/minui_launcher", "minui_launcher", rom.c_str(), (char*)NULL);
        execlp("minui_launcher", "minui_launcher", rom.c_str(), (char*)NULL);
        _exit(127);
    } else if (pid > 0) {
        int status; 
        waitpid(pid, &status, 0);
    }
}

void SliderUI::showUnlockAnimation() {
    SDL_FillRect(screen, nullptr, SDL_MapRGB(screen->format, 0, 0, 0));
    drawTextCentered("✨ KONAMI UNLOCKED! ✨", fontBig, 320, 220, COLOR_ORANGE);
    drawTextCentered("Returning to full menu...", fontSmall, 320, 260, COLOR_TEXT);
    SDL_Flip(screen);
    SDL_Delay(900);
}