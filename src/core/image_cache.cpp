// src/core/image_cache.cpp
#include "core/image_cache.h"
#include "core/logger.h"
#include <cstring>
#include <iostream>

// DO NOT define STB_IMAGE_IMPLEMENTATION here!
// image_loader.cpp contains the single STB implementation.
#include "stb_image.h"

#if defined(__unix__) || defined(__APPLE__)
 #include <SDL/SDL.h>
#elif defined(_WIN32)
 #include <SDL.h>
#else
 #include <SDL/SDL.h>
#endif

using namespace core;

ImageCache::ImageCache() : prefer_rgba_(false) {}

ImageCache::~ImageCache() {
    clear();
}

void ImageCache::set_prefer_rgba(bool v) {
    std::lock_guard<std::mutex> lock(mtx_);
    prefer_rgba_ = v;
}

bool ImageCache::preload(const std::string &path) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (cache_.find(path) != cache_.end()) return true;
    ImageData data;
    if (!decode_image_to_memory(path, data)) {
        return false;
    }
    cache_.emplace(path, std::move(data));
    return true;
}

bool ImageCache::has(const std::string &path) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return cache_.find(path) != cache_.end();
}

ImageData ImageCache::get(const std::string &path) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = cache_.find(path);
    if (it != cache_.end()) return it->second;
    return ImageData();
}

void *ImageCache::get_surface_for_path(const std::string &path) {
#if defined(SDL_MAJOR_VERSION)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto sit = surface_cache_.find(path);
        if (sit != surface_cache_.end()) return sit->second;
    }

    // Ensure decoded pixel data available (lazy decode)
    ImageData data;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = cache_.find(path);
        if (it != cache_.end()) data = it->second;
    }

    if (data.pixels.empty()) {
        ImageData decoded;
        if (!decode_image_to_memory(path, decoded)) return nullptr;
        std::lock_guard<std::mutex> lock(mtx_);
        cache_[path] = std::move(decoded);
        data = cache_[path];
    }

    if (data.pixels.empty()) return nullptr;

    SDL_Surface *surf = nullptr;
    int w = data.width;
    int h = data.height;
    int channels = data.channels;

    // Decide desired channels based on prefer_rgba_ and what data has
    int want = 3;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        want = prefer_rgba_ ? 4 : 3;
    }

    // If decoded data channels != want, reload with stbi to get desired channels
    if (channels != want) {
        stbi_uc *pixels = stbi_load(path.c_str(), &w, &h, &channels, want);
        if (!pixels) {
            Logger::instance().info(std::string("stbi_load in get_surface_for_path failed for ") + path);
            return nullptr;
        }
        channels = want;
        // copy pixels into ImageData for future use
        ImageData newd;
        newd.path = path;
        newd.width = w;
        newd.height = h;
        newd.channels = channels;
        size_t bytes2 = size_t(w) * size_t(h) * size_t(channels);
        newd.pixels.assign(pixels, pixels + bytes2);
        stbi_image_free(pixels);
        {
            std::lock_guard<std::mutex> lock(mtx_);
            cache_[path] = std::move(newd);
            data = cache_[path];
        }
    }

    if (data.channels == 4) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        Uint32 rmask = 0xff000000;
        Uint32 gmask = 0x00ff0000;
        Uint32 bmask = 0x0000ff00;
        Uint32 amask = 0x000000ff;
#else
        Uint32 rmask = 0x000000ff;
        Uint32 gmask = 0x0000ff00;
        Uint32 bmask = 0x00ff0000;
        Uint32 amask = 0xff000000;
#endif
        surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask);
        if (!surf) {
            Logger::instance().error(std::string("SDL_CreateRGBSurface failed for RGBA: ") + SDL_GetError());
            return nullptr;
        }
        Uint8 *dst = (Uint8*)surf->pixels;
        const unsigned char *src = data.pixels.data();
        size_t bytes2 = size_t(w) * size_t(h) * 4;
        memcpy(dst, src, bytes2);
    } else if (data.channels == 3) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        Uint32 rmask = 0xff000000;
        Uint32 gmask = 0x00ff0000;
        Uint32 bmask = 0x0000ff00;
        Uint32 amask = 0x000000ff;
#else
        Uint32 rmask = 0x000000ff;
        Uint32 gmask = 0x0000ff00;
        Uint32 bmask = 0x00ff0000;
        Uint32 amask = 0xff000000;
#endif
        SDL_Surface *tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask);
        if (!tmp) {
            Logger::instance().error(std::string("SDL_CreateRGBSurface failed for RGB: ") + SDL_GetError());
            return nullptr;
        }
        Uint8 *dst = (Uint8*)tmp->pixels;
        const unsigned char *src = data.pixels.data();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                size_t si = (size_t(y) * w + x) * 3;
                size_t di = (size_t(y) * w + x) * 4;
                dst[di + 0] = src[si + 0];
                dst[di + 1] = src[si + 1];
                dst[di + 2] = src[si + 2];
                dst[di + 3] = 255;
            }
        }
        surf = tmp;
    } else {
        Logger::instance().info(std::string("Unsupported channel count: ") + std::to_string(data.channels));
        return nullptr;
    }

    if (!surf) return nullptr;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        surface_cache_[path] = surf;
    }
    return surf;
#else
    (void)path;
    return nullptr;
#endif
}

void ImageCache::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
#if defined(SDL_MAJOR_VERSION)
    for (auto &p : surface_cache_) {
        if (p.second) {
            SDL_Surface *s = (SDL_Surface*)p.second;
            SDL_FreeSurface(s);
        }
    }
#endif
    surface_cache_.clear();
    cache_.clear();
}

bool ImageCache::decode_image_to_memory(const std::string &path, ImageData &out) {
    int w=0,h=0,ch=0;
    int wanted = prefer_rgba_ ? 4 : 3;
    unsigned char *pixels = stbi_load(path.c_str(), &w, &h, &ch, wanted);
    if (!pixels) {
        Logger::instance().info(std::string("stbi_load failed for ") + path);
        return false;
    }
    out.path = path;
    out.width = w;
    out.height = h;
    out.channels = wanted;
    out.pixels.assign(pixels, pixels + (size_t)w * (size_t)h * (size_t)wanted);
    stbi_image_free(pixels);
    return true;
}
