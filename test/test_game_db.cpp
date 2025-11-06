#include "core/game_db.h"
#include "core/csv_parser.h"
#include <iostream>
#include <fstream>
#include <unistd.h>

using core::GameDB;
using core::Game;

static std::string tmpfile(const std::string &s) {
    return std::string("/tmp/sliderui_gamedb_test_") + s + ".csv";
}

int test_load_and_assign() {
    std::string path = tmpfile("load");
    {
        std::ofstream out(path, std::ios::binary);
        out << "gamePath;order;gameName;release\n";
        // path contains platform folder with parentheses to exercise extraction
        out << "/mnt/SDCARD/PlatformOne (coreA)/gameA; ;Game A (deluxe);1990-01-01\n"; // missing order
        out << "/mnt/SDCARD/PlatformTwo/gameB;2;Game B;1991\n";
        out << "/mnt/SDCARD/PlainPlatform (coreC)/gameC; ;Game C;\n";
    }
    GameDB db;
    if (!db.load(path)) {
        std::cerr << "[FAIL] load failed\n";
        unlink(path.c_str());
        return 1;
    }
    if (db.games().size() != 3) {
        std::cerr << "[FAIL] load size mismatch\n";
        unlink(path.c_str());
        return 2;
    }

    // Check platform extraction for first row
    {
      const auto &g0 = db.games()[0];
      if (g0.platform_id != "PlatformOne") {
        std::cerr << "[FAIL] platform_id extraction mismatch: got '" << g0.platform_id << "' expected 'PlatformOne'\n";
        unlink(path.c_str());
        return 3;
      }
      if (!g0.platform_core.has_value() || *g0.platform_core != "coreA") {
        std::cerr << "[FAIL] platform_core extraction mismatch: got '" << (g0.platform_core.has_value() ? *g0.platform_core : "<none>") << "' expected 'coreA'\n";
        unlink(path.c_str());
        return 4;
      }
      if (g0.name != "Game A") {
        std::cerr << "[FAIL] name cleaning mismatch: got '" << g0.name << "' expected 'Game A'\n";
        unlink(path.c_str());
        return 5;
      }
    }

    db.ensure_orders_assigned();
    // after assignment, orders should be unique and present and normalized
    int last = -1;
    for (const auto &g : db.games()) {
        if (g.order <= last) {
            std::cerr << "[FAIL] orders not increasing\n";
            unlink(path.c_str());
            return 6;
        }
        last = g.order;
    }
    unlink(path.c_str());
    return 0;
}

int test_move_remove_commit() {
    std::string path = tmpfile("ops");
    {
        std::ofstream out(path, std::ios::binary);
        out << "gamePath;order;gameName;release\n";
        out << "/mnt/SDCARD/POne/gameA;0;A;1990\n";
        out << "/mnt/SDCARD/PTwo/gameB;1;B;1991\n";
        out << "/mnt/SDCARD/PThree/gameC;2;C;1992\n";
    }
    GameDB db;
    if (!db.load(path)) {
        std::cerr << "[FAIL] load failed\n";
        unlink(path.c_str());
        return 1;
    }
    // move down 0 -> becomes B,A,C
    db.move_down(0);
    if (db.games().size() < 2 || db.games()[0].path.find("/mnt/SDCARD/PTwo") == std::string::npos) {
        std::cerr << "[FAIL] move_down failed\n";
        unlink(path.c_str());
        return 2;
    }
    // move up index 1 (A) -> back to A,B,C
    db.move_up(1);
    if (db.games()[0].path.find("/mnt/SDCARD/POne") == std::string::npos) {
        std::cerr << "[FAIL] move_up failed\n";
        unlink(path.c_str());
        return 3;
    }
    // remove middle (B)
    if (!db.remove(1)) {
        std::cerr << "[FAIL] remove returned false\n";
        unlink(path.c_str());
        return 4;
    }
    if (db.games().size() != 2 || db.games()[1].path.find("/mnt/SDCARD/PThree") == std::string::npos) {
        std::cerr << "[FAIL] remove produced wrong vector\n";
        unlink(path.c_str());
        return 5;
    }
    // commit and reload to ensure persisted order is contiguous and correct
    if (!db.commit()) {
        std::cerr << "[FAIL] commit failed\n";
        unlink(path.c_str());
        return 6;
    }
    GameDB db2;
    if (!db2.load(path)) {
        std::cerr << "[FAIL] reload failed\n";
        unlink(path.c_str());
        return 7;
    }
    if (db2.games().size() != 2) {
        std::cerr << "[FAIL] reload size mismatch\n";
        unlink(path.c_str());
        return 8;
    }
    // expected paths: first is POne, second is PThree
    if (db2.games()[0].path.find("/mnt/SDCARD/POne") == std::string::npos || db2.games()[1].path.find("/mnt/SDCARD/PThree") == std::string::npos) {
        std::cerr << "[FAIL] reload content mismatch\n";
        unlink(path.c_str());
        return 9;
    }
    unlink(path.c_str());
    return 0;
}

int main() {
    int fails = 0;
    std::cout << "[test] game_db: running tests\n";
    fails += test_load_and_assign();
    fails += test_move_remove_commit();

    if (fails == 0) {
        std::cout << "[OK] game_db tests passed\n";
    } else {
        std::cout << "[FAIL] game_db tests failed (" << fails << ")\n";
    }
    return fails;
}
