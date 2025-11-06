#include "core/image_cache.h"
#include "core/image_loader.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <cstdint>

using core::ImageCache;
using core::Texture;

// reuse BMP writer from earlier tests (2x2)
static bool write_2x2_bmp(const std::string &path, const std::vector<unsigned char> &pixels_rgb_topdown) {
    if (pixels_rgb_topdown.size() != 2 * 2 * 3) return false;
    const uint32_t width = 2;
    const uint32_t height = 2;
    const uint32_t row_bytes = ((width * 3 + 3) / 4) * 4;
    const uint32_t pixel_data_size = row_bytes * height;
    const uint32_t file_size = 14 + 40 + pixel_data_size;
    std::vector<unsigned char> out;
    out.reserve(file_size);
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
    // header
    out.push_back('B'); out.push_back('M');
    push_u32(file_size);
    push_u16(0); push_u16(0);
    push_u32(14 + 40);
    // infoheader
    push_u32(40); push_u32(width); push_u32(height); push_u16(1); push_u16(24);
    push_u32(0); push_u32(pixel_data_size); push_u32(2835); push_u32(2835); push_u32(0); push_u32(0);
    // pixel data bottom-up
    for (int row = height - 1; row >= 0; --row) {
        for (uint32_t col = 0; col < width; ++col) {
            size_t idx = (row * width + col) * 3;
            unsigned char r = pixels_rgb_topdown[idx + 0];
            unsigned char g = pixels_rgb_topdown[idx + 1];
            unsigned char b = pixels_rgb_topdown[idx + 2];
            out.push_back(b); out.push_back(g); out.push_back(r);
        }
        size_t pad = row_bytes - width * 3;
        for (size_t i = 0; i < pad; ++i) out.push_back(0);
    }
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(out.data()), out.size());
    return f.good();
}

static uint16_t rgb565_from_rgb(unsigned char r, unsigned char g, unsigned char b) {
    uint16_t R = static_cast<uint16_t>(r >> 3);
    uint16_t G = static_cast<uint16_t>(g >> 2);
    uint16_t B = static_cast<uint16_t>(b >> 3);
    return static_cast<uint16_t>((R << 11) | (G << 5) | B);
}

int main() {
    std::cout << "[test] image_cache: running\n";

    // create 4 small images in /tmp
    std::vector<std::string> paths;
    for (int i = 0; i < 4; ++i) {
        std::string p = "/tmp/sliderui_cache_test_" + std::to_string(i) + ".bmp";
        // pixels: top-left varies by i
        std::vector<unsigned char> pix = {
            static_cast<unsigned char>((i*40) & 0xFF), 0, 0,
            0, static_cast<unsigned char>((i*60) & 0xFF), 0,
            0, 0, static_cast<unsigned char>((i*80) & 0xFF),
            255, 255, 255
        };
        if (!write_2x2_bmp(p, pix)) {
            std::cerr << "[FAIL] cannot write bmp " << p << "\n";
            return 1;
        }
        paths.push_back(p);
    }

    ImageCache cache(3); // capacity 3

    // preload all four images
    for (const auto &p : paths) cache.preload_priority(p, 2, 2);

    // process tasks one by one
    int tasks_processed = 0;
    while (cache.tick_one_task()) {
        ++tasks_processed;
        if (tasks_processed > 10) break;
    }
    if (tasks_processed != 4) {
        std::cerr << "[FAIL] expected 4 tasks processed, got " << tasks_processed << "\n";
        return 2;
    }

    // At this point cache capacity is 3; the first enqueued (paths[0]) should have been evicted
    auto t0 = cache.get(paths[0]);
    if (t0.has_value()) {
        std::cerr << "[FAIL] expected first image evicted\n";
        return 3;
    }
    // others must exist
    for (int i = 1; i < 4; ++i) {
        auto t = cache.get(paths[i]);
        if (!t.has_value()) {
            std::cerr << "[FAIL] expected path " << paths[i] << " in cache\n";
            return 4;
        }
        // validate pixel 0 equals the top-left color
        uint16_t expected = rgb565_from_rgb(static_cast<unsigned char>((i*40)&0xFF), 0, 0);
        if (t->pixels[0] != expected) {
            std::cerr << "[FAIL] pixel mismatch for " << paths[i] << "\n";
            return 5;
        }
    }

    // test LRU behavior: access paths[1] to make it most recent, then preload a new image
    cache.get(paths[1]);
    // create one more image
    std::string p4 = "/tmp/sliderui_cache_test_4.bmp";
    std::vector<unsigned char> pix4 = {10,11,12, 13,14,15, 16,17,18, 255,255,255};
    if (!write_2x2_bmp(p4, pix4)) { std::cerr << "[FAIL] write p4\n"; return 6; }
    cache.preload_priority(p4, 2, 2);
    cache.tick_one_task(); // process new task

    // now cache should contain paths[1], paths[3], p4 (paths[2] maybe evicted depending on LRU)
    // check that paths[1] is present (we accessed it)
    if (!cache.get(paths[1]).has_value()) {
        std::cerr << "[FAIL] expected paths[1] present after LRU touch\n";
        return 7;
    }

    // cleanup files
    for (const auto &p : paths) unlink(p.c_str());
    unlink(p4.c_str());

    std::cout << "[OK] image_cache tests passed\n";
    return 0;
}
