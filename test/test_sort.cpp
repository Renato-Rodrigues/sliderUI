#include "core/sort.h"
#include "core/game_db.h"
#include <iostream>
#include <vector>
#include <optional>

using core::Game;
using core::SortMode;
using core::sort_games;

int test_alpha() {
    std::vector<Game> v = {
        {"/games/zoo.rom", 0, "Zelda", std::optional<std::string>{}, "", std::nullopt},
        {"/games/abc.rom", 0, "apple", std::optional<std::string>{}, "", std::nullopt},
        {"/games/norom.rom", 0, "", std::optional<std::string>{}, "", std::nullopt}, // basename no ext -> norom
    };
    sort_games(v, SortMode::ALPHA);
    // expected: apple, norom, Zelda  (case-insensitive)
    if (v[0].name != "apple") return 1;
    if ((v[1].name.empty() ? std::string("norom") : v[1].name) != "norom") return 2;
    if (v[2].name != "Zelda") return 3;
    return 0;
}

int test_release() {
    std::vector<Game> v = {
        {"/g/a.rom", 0, "A", std::optional<std::string>("1995-01-01"), "", std::nullopt},
        {"/g/b.rom", 0, "B", std::optional<std::string>("1996"), "", std::nullopt},
        {"/g/c.rom", 0, "C", std::optional<std::string>{}, "", std::nullopt}, // unknown
        {"/g/d.rom", 0, "D", std::optional<std::string>("1996-05"), "", std::nullopt}
    };
    sort_games(v, SortMode::RELEASE);
    // default ascending: oldest-first -> A(1995-01-01), B(1996), D(1996-05), C(unknown)
    if (!v[0].release_iso.has_value() || *v[0].release_iso != std::string("1995-01-01")) return 1;
    if (!v[1].release_iso.has_value() || *v[1].release_iso != std::string("1996")) return 2;
    if (!v[2].release_iso.has_value() || *v[2].release_iso != std::string("1996-05")) return 3;
    if (v[3].release_iso.has_value()) return 4;
    return 0;
}

int test_custom() {
    std::vector<Game> v = {
        {"/g/a.rom", 5, "A", std::optional<std::string>{}, "", std::nullopt},
        {"/g/b.rom", 2, "B", std::optional<std::string>{}, "", std::nullopt},
        {"/g/c.rom", 3, "C", std::optional<std::string>{}, "", std::nullopt}
    };
    sort_games(v, SortMode::CUSTOM);
    // order ascending: B(order2), C(order3), A(order5)
    if (v[0].path.find("/g/b.rom") == std::string::npos) return 1;
    if (v[1].path.find("/g/c.rom") == std::string::npos) return 2;
    if (v[2].path.find("/g/a.rom") == std::string::npos) return 3;
    return 0;
}

int main() {
    int fails = 0;
    std::cout << "[test] sort: running tests\n";
    fails += test_alpha();
    fails += test_release();
    fails += test_custom();
    if (fails == 0) {
        std::cout << "[OK] sort tests passed\n";
    } else {
        std::cout << "[FAIL] sort tests failed (" << fails << ")\n";
    }
    return fails;
}
