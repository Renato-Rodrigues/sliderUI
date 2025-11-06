#include "core/config_manager.h"
#include "core/file_utils.h"
#include <iostream>
#include <fstream>
#include <unistd.h>

using core::ConfigManager;

static std::string tmp_path(const std::string &name) {
    std::string base = "/tmp/sliderui_cfg_test_";
    base += name;
    return base;
}

int test_missing_file() {
    std::string p = tmp_path("missing.json");
    // Ensure removal
    unlink(p.c_str());

    ConfigManager cfg;
    bool ok = cfg.load(p);
    if (!ok) {
        std::cerr << "[FAIL] load returned false for missing file (expected true with defaults)\n";
        return 1;
    }
    // check a default value present
    std::string bg = cfg.get<std::string>("ui.background", "MISSING");
    if (bg != "bckg.png") {
        std::cerr << "[FAIL] default background mismatch: " << bg << "\n";
        return 2;
    }
    return 0;
}

int test_corrupt_file() {
    std::string p = tmp_path("corrupt.json");
    // write corrupted content
    {
        std::ofstream out(p, std::ios::binary);
        out << "{ this is not valid json ";
    }
    ConfigManager cfg;
    bool ok = cfg.load(p);
    if (ok) {
        std::cerr << "[FAIL] load returned true for corrupt file (expected false)\n";
        unlink(p.c_str());
        return 1;
    }
    // After corrupt load, defaults must be present
    std::string sort_mode = cfg.get<std::string>("behavior.sort_mode", "MISSING");
    if (sort_mode != "alphabetical") {
        std::cerr << "[FAIL] after corrupt fallback, sort_mode mismatch: " << sort_mode << "\n";
        unlink(p.c_str());
        return 2;
    }
    unlink(p.c_str());
    return 0;
}

int test_partial_file() {
    std::string p = tmp_path("partial.json");
    // write a partial JSON with only background overwritten
    {
        std::ofstream out(p, std::ios::binary);
        out << R"({"ui": {"background": "custom.png"}, "behavior": {"start_game":"first_game"}})";
    }
    ConfigManager cfg;
    bool ok = cfg.load(p);
    if (!ok) {
        std::cerr << "[FAIL] load returned false for partial file\n";
        unlink(p.c_str());
        return 1;
    }
    // check that custom value was kept
    std::string bg = cfg.get<std::string>("ui.background", "MISSING");
    if (bg != "custom.png") {
        std::cerr << "[FAIL] partial load didn't keep custom background: " << bg << "\n";
        unlink(p.c_str());
        return 2;
    }
    // check that other defaults remain
    int title_x = cfg.get<int>("ui.title.x", -1);
    if (title_x != 20) {
        std::cerr << "[FAIL] partial load lost default ui.title.x: " << title_x << "\n";
        unlink(p.c_str());
        return 3;
    }
    // check overridden behavior.start_game
    std::string sg = cfg.get<std::string>("behavior.start_game", "MISSING");
    if (sg != "first_game") {
        std::cerr << "[FAIL] partial load override start_game mismatch: " << sg << "\n";
        unlink(p.c_str());
        return 4;
    }
    unlink(p.c_str());
    return 0;
}

int main() {
    int fails = 0;
    std::cout << "[test] config_manager: running tests\n";
    fails += test_missing_file();
    fails += test_corrupt_file();
    fails += test_partial_file();

    if (fails == 0) {
        std::cout << "[OK] config_manager tests passed\n";
    } else {
        std::cout << "[FAIL] config_manager tests failed (" << fails << ")\n";
    }
    return fails;
}
