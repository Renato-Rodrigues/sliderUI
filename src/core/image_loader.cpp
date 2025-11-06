#include "core/image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
// Keep default stb_image features: supports BMP, PNG, JPG, etc.
#include "stb_image.h"

#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <optional>

#ifdef HAVE_WEBP
// If you enable libwebp, you can add decoding path here (not implemented in this file).
#include <webp/decode.h>
#endif

namespace core {

// Helper - lowercase extension
static std::string path_lower(const std::string &p) {
  std::string s = p;
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
  return s;
}

static bool ends_with(const std::string &s, const std::string &suffix) {
  if (s.size() < suffix.size()) return false;
  return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Convert 8-bit r,g,b -> rgb565 uint16_t
static inline uint16_t rgb_to_rgb565(unsigned char r, unsigned char g, unsigned char b) {
  uint16_t R = static_cast<uint16_t>(r >> 3);
  uint16_t G = static_cast<uint16_t>(g >> 2);
  uint16_t B = static_cast<uint16_t>(b >> 3);
  return static_cast<uint16_t>((R << 11) | (G << 5) | (B));
}

// Nearest-neighbor resample from src_rgb (src_w*src_h*3) to target size
static std::vector<uint16_t> resample_nearest_to_rgb565(const unsigned char *src_rgb,
                                                        int src_w, int src_h,
                                                        int tgt_w, int tgt_h) {
  std::vector<uint16_t> out;
  out.resize(static_cast<size_t>(tgt_w) * static_cast<size_t>(tgt_h));
  for (int y = 0; y < tgt_h; ++y) {
    // compute source y
    int sy = (src_h == 1) ? 0 : (y * src_h) / tgt_h;
    if (sy >= src_h) sy = src_h - 1;
    for (int x = 0; x < tgt_w; ++x) {
      int sx = (src_w == 1) ? 0 : (x * src_w) / tgt_w;
      if (sx >= src_w) sx = src_w - 1;
      const unsigned char *p = src_rgb + (sy * src_w + sx) * 3;
      unsigned char r = p[0];
      unsigned char g = p[1];
      unsigned char b = p[2];
      out[y * tgt_w + x] = rgb_to_rgb565(r, g, b);
    }
  }
  return out;
}

// Main API
std::optional<Texture> decode_to_texture(const std::string &path, int target_w, int target_h) {
  if (target_w <= 0 || target_h <= 0) return std::nullopt;

  std::string lower = path_lower(path);
  if (ends_with(lower, ".webp")) {
#ifdef HAVE_WEBP
    // If libwebp is available, implement decoding path. For now, fallthrough to file-based decode only.
    // A proper implementation would read file contents and call WebPDecodeRGB into memory then continue.
    // Here we return nullopt if HAVE_WEBP not implemented.
    return std::nullopt;
#else
    return std::nullopt;
#endif
  }

  // Read file into memory to allow stbi_load_from_memory
  std::ifstream in(path, std::ios::binary);
  if (!in) return std::nullopt;
  std::vector<char> buf((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

  if (buf.empty()) return std::nullopt;

  int src_w = 0, src_h = 0, channels = 0;
  unsigned char *img = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(buf.data()),
                                             static_cast<int>(buf.size()),
                                             &src_w, &src_h, &channels, 3);
  if (!img) return std::nullopt;

  // Now img is src_w x src_h with 3 channels (RGB)
  Texture tex;
  tex.width = target_w;
  tex.height = target_h;
  // If target equals source, fast path convert to rgb565 directly
  if (src_w == target_w && src_h == target_h) {
    tex.pixels.resize(static_cast<size_t>(target_w) * static_cast<size_t>(target_h));
    const unsigned char *p = img;
    for (int y = 0; y < src_h; ++y) {
      for (int x = 0; x < src_w; ++x) {
        unsigned char r = p[0];
        unsigned char g = p[1];
        unsigned char b = p[2];
        tex.pixels[y * target_w + x] = rgb_to_rgb565(r, g, b);
        p += 3;
      }
    }
    stbi_image_free(img);
    return tex;
  }

  // Resample nearest
  auto out_pixels = resample_nearest_to_rgb565(img, src_w, src_h, target_w, target_h);
  tex.pixels = std::move(out_pixels);
  stbi_image_free(img);
  return tex;
}

} // namespace core
