#include "reflection_cache.hpp"
#include "stb_image_write.h"
#include <SDL_image.h>
#include <filesystem>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

static SDL_Surface* convert_to_rgba32(SDL_Surface* s) {
    if (!s) return nullptr;
    SDL_Surface* conv = SDL_CreateRGBSurface(0, s->w, s->h, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    if (!conv) return nullptr;
    SDL_BlitSurface(s, nullptr, conv, nullptr);
    return conv;
}

static SDL_Surface* create_reflection_surface(SDL_Surface* src, float ratio = 0.5f, int max_alpha = 180) {
    if (!src) return nullptr;
    SDL_Surface* rgba = convert_to_rgba32(src);
    if (!rgba) return nullptr;
    
    int w = rgba->w;
    int h = rgba->h;
    int rh = std::max(1, int(h * ratio));
    
    SDL_Surface* refl = SDL_CreateRGBSurface(0, w, rh, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    if (!refl) { 
        SDL_FreeSurface(rgba); 
        return nullptr; 
    }

    SDL_LockSurface(rgba);
    SDL_LockSurface(refl);
    
    unsigned char* srcp = (unsigned char*)rgba->pixels;
    unsigned char* dstp = (unsigned char*)refl->pixels;
    int srcPitch = rgba->pitch;
    int dstPitch = refl->pitch;

    for (int y = 0; y < rh; y++) {
        int sy = h - 1 - y;
        unsigned char* srow = srcp + sy * srcPitch;
        unsigned char* drow = dstp + y * dstPitch;
        float t = 1.0f - (float)y / float(rh - 1 < 1 ? 1 : (rh - 1));
        float alpha_mul = (max_alpha / 255.0f) * t;
        
        for (int x = 0; x < w; x++) {
            unsigned char sr = srow[x*4 + 0];
            unsigned char sg = srow[x*4 + 1];
            unsigned char sb = srow[x*4 + 2];
            unsigned char sa = srow[x*4 + 3];
            unsigned char out_a = (unsigned char)(sa * alpha_mul);
            float dark = 0.96f;
            drow[x*4 + 0] = (unsigned char)(sr * dark);
            drow[x*4 + 1] = (unsigned char)(sg * dark);
            drow[x*4 + 2] = (unsigned char)(sb * dark);
            drow[x*4 + 3] = out_a;
        }
    }

    SDL_UnlockSurface(refl);
    SDL_UnlockSurface(rgba);
    SDL_FreeSurface(rgba);
    return refl;
}

static bool save_surface_png(SDL_Surface* surf, const std::string& out) {
    if (!surf) return false;
    SDL_Surface* rgba = convert_to_rgba32(surf);
    if (!rgba) return false;
    
    int w = rgba->w, h = rgba->h;
    int stride = w * 4;
    int res = stbi_write_png(out.c_str(), w, h, 4, rgba->pixels, stride);
    SDL_FreeSurface(rgba);
    return res != 0;
}

SDL_Surface* loadOrGenerateReflection(const std::string& boxartPath, const std::string& reflCacheDir) {
    if (boxartPath.empty()) return nullptr;
    fs::create_directories(reflCacheDir);

    std::string base = fs::path(boxartPath).stem().string();
    std::string cachePath = reflCacheDir + "/" + base + "_refl.png";

    // Try to load from cache
    if (fs::exists(cachePath)) {
        SDL_Surface* s = IMG_Load(cachePath.c_str());
        if (s) return s;
    }

    // Generate new reflection
    SDL_Surface* orig = IMG_Load(boxartPath.c_str());
    if (!orig) return nullptr;
    
    SDL_Surface* reflSurf = create_reflection_surface(orig, 0.5f, 180);
    if (!reflSurf) { 
        SDL_FreeSurface(orig); 
        return nullptr; 
    }

    // Save to cache
    if (!save_surface_png(reflSurf, cachePath)) {
        std::cerr << "Warning: could not save reflection to " << cachePath << "\n";
    }

    SDL_FreeSurface(orig);
    return reflSurf;
}