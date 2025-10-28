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

namespace fs = std::filesystem;

static const SDL_Color COLOR_TEXT = {240,240,240,255};
static const SDL_Color COLOR_WHITE = {255,255,255,255};
static const SDL_Color COLOR_ORANGE = {255,140,0,255};
static const int BOX_W = 200;
static const int BOX_H = 260;

SliderUI::SliderUI(SDL_Renderer* rdr,
                   const std::string& icons_dir_,
                   const std::string& base_cache_dir_,
                   const std::string& dat_cache_file_,
                   const std::string& refl_cache_dir_,
                   int lazy_radius,
                   bool boxart_transparency)
: renderer(rdr), fontBig(nullptr), fontSmall(nullptr), selectedIndex(0),
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
    std::string font_path = icons_dir + "/../fonts/default.ttf"; // Also fixing issue #2
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

    if (!loadGamesList(slider_games_path, datCache)) return false;

    // initial lazy load window centered at index 0
    loadAssetsAround(selectedIndex);

    // save dat cache if new entries were parsed
    datCache.save();

    return true;
}

bool SliderUI::loadGamesList(const std::string& slider_games_path, DatCache& datCache) {
    std::ifstream f(slider_games_path);
    if (!f.is_open()) {
        std::cerr << "Failed to open slider games list: " << slider_games_path << "\n";
        return false;
    }
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

        // Use passed dat_cache to populate displayName and year if possible
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

    // load boxart texture
    if (!g.boxartPath.empty()) {
        SDL_Surface* s = IMG_Load(g.boxartPath.c_str());
        if (s) {
            g.boxart = SDL_CreateTextureFromSurface(renderer, s);
            SDL_FreeSurface(s);
        }
        // reflection (uses cache)
        g.reflection = loadOrGenerateReflection(renderer, g.boxartPath, refl_cache_dir);
    }

    // system icon
    if (!g.systemIconPath.empty()) {
        SDL_Surface* s2 = IMG_Load(g.systemIconPath.c_str());
        if (s2) {
            g.systemIcon = SDL_CreateTextureFromSurface(renderer, s2);
            SDL_FreeSurface(s2);
        }
    }

    g.assetsLoaded = true;
}

void SliderUI::unloadGameAssets(GameEntry& g) {
    if (!g.assetsLoaded) return;
    if (g.boxart) { SDL_DestroyTexture(g.boxart); g.boxart = nullptr; }
    if (g.reflection) { SDL_DestroyTexture(g.reflection); g.reflection = nullptr; }
    if (g.systemIcon) { SDL_DestroyTexture(g.systemIcon); g.systemIcon = nullptr; }
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

void SliderUI::run() {
    bool running = true;
    const int FPS = 30;
    const int frameDelay = 1000 / FPS;

    for (int i=0;i<SDL_NumJoysticks();++i) {
        if (SDL_IsGameController(i)) SDL_GameControllerOpen(i);
        else SDL_JoystickOpen(i);
    }

    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            bool konami = handleEvent(e);
            if (konami) {
                // Konami code detected - handleEvent already called exit(42)
                // This line won't be reached, but for clarity:
                running = false;
                break;
            }
        }
        draw();
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) SDL_Delay(frameDelay - frameTime);
    }
}

