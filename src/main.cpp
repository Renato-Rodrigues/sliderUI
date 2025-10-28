#include "slider.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <fstream>
#include <iostream>
#include <string>
#include <map>

static std::map<std::string, std::string> readConfig(const std::string& path) {
    std::map<std::string, std::string> out;
    std::ifstream in(path);
    if (!in.is_open()) return out;
    std::string line;
    while (std::getline(in, line)) {
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == std::string::npos) continue;
        if (line[s] == '#') continue;
        size_t eq = line.find('=', s);
        if (eq == std::string::npos) continue;
        std::string k = line.substr(s, eq - s);
        std::string v = line.substr(eq + 1);
        auto trim = [](std::string &z){
            size_t a = z.find_first_not_of(" \t\r\n");
            size_t b = z.find_last_not_of(" \t\r\n");
            if (a==std::string::npos) { z = ""; return; }
            z = z.substr(a, b - a + 1);
        };
        trim(k); trim(v);
        if (!k.empty()) out[k] = v;
    }
    return out;
}

int main(int argc, char** argv) {
    const std::string games_list = "/mnt/SDCARD/Roms/sliderUI_games.txt";
    const std::string icons_dir = "/mnt/SDCARD/App/sliderUI/assets/icons";
    const std::string base_cache_dir = "/mnt/SDCARD/App/sliderUI/cache";
    const std::string dat_cache_file = "/mnt/SDCARD/App/sliderUI/dat_cache.txt";
    const std::string refl_cache_dir = "/mnt/SDCARD/App/sliderUI/cache/reflections";
    const std::string config_path = "/mnt/SDCARD/Roms/sliderUI.cfg";

    // defaults
    int lazy_radius = 2;
    bool boxart_transparency = true;

    auto cfg = readConfig(config_path);
    if (cfg.count("lazy_radius")) {
        try { lazy_radius = std::stoi(cfg["lazy_radius"]); }
        catch(...) { lazy_radius = 2; }
        if (lazy_radius < 0) lazy_radius = 0;
    }
    if (cfg.count("boxart_transparency")) {
        std::string v = cfg["boxart_transparency"];
        if (v == "0" || v == "false" || v == "False") boxart_transparency = false;
        else boxart_transparency = true;
    }

    // Initialize SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
        return 1;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        std::cerr << "IMG_Init error: " << IMG_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init error: " << TTF_GetError() << "\n";
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create window
    const int SCREEN_W = 640, SCREEN_H = 480;
    SDL_Window* window = SDL_CreateWindow(
        "sliderUI",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create renderer (try hardware accelerated first, fallback to software)
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "Hardware renderer failed, trying software...\n";
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed (both HW and SW): " << SDL_GetError() << "\n";
            SDL_DestroyWindow(window);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
    }

    // Initialize SliderUI
    SliderUI ui(renderer, icons_dir, base_cache_dir, dat_cache_file, refl_cache_dir, lazy_radius, boxart_transparency);
    if (!ui.init(games_list)) {
        std::cerr << "sliderUI init failed\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Run the UI
    ui.run();

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;

    SliderUI ui(renderer, icons_dir, base_cache_dir, dat_cache_file, refl_cache_dir, lazy_radius, boxart_transparency);
    if (!ui.init(games_list)) {
        std::cerr << "sliderUI init failed\n";
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    ui.run();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}