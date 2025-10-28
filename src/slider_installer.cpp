/* slider_installer.cpp - small SDL-based installer that runs from MinUI Apps */
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
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
static const SDL_Color CLR_BG = {18,18,22,0};
static const SDL_Color CLR_TEXT = {230,230,230,0};
static const SDL_Color CLR_BTN = {60,60,70,0};
static const SDL_Color CLR_BTN_H = {100,100,120,0};

struct Button { SDL_Rect r; std::string label; };

static void drawText(SDL_Surface* screen, TTF_Font* f, const std::string& txt, int x, int y, SDL_Color c) {
    if (!f) return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt.c_str(), c);
    if (!s) return;
    SDL_Rect dst = { (Sint16)x, (Sint16)y, 0, 0 };
    SDL_BlitSurface(s, NULL, screen, &dst);
    SDL_FreeSurface(s);
}

static void drawCenteredText(SDL_Surface* screen, TTF_Font* f, const std::string& txt, int cx, int y, SDL_Color c) {
    if (!f) return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt.c_str(), c);
    if (!s) return;
    SDL_Rect dst = { (Sint16)(cx - s->w/2), (Sint16)y, 0, 0 };
    SDL_BlitSurface(s, NULL, screen, &dst);
    SDL_FreeSurface(s);
}

static void drawButton(SDL_Surface* screen, TTF_Font* f, const Button& b, bool hover) {
    Uint32 color = SDL_MapRGB(screen->format, 
        hover ? CLR_BTN_H.r : CLR_BTN.r, 
        hover ? CLR_BTN_H.g : CLR_BTN.g, 
        hover ? CLR_BTN_H.b : CLR_BTN.b);
    SDL_FillRect(screen, (SDL_Rect*)&b.r, color);
    
    if (f) {
        drawCenteredText(screen, f, b.label, b.r.x + b.r.w/2, b.r.y + (b.r.h/2 - 10), CLR_TEXT);
    }
}

