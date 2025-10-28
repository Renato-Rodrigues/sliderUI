/* slider_installer.cpp - small SDL-based installer that runs from MinUI Apps */
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

static const int W = 640, H = 480;
static const SDL_Color CLR_BG = {18,18,22,255};
static const SDL_Color CLR_TEXT = {230,230,230,255};
static const SDL_Color CLR_BTN = {60,60,70,255};
static const SDL_Color CLR_BTN_H = {100,100,120,255};

struct Button { SDL_Rect r; std::string label; };

static void drawText(SDL_Renderer* ren, TTF_Font* f, const std::string& txt, int x, int y, SDL_Color c) {
    if (!f) return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt.c_str(), c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(ren, s);
    SDL_Rect dst = { x, y, s->w, s->h };
    SDL_FreeSurface(s);
    SDL_RenderCopy(ren, t, NULL, &dst);
    SDL_DestroyTexture(t);
}

static void drawCenteredText(SDL_Renderer* ren, TTF_Font* f, const std::string& txt, int cx, int y, SDL_Color c) {
    if (!f) return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt.c_str(), c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(ren, s);
    SDL_Rect dst = { cx - s->w/2, y, s->w, s->h };
    SDL_FreeSurface(s);
    SDL_RenderCopy(ren, t, NULL, &dst);
    SDL_DestroyTexture(t);
}

static void drawButton(SDL_Renderer* ren, TTF_Font* f, const Button& b, bool hover) {
    SDL_SetRenderDrawColor(ren, hover ? CLR_BTN_H.r : CLR_BTN.r, hover ? CLR_BTN_H.g : CLR_BTN.g, hover ? CLR_BTN_H.b : CLR_BTN.b, 255);
    SDL_RenderFillRect(ren, &b.r);
    if (f) {
        drawCenteredText(ren, f, b.label, b.r.x + b.r.w/2, b.r.y + (b.r.h/2 - 10), CLR_TEXT);
    }
}

