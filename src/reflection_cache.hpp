#ifndef REFLECTION_CACHE_HPP
#define REFLECTION_CACHE_HPP

#include <SDL.h>
#include <string>

// Creates or loads a reflection texture for the given boxart path.
// - renderer: SDL_Renderer*
// - boxartPath: absolute path to original boxart image
// - reflCacheDir: directory to save/load cached reflections (e.g. /mnt/SDCARD/App/sliderUI/cache/reflections)
// Returns SDL_Texture* (owned by caller). On failure returns nullptr.
SDL_Texture* loadOrGenerateReflection(SDL_Renderer* renderer, const std::string& boxartPath, const std::string& reflCacheDir);

#endif // REFLECTION_CACHE_HPP