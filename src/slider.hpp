#ifndef SLIDER_HPP
#define SLIDER_HPP

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

struct GameEntry {
    std::string displayName;
    std::string rawGameFile;
    std::string systemFolder;
    std::string systemCode;
    int year = 0;

    std::string romPath;
    std::string boxartPath;
    std::string systemIconPath;

    SDL_Surface* boxart = nullptr;
    SDL_Surface* reflection = nullptr;
    SDL_Surface* systemIcon = nullptr;

    bool assetsLoaded = false;
};

class SliderUI {
public:
    SliderUI(SDL_Surface* screen,
             const std::string& icons_dir,
             const std::string& base_cache_dir,
             const std::string& dat_cache_file,
             const std::string& refl_cache_dir,
             int lazy_radius,
             bool boxart_transparency);

    ~SliderUI();

    bool init(const std::string& slider_games_path);
    void run();

private:
    SDL_Surface* screen;
    TTF_Font* fontBig;
    TTF_Font* fontSmall;

    std::vector<GameEntry> games;
    int selectedIndex;

    std::string icons_dir;
    std::string base_cache_dir;
    std::string dat_cache_file;
    std::string refl_cache_dir;

    int lazyRadius;
    bool boxartTransparency;

    bool loadGamesList(const std::string& slider_games_path);
    void loadAssetsAround(int index);
    void unloadAssetsOutside(int index);
    void loadGameAssets(GameEntry& g);
    void unloadGameAssets(GameEntry& g);
    void draw();
    void drawTextLeft(const std::string& txt, TTF_Font* font, int x, int y, SDL_Color color);
    void drawTextCentered(const std::string& txt, TTF_Font* font, int x, int y, SDL_Color color);
    void launchROM(const std::string& rom);
    bool handleEvent(SDL_Event& e);
    void showUnlockAnimation();
    
    // Helper for blitting with alpha
    void blitSurfaceAlpha(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect, Uint8 alpha);
    SDL_Surface* scaleSurface(SDL_Surface* src, int newW, int newH);

    enum KonamiAction { KA_UP, KA_DOWN, KA_LEFT, KA_RIGHT, KA_B, KA_A, KA_NONE };
    std::vector<KonamiAction> konamiSeq;
    int konamiIndex;
    Uint32 lastKonamiTime;
    const Uint32 KONAMI_TIMEOUT_MS = 3000;
    const Uint32 KONAMI_DEBOUNCE_MS = 80;
    Uint32 lastInputTime;

    SliderUI(const SliderUI&) = delete;
    SliderUI& operator=(const SliderUI&) = delete;
};

#endif // SLIDER_HPP