static bool copy_file(const fs::path& src, const fs::path& dst, bool make_executable = true) {
    try {
        fs::create_directories(dst.parent_path());
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
        if (make_executable) {
            if (chmod(dst.c_str(), 0755) != 0) {
                std::cerr << "Warning: chmod failed for " << dst << "\n";
                return false;
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Copy failed: " << e.what() << "\n";
        return false;
    }
}

static bool safe_remove_dir(const fs::path& p) {
    try {
        if (fs::exists(p)) fs::remove_all(p);
        return true;
    } catch (...) { return false; }
}

int main(int argc, char** argv) {
    std::string installerDir;
    char buff[4096];
    ssize_t len = readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if (len != -1) { buff[len] = 0; installerDir = fs::path(buff).parent_path().string(); }
    else installerDir = ".";

    const fs::path BIN_SRC = fs::path(installerDir) / "../build/sliderUI";
    const fs::path APP_DST = "/mnt/SDCARD/App/sliderUI";
    const fs::path ICON_SRC = fs::path(installerDir) / "../assets/icons";
    const fs::path CONFIG_SRC = fs::path(installerDir) / "../config/sliderUI.cfg";
    const fs::path KIDS_LIST_SRC = fs::path(installerDir) / "../data/sliderUI_games.txt";
    const fs::path AUTORUN_SRC = fs::path(installerDir) / "../tools/launch_sliderUI.sh";
    const fs::path AUTORUN_DST_DIR = "/mnt/SDCARD/.minui/autorun";
    const fs::path AUTORUN_DST = AUTORUN_DST_DIR / "launch_sliderUI.sh";

    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    TTF_Init();

    SDL_Window* win = SDL_CreateWindow("SliderUI Installer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    TTF_Font* font = TTF_OpenFont((installerDir + "/assets/fonts/default.ttf").c_str(), 18);
    if (!font) font = nullptr;

    std::vector<Button> buttons;
    int bw = 420, bh = 48;
    int bx = (W - bw)/2;
    int by0 = 140;
    buttons.push_back({{bx, by0, bw, bh}, "Install / Update SliderUI"});
    buttons.push_back({{bx, by0 + 60, bw, bh}, "Enable Auto-boot (Kids Mode)"});
    buttons.push_back({{bx, by0 + 120, bw, bh}, "Disable Auto-boot"});
    buttons.push_back({{bx, by0 + 180, bw, bh}, "Uninstall SliderUI"});
    buttons.push_back({{bx, by0 + 240, bw, bh}, "Exit Installer"});

    std::string status = "Ready";
    bool running = true;
    while (running) {
        SDL_Event e;
        int mx = -1, my = -1;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_MOUSEMOTION) { mx = e.motion.x; my = e.motion.y; }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int x = e.button.x, y = e.button.y;
                for (size_t i=0;i<buttons.size();++i) {
                    if (SDL_PointInRect(&SDL_Point{x,y}, &buttons[i].r)) {
                        if (i == 0) { // Install
                            status = "Installing...";
                            SDL_SetRenderDrawColor(ren, CLR_BG.r, CLR_BG.g, CLR_BG.b, 255);
                            SDL_RenderClear(ren);
                            drawCenteredText(ren, font, "Installing SliderUI...", W/2, 40, CLR_TEXT);
                            SDL_RenderPresent(ren);

                            bool ok = true;
                            
                            // Fixed: Use correct binary name (no .elf extension)
                            const fs::path CORRECT_BIN_SRC = fs::path(installerDir) / "build/sliderUI";
                            
                            if (!fs::exists(CORRECT_BIN_SRC)) {
                                status = "Error: build/sliderUI not found at " + CORRECT_BIN_SRC.string();
                                ok = false;
                            } else {
                                ok &= copy_file(CORRECT_BIN_SRC, APP_DST / "sliderUI", true);
                                
                                // Double-check executability was set
                                if (ok) {
                                    struct stat st;
                                    if (stat((APP_DST / "sliderUI").c_str(), &st) == 0) {
                                        if (!(st.st_mode & S_IXUSR)) {
                                            std::cerr << "Binary not executable, forcing chmod...\n";
                                            chmod((APP_DST / "sliderUI").c_str(), 0755);
                                        }
                                    }
                                }
                            }
                            try {
                                if (fs::exists(ICON_SRC)) {
                                    fs::create_directories(APP_DST / "assets" / "icons");
                                    for (auto &p : fs::directory_iterator(ICON_SRC)) {
                                        if (!p.is_regular_file()) continue;
                                        copy_file(p.path(), APP_DST / "assets" / "icons" / p.path().filename());
                                    }
                                }
                            } catch(...) {}
                            if (fs::exists(CONFIG_SRC) && !fs::exists("/mnt/SDCARD/Roms/sliderUI.cfg")) copy_file(CONFIG_SRC, "/mnt/SDCARD/Roms/sliderUI.cfg");
                            if (fs::exists(KIDS_LIST_SRC) && !fs::exists("/mnt/SDCARD/Roms/sliderUI_games.txt")) copy_file(KIDS_LIST_SRC, "/mnt/SDCARD/Roms/sliderUI_games.txt");
                            fs::create_directories(APP_DST / "cache" / "reflections");
                            {
                                fs::create_directories(APP_DST);
                                std::ofstream m((APP_DST / "metadata.txt").string());
                                m << "title=Slider Mode\n";
                                m << "description=Kid-friendly slider UI\n";
                                m << "exec=/mnt/SDCARD/App/sliderUI/sliderUI\n";
                                m.close();
                            }
                            if (ok) status = "Install complete.";
                            else status = "Install failed (check logs).";
                        } else if (i == 1) { // enable autorun
                            status = "Installing autorun...";
                            fs::create_directories(AUTORUN_DST_DIR);
                            if (fs::exists(AUTORUN_SRC)) {
                                bool ok = copy_file(AUTORUN_SRC, AUTORUN_DST);
                                if (ok) status = "Autorun launcher installed.";
                                else status = "Failed to install autorun.";
                            } else status = "launcher script not found.";
                            if (fs::exists("/mnt/SDCARD/Roms/sliderUI.cfg")) {
                                std::string cfg = "/mnt/SDCARD/Roms/sliderUI.cfg";
                                std::ifstream in(cfg);
                                std::string content;
                                bool found = false;
                                std::string line;
                                while (std::getline(in, line)) {
                                    if (line.find("autorun_enabled") != std::string::npos) { content += "autorun_enabled=true\n"; found = true; }
                                    else content += line + "\n";
                                }
                                in.close();
                                if (!found) content += "autorun_enabled=true\n";
                                std::ofstream out(cfg);
                                out << content; out.close();
                            }
                        } else if (i == 2) { // disable autorun
                            status = "Disabling autorun...";
                            if (fs::exists(AUTORUN_DST)) {
                                try { fs::rename(AUTORUN_DST, AUTORUN_DST.string() + ".disabled"); status = "Autorun disabled."; } catch(...) { status = "Failed to disable autorun."; }
                            } else status = "Autorun launcher not present.";
                            if (fs::exists("/mnt/SDCARD/Roms/sliderUI.cfg")) {
                                std::string cfg = "/mnt/SDCARD/Roms/sliderUI.cfg";
                                std::ifstream in(cfg);
                                std::string content; std::string line; bool found=false;
                                while (std::getline(in, line)) {
                                    if (line.find("autorun_enabled") != std::string::npos) { content += "autorun_enabled=false\n"; found = true; }
                                    else content += line + "\n";
                                }
                                in.close();
                                if (!found) content += "autorun_enabled=false\n";
                                std::ofstream out(cfg); out << content; out.close();
                            }
                        } else if (i == 3) { // uninstall
                            if (status.find("confirm uninstall") == std::string::npos) status = "Click Uninstall again to confirm.";
                            else {
                                bool ok = safe_remove_dir(APP_DST);
                                if (ok) status = "Uninstalled (app directory removed).";
                                else status = "Uninstall failed.";
                            }
                        } else if (i == 4) { // exit
                            running = false;
                        }
                    }
                }
            }
        }
        SDL_SetRenderDrawColor(ren, CLR_BG.r, CLR_BG.g, CLR_BG.b, 255);
        SDL_RenderClear(ren);
        drawCenteredText(ren, font, "SliderUI Installer", W/2, 20, CLR_TEXT);
        drawCenteredText(ren, font, "Install and manage SliderUI from MinUI menu", W/2, 52, CLR_TEXT);

        int mxpos = -1, mypos = -1;
        int mxs, mys;
        SDL_GetMouseState(&mxs, &mys);
        mxpos = mxs; mypos = mys;

        for (auto &b : buttons) {
            bool hover = (mxpos >= b.r.x && mxpos <= b.r.x + b.r.w && mypos >= b.r.y && mypos <= b.r.y + b.r.h);
            drawButton(ren, font, b, hover);
        }

        SDL_Rect st = { (W - 560)/2, H - 80, 560, 48 };
        SDL_SetRenderDrawColor(ren, 30,30,36,255);
        SDL_RenderFillRect(ren, &st);
        if (font) drawText(ren, font, status, st.x + 12, st.y + 12, CLR_TEXT);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