void SliderUI::drawTextLeft(const std::string &txt, TTF_Font* font, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, txt.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_FreeSurface(surf);
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}
void SliderUI::drawTextCentered(const std::string &txt, TTF_Font* font, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, txt.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = { x - surf->w/2, y - surf->h/2, surf->w, surf->h };
    SDL_FreeSurface(surf);
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

void SliderUI::draw() {
    SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255);
    SDL_RenderClear(renderer);

    if (games.empty()) {
        drawTextCentered("No games found", fontBig, 320, 240, COLOR_TEXT);
        SDL_RenderPresent(renderer);
        return;
    }

    // header
    int header_h = 56;
    const GameEntry &cur = games[selectedIndex];
    std::string title = cur.displayName.empty() ? cur.rawGameFile : cur.displayName;
    drawTextLeft(title, fontBig, 16, 12, COLOR_TEXT);

    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_Rect hdr_line = { 8, header_h - 2, 640 - 16, 2 };
    SDL_RenderFillRect(renderer, &hdr_line);

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
            SDL_Rect ph = { x - draw_w/2, center_y - draw_h/2, draw_w, draw_h };
            SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
            SDL_RenderFillRect(renderer, &ph);
        } else {
            SDL_Rect dst = { x - draw_w/2, center_y - draw_h/2, draw_w, draw_h };
            SDL_SetTextureBlendMode(games[i].boxart, SDL_BLENDMODE_BLEND);
            if (boxartTransparency) {
                float alpha = (i==selectedIndex) ? 1.0f : 0.5f;
                SDL_SetTextureAlphaMod(games[i].boxart, (Uint8)(alpha * 255));
            } else {
                SDL_SetTextureAlphaMod(games[i].boxart, 255);
            }
            SDL_RenderCopy(renderer, games[i].boxart, nullptr, &dst);

            if (games[i].reflection) {
                SDL_Rect rdst = { dst.x, dst.y + dst.h + 6, dst.w, dst.h/2 };
                SDL_SetTextureBlendMode(games[i].reflection, SDL_BLENDMODE_BLEND);
                SDL_RenderCopy(renderer, games[i].reflection, nullptr, &rdst);
            }
        }
    }

    // orange outline around selected
    int sel_w = int(BOX_W * 1.05f);
    int sel_h = int(BOX_H * 1.05f);
    SDL_Rect selRect = { center_x - sel_w/2, center_y - sel_h/2, sel_w, sel_h };
    SDL_SetRenderDrawColor(renderer, COLOR_ORANGE.r, COLOR_ORANGE.g, COLOR_ORANGE.b, 255);
    for (int i=0;i<3;i++) {
        SDL_Rect r = { selRect.x - i, selRect.y - i, selRect.w + i*2, selRect.h + i*2 };
        SDL_RenderDrawRect(renderer, &r);
    }

    // separator
    int sep_y = center_y + sel_h/2 + 10;
    SDL_Rect sep = { 8, sep_y, 640 - 16, 2 };
    SDL_SetRenderDrawColor(renderer, 255,255,255,255);
    SDL_RenderFillRect(renderer, &sep);

    // footer left: system icon + text
    int footer_y = sep_y + 18;
    SDL_Rect sysIconDst = { 18, footer_y, 40, 24 };
    if (cur.systemIcon) SDL_RenderCopy(renderer, cur.systemIcon, nullptr, &sysIconDst);
    std::string sysText = cur.systemFolder;
    if (cur.year) sysText += " - " + std::to_string(cur.year);
    drawTextLeft(sysText, fontSmall, 18 + 48, footer_y, COLOR_TEXT);

    // footer right: A OPEN
    int btnW = 56; int btnX = 640 - btnW - 18; int btnY = footer_y - 6;
    SDL_Rect rect = { btnX, btnY, btnW, btnW };
    SDL_SetRenderDrawColor(renderer, 80,80,80,255);
    SDL_RenderFillRect(renderer, &rect);
    drawTextCentered("A", fontBig, btnX + btnW/2, btnY + btnW/2 - 6, COLOR_TEXT);
    drawTextLeft("OPEN", fontSmall, btnX - 72, btnY + 16, COLOR_WHITE);

    SDL_RenderPresent(renderer);
}

// mapping to KonamiAction
static SliderUI::KonamiAction toKonami(const SDL_Event &e) {
    if (e.type == SDL_KEYDOWN) {
        SDL_Scancode sc = e.key.keysym.scancode;
        if (sc == SDL_SCANCODE_UP) return SliderUI::KA_UP;
        if (sc == SDL_SCANCODE_DOWN) return SliderUI::KA_DOWN;
        if (sc == SDL_SCANCODE_LEFT) return SliderUI::KA_LEFT;
        if (sc == SDL_SCANCODE_RIGHT) return SliderUI::KA_RIGHT;
        if (sc == SDL_SCANCODE_A || sc == SDL_SCANCODE_RETURN) return SliderUI::KA_A;
        if (sc == SDL_SCANCODE_B || sc == SDL_SCANCODE_ESCAPE) return SliderUI::KA_B;
    } else if (e.type == SDL_JOYHATMOTION) {
        if (e.jhat.value & SDL_HAT_UP) return SliderUI::KA_UP;
        if (e.jhat.value & SDL_HAT_DOWN) return SliderUI::KA_DOWN;
        if (e.jhat.value & SDL_HAT_LEFT) return SliderUI::KA_LEFT;
        if (e.jhat.value & SDL_HAT_RIGHT) return SliderUI::KA_RIGHT;
    } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        switch(e.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP: return SliderUI::KA_UP;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return SliderUI::KA_DOWN;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return SliderUI::KA_LEFT;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return SliderUI::KA_RIGHT;
            case SDL_CONTROLLER_BUTTON_A: return SliderUI::KA_A;
            case SDL_CONTROLLER_BUTTON_B: return SliderUI::KA_B;
            default: break;
        }
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
                // Exit with code 42 to signal Konami unlock to launcher script
                exit(42);
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
        } else if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_RETURN) {
            launchROM(games[selectedIndex].romPath);
        } else if (e.key.keysym.sym == SDLK_b || e.key.keysym.sym == SDLK_ESCAPE) {
            pid_t pid = fork();
            if (pid == 0) _exit(0);
        }
    } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
            selectedIndex = (selectedIndex + 1) % games.size();
            loadAssetsAround(selectedIndex);
        } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
            selectedIndex = (selectedIndex - 1 + games.size()) % games.size();
            loadAssetsAround(selectedIndex);
        } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            launchROM(games[selectedIndex].romPath);
        } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
            pid_t pid = fork(); if (pid==0) _exit(0);
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
        int status; waitpid(pid, &status, 0);
    }
}

void SliderUI::showUnlockAnimation() {
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);
    drawTextCentered("✨ KONAMI UNLOCKED! ✨", fontBig, 320, 220, COLOR_ORANGE);
    drawTextCentered("Returning to full menu...", fontSmall, 320, 260, COLOR_TEXT);
    SDL_RenderPresent(renderer);
    SDL_Delay(900);
}
