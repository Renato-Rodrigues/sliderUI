// include/core/image_cache.h
#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace core {

struct ImageData {
    std::string path;
    int width = 0;
    int height = 0;
    int channels = 0; // 3 => RGB, 4 => RGBA
    std::vector<unsigned char> pixels; // decoded pixels, row-major
};

class ImageCache {
public:
    ImageCache();
    ~ImageCache();

    // ensure image is decoded and cached (returns true on success)
    bool preload(const std::string &path);

    bool has(const std::string &path) const;
    ImageData get(const std::string &path) const;

    // NEW: return a cached native surface (SDL_Surface*) or nullptr if not available.
    // Owned by ImageCache; do not free the pointer.
    void *get_surface_for_path(const std::string &path);

    // configure whether ImageCache should prefer RGBA surfaces (true) or RGB-only (false).
    // Default is false (RGB only).
    void set_prefer_rgba(bool v);

    // remove cached decoded images and surfaces
    void clear();

private:
    bool decode_image_to_memory(const std::string &path, ImageData &out);

    mutable std::mutex mtx_;
    std::unordered_map<std::string, ImageData> cache_;

    // store native surfaces as void* to avoid SDL headers in the public header
    std::unordered_map<std::string, void*> surface_cache_;

    bool prefer_rgba_;
};

} // namespace core
