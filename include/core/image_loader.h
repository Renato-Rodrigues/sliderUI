#pragma once
#ifndef SLIDERUI_CORE_IMAGE_LOADER_H
#define SLIDERUI_CORE_IMAGE_LOADER_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace core {

/**
 * Texture - decoded image in RGB565 format
 * width, height: dimensions in pixels
 * pixels: row-major, top-to-bottom, left-to-right, size == width * height
 * Each pixel is a 16-bit RGB565 value.
 */
struct Texture {
  int width = 0;
  int height = 0;
  std::vector<uint16_t> pixels; // size = width * height
};

/**
 * Decode image at `path` and convert/rescale to target_w x target_h.
 * Allowed source formats depend on stb_image compiled formats (PNG, JPG, BMP...).
 *
 * - If path ends with .webp and HAVE_WEBP is **not** defined at compile time,
 *   function returns std::nullopt (caller must handle).
 * - Returns std::optional<Texture> with rgb565 pixels on success, std::nullopt on failure.
 *
 * Note: implementation uses stb_image.h. No dynamic allocation policy applies only
 * to runtime rendering loop; this helper can allocate.
 */
std::optional<Texture> decode_to_texture(const std::string &path, int target_w, int target_h);

} // namespace core

#endif // SLIDERUI_CORE_IMAGE_LOADER_H
