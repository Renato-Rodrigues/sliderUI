#include "core/image_loader.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <cstdint>

using core::Texture;
using core::decode_to_texture;

// utility: convert rgb uint8 -> rgb565
static uint16_t rgb565_from_rgb(unsigned char r, unsigned char g, unsigned char b) {
    uint16_t R = static_cast<uint16_t>(r >> 3);
    uint16_t G = static_cast<uint16_t>(g >> 2);
    uint16_t B = static_cast<uint16_t>(b >> 3);
    return static_cast<uint16_t>((R << 11) | (G << 5) | B);
}

// create a tiny 2x2 BMP (24-bit) at path with specified pixels (row-major top-to-bottom)
static bool write_2x2_bmp(const std::string &path, const std::vector<unsigned char> &pixels_rgb_topdown) {
    if (pixels_rgb_topdown.size() != 2 * 2 * 3) return false;
    // BMP stores rows bottom-up. We will construct pixel data accordingly with padding to 4-bytes per row.
    // Each row: 2 pixels * 3 bytes = 6 bytes -> padded to 8 (2 padding bytes)
    const uint32_t width = 2;
    const uint32_t height = 2;
    const uint32_t row_bytes = ((width * 3 + 3) / 4) * 4; // padded row size
    const uint32_t pixel_data_size = row_bytes * height;
    const uint32_t file_size = 14 + 40 + pixel_data_size;

    std::vector<unsigned char> out;
    out.reserve(file_size);

    // BITMAPFILEHEADER (14 bytes)
    out.push_back('B'); out.push_back('M');
    auto push_u32 = [&](uint32_t v) {
        out.push_back(static_cast<unsigned char>(v & 0xFF));
        out.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>((v >> 16) & 0xFF));
        out.push_back(static_cast<unsigned char>((v >> 24) & 0xFF));
    };
    auto push_u16 = [&](uint16_t v) {
        out.push_back(static_cast<unsigned char>(v & 0xFF));
        out.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
    };
    push_u32(file_size);
    push_u16(0); push_u16(0);
    push_u32(14 + 40); // pixel data offset

    // BITMAPINFOHEADER (40 bytes)
    push_u32(40); // header size
    push_u32(width);
    push_u32(height);
    push_u16(1); // planes
    push_u16(24); // bits per pixel
    push_u32(0); // compression
    push_u32(pixel_data_size);
    push_u32(2835); // xppm ~72 dpi
    push_u32(2835); // yppm
    push_u32(0); // colors used
    push_u32(0); // important

    // Pixel data: bottom-up
    // pixels_rgb_topdown is top-to-bottom; we must write bottom row first.
    for (int row = height - 1; row >= 0; --row) {
        // each pixel: RGB in input; BMP expects BGR
        for (uint32_t col = 0; col < width; ++col) {
            size_t idx = (row * width + col) * 3;
            unsigned char r = pixels_rgb_topdown[idx + 0];
            unsigned char g = pixels_rgb_topdown[idx + 1];
            unsigned char b = pixels_rgb_topdown[idx + 2];
            // BMP pixel order: B, G, R
            out.push_back(b);
            out.push_back(g);
            out.push_back(r);
        }
        // padding to 4 bytes per row
        size_t pad = row_bytes - width * 3;
        for (size_t i = 0; i < pad; ++i) out.push_back(0);
    }

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(out.data()), out.size());
    return f.good();
}

int main() {
    std::string path = "/tmp/sliderui_test_img.bmp";
    // define top-to-bottom pixels:
    // Row0 (top): pixel0 (red), pixel1 (green)
    // Row1 (bottom): pixel2 (blue), pixel3 (white)
    std::vector<unsigned char> pixels = {
        // top row (left to right)
        255, 0, 0,   // red
        0, 255, 0,   // green
        // bottom row
        0, 0, 255,   // blue
        255, 255, 255 // white
    };
    if (!write_2x2_bmp(path, pixels)) {
        std::cerr << "[FAIL] cannot write bmp\n";
        return 1;
    }

    auto opt = decode_to_texture(path, 2, 2);
    unlink(path.c_str());
    if (!opt.has_value()) {
        std::cerr << "[FAIL] decode_to_texture returned nullopt\n";
        return 2;
    }
    Texture t = *opt;
    if (t.width != 2 || t.height != 2) {
        std::cerr << "[FAIL] wrong texture size\n";
        return 3;
    }
    // expected rgb565 values for pixels (top-left, top-right, bottom-left, bottom-right)
    uint16_t e0 = rgb565_from_rgb(255,0,0); // red
    uint16_t e1 = rgb565_from_rgb(0,255,0); // green
    uint16_t e2 = rgb565_from_rgb(0,0,255); // blue
    uint16_t e3 = rgb565_from_rgb(255,255,255); // white

    if (t.pixels.size() != 4) {
        std::cerr << "[FAIL] pixel count\n";
        return 4;
    }
    // row-major top-to-bottom
    if (t.pixels[0] != e0) { std::cerr << "[FAIL] p0 mismatch\n"; return 5; }
    if (t.pixels[1] != e1) { std::cerr << "[FAIL] p1 mismatch\n"; return 6; }
    if (t.pixels[2] != e2) { std::cerr << "[FAIL] p2 mismatch\n"; return 7; }
    if (t.pixels[3] != e3) { std::cerr << "[FAIL] p3 mismatch\n"; return 8; }

    std::cout << "[OK] image_loader test passed\n";
    return 0;
}