static bool copy_file_custom(const fs::path& src, const fs::path& dst, bool make_executable = true) {
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
    if (len != -1) { 
        buff[len] = 0; 
        installerDir = fs::path(buff).parent_path().string(); 
    } else {
        installerDir = ".";
    }

    const fs::path BIN_SRC = fs::path(installerDir) / "build/sliderUI";
    const fs::path APP_DST = "/mnt/SDCARD/App/sliderUI";
    const fs::path ICON_SRC = fs::path(installerDir) / "assets/icons";
    const fs::path CONFIG_SRC = fs::path(installerDir) / "config/sliderUI.cfg";
    const fs::path KIDS_LIST_SRC = fs::path(installerDir) / "data/sliderUI_games.txt";
    const fs::path AUTORUN_SRC = fs::path(installerDir) / "tools/launch_sliderUI.sh";
    const fs::path AUTORUN_DST_DIR = "/mnt/SDCARD/.minui/autorun";
    const fs::path AUTORUN_DST = AUTORUN_DST_DIR / "launch_sliderUI.sh";

    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    TTF_Init();

    SDL_Surface* screen = SDL_SetVideoMode(W, H, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen) {
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_ShowCursor(SDL_DISABLE);
    
    TTF_Font* font = TTF_OpenFont((installerDir + "/assets/fonts/default.ttf").c_str(), 18);
    if (!font) font = nullptr;

    std::vector<Button> buttons;
    int bw = 420, bh = 48;
    int bx = (W - bw)/2;
    int by0 = 140;
    buttons.push_back({{(Sint16)bx, (Sint16)by0, (Uint16)bw, (Uint16)bh}, "Install / Update SliderUI"});
    buttons.push_back({{(Sint16)bx, (Sint16)(by0 + 60), (Uint16)bw, (Uint16)bh}, "Enable Auto-boot (Kids Mode)"});
    buttons.push_back({{(Sint16)bx, (Sint16)(by0 + 120), (Uint16)bw, (Uint16)bh}, "Disable Auto-boot"});
    buttons.push_back({{(Sint16)bx, (Sint16)(by0 + 180), (Uint16)bw, (Uint16)bh}, "Uninstall SliderUI"});
    buttons.push_back({{(Sint16)bx, (Sint16)(by0 + 240), (Uint16)bw, (Uint16)bh}, "Exit Installer"});

    std::string status = "Ready";
    bool running = true;
    int selectedBtn = 0;
    
    // Open joystick
    SDL_Joystick* joystick = nullptr;
    if (SDL_NumJoysticks() > 0) {
        joystick = SDL_JoystickOpen(0);
    }

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            
            // Keyboard navigation
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) {
                    selectedBtn = (selectedBtn - 1 + buttons.size()) % buttons.size();
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    selectedBtn = (selectedBtn + 1) % buttons.size();
                } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_LCTRL) {
                    // Execute button action
                    size_t i = selectedBtn;
                    
                    if (i == 0) { // Install
                        status = "Installing...";
                        SDL_FillRect(screen, nullptr, SDL_MapRGB(screen->format, CLR_BG.r, CLR_BG.g, CLR_BG.b));
                        drawCenteredText(screen, font, "Installing SliderUI...", W/2, 40, CLR_TEXT);
                        SDL_Flip(screen);

                        bool ok = true;
                        if (!fs::exists(BIN_SRC)) {
                            status = "Error: build/sliderUI not found.";
                            ok = false;
                        } else {
                            ok &= copy_file_custom(BIN_SRC, APP_DST / "sliderUI", true);
                            
                            // Double-check executability
                            if (ok) {
                                struct stat st;
                                if (stat((APP_DST / "sliderUI").c_str(), &st) == 0) {
                                    if (!(st.st_mode & S_IXUSR)) {
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
                        
                        if (fs::exists(CONFIG_SRC) && !fs::exists("/mnt/SDCARD/Roms/sliderUI.cfg")) {
                            copy_file(CONFIG_SRC, "/mnt/SDCARD/Roms/sliderUI.cfg");
                        }
                        if (fs::exists(KIDS_LIST_SRC) && !fs::exists("/mnt/SDCARD/Roms/sliderUI_games.txt")) {
                            copy_file(KIDS_LIST_SRC, "/mnt/SDCARD/Roms/sliderUI_games.txt");
                        }
                        
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
                            bool ok = copy_file_custom(AUTORUN_SRC, AUTORUN_DST);
                            if (ok) status = "Autorun launcher installed.";
                            else status = "Failed to install autorun.";
                        } else {
                            status = "launcher script not found.";
                        }
                        
                        if (fs::exists("/mnt/SDCARD/Roms/sliderUI.cfg")) {
                            std::string cfg = "/mnt/SDCARD/Roms/sliderUI.cfg";
                            std::ifstream in(cfg);
                            std::string content;
                            bool found = false;
                            std::string line;
                            while (std::getline(in, line)) {
                                if (line.find("autorun_enabled") != std::string::npos) {
                                    content += "autorun_enabled=true\n";
                                    found = true;
                                } else {
                                    content += line + "\n";
                                }
                            }
                            in.close();
                            if (!found) content += "autorun_enabled=true\n";
                            std::ofstream out(cfg);
                            out << content;
                            out.close();
                        }
                        
                    } else if (i == 2) { // disable autorun
                        status = "Disabling autorun...";
                        if (fs::exists(AUTORUN_DST)) {
                            try {
                                fs::rename(AUTORUN_DST, AUTORUN_DST.string() + ".disabled");
                                status = "Autorun disabled.";
                            } catch(...) {
                                status = "Failed to disable autorun.";
                            }
                        } else {
                            status = "Autorun launcher not present.";
                        }
                        
                        if (fs::exists("/mnt/SDCARD/Roms/sliderUI.cfg")) {
                            std::string cfg = "/mnt/SDCARD/Roms/sliderUI.cfg";
                            std::ifstream in(cfg);
                            std::string content;
                            std::string line;
                            bool found = false;
                            while (std::getline(in, line)) {
                                if (line.find("autorun_enabled") != std::string::npos) {
                                    content += "autorun_enabled=false\n";
                                    found = true;
                                } else {
                                    content += line + "\n";
                                }
                            }
                            in.close();
                            if (!found) content += "autorun_enabled=false\n";
                            std::ofstream out(cfg);
                            out << content;
                            out.close();
                        }
                        
                    } else if (i == 3) { // uninstall
                        if (status.find("confirm uninstall") == std::string::npos) {
                            status = "Click Uninstall again to confirm.";
                        } else {
                            bool ok = safe_remove_dir(APP_DST);
                            if (ok) status = "Uninstalled (app directory removed).";
                            else status = "Uninstall failed.";
                        }
                        
                    } else if (i == 4) { // exit
                        running = false;
                    }
                }
            }
            
            // Joystick navigation
            if (e.type == SDL_JOYHATMOTION) {
                if (e.jhat.value & SDL_HAT_UP) {
                    selectedBtn = (selectedBtn - 1 + buttons.size()) % buttons.size();
                } else if (e.jhat.value & SDL_HAT_DOWN) {
                    selectedBtn = (selectedBtn + 1) % buttons.size();
                }
            }
            
            if (e.type == SDL_JOYBUTTONDOWN && e.jbutton.button == 0) { // A button
                // Trigger same action as Enter key
                SDL_Event fakeEvent;
                fakeEvent.type = SDL_KEYDOWN;
                fakeEvent.key.keysym.sym = SDLK_RETURN;
                SDL_PushEvent(&fakeEvent);
            }
        }
        
        // Clear screen
        SDL_FillRect(screen, nullptr, SDL_MapRGB(screen->format, CLR_BG.r, CLR_BG.g, CLR_BG.b));
        
        // Draw title
        drawCenteredText(screen, font, "SliderUI Installer", W/2, 20, CLR_TEXT);
        drawCenteredText(screen, font, "Install and manage SliderUI from MinUI menu", W/2, 52, CLR_TEXT);

        // Draw buttons
        for (size_t i = 0; i < buttons.size(); ++i) {
            drawButton(screen, font, buttons[i], i == (size_t)selectedBtn);
        }

        // Draw status
        SDL_Rect st = { (Sint16)((W - 560)/2), (Sint16)(H - 80), 560, 48 };
        SDL_FillRect(screen, &st, SDL_MapRGB(screen->format, 30, 30, 36));
        if (font) drawText(screen, font, status, st.x + 12, st.y + 12, CLR_TEXT);

        SDL_Flip(screen);
        SDL_Delay(16);
    }

    if (joystick) SDL_JoystickClose(joystick);
    if (font) TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}