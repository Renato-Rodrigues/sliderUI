#ifndef REFLECTION_CACHE_HPP
#define REFLECTION_CACHE_HPP

#include <SDL.h>
#include <string>

// Creates or loads a reflection surface for the given boxart path.
// - boxartPath: absolute path to original boxart image
// - reflCacheDir: directory to save/load cached reflections
// Returns SDL_Surface* (owned by caller). On failure returns nullptr.
SDL_Surface* loadOrGenerateReflection(const std::string& boxartPath, const std::string& reflCacheDir);

#endif // REFLECTION_CACHE_HPP