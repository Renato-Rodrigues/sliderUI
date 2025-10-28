#ifndef SLIDER_HPP
#define SLIDER_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

    SDL_Texture* boxart = nullptr;
    SDL_Texture* reflection = nullptr;
    SDL_Texture* systemIcon = nullptr;

    bool assetsLoaded = false;
};

class SliderUI {
public:
    SliderUI(SDL_Renderer* renderer,
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
    SDL_Renderer* renderer;
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

    bool loadGamesList(const std::string& slider_games_path, DatCache& datCache);
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