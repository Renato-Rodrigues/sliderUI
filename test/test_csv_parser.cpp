#include "core/csv_parser.h"
#include <iostream>
#include <fstream>
#include <unistd.h>

using core::CSVReader;

static std::string tmpfile(const std::string &s) {
    return std::string("/tmp/sliderui_csv_test_") + s + ".csv";
}

int test_quoted_semicolons() {
    std::string path = tmpfile("quoted");
    {
        std::ofstream out(path, std::ios::binary);
        out << "path;order;name;release\n";
        out << "\"/games/doom;v1\";0;\"Doom; The Game\";1993\n";
        out << "/games/quake;1;Quake;1996\n";
    }

    CSVReader r(';', true);
    if (!r.load(path)) {
        std::cerr << "[FAIL] load failed: " << (r.last_error().has_value() ? *r.last_error() : "unknown") << "\n";
        unlink(path.c_str());
        return 1;
    }

    auto rows = r.rows();
    if (rows.size() < 3) {
        std::cerr << "[FAIL] expected 3 rows, got " << rows.size() << "\n";
        unlink(path.c_str());
        return 2;
    }

    // header check
    if (rows[0].size() != 4 || rows[0][0] != "path" || rows[0][3] != "release") {
        std::cerr << "[FAIL] header mismatch\n";
        unlink(path.c_str());
        return 3;
    }

    // quoted semicolons preserved
    if (rows[1].size() != 4) {
        std::cerr << "[FAIL] row1 field count mismatch\n";
        unlink(path.c_str());
        return 4;
    }
    if (rows[1][0] != "/games/doom;v1") {
        std::cerr << "[FAIL] path field mismatch: got '" << rows[1][0] << "'\n";
        unlink(path.c_str());
        return 5;
    }
    if (rows[1][2] != "Doom; The Game") {
        std::cerr << "[FAIL] name field mismatch: got '" << rows[1][2] << "'\n";
        unlink(path.c_str());
        return 6;
    }

    unlink(path.c_str());
    return 0;
}

int test_save_and_reload() {
    std::string path = tmpfile("save");
    CSVReader r(';', true);
    std::vector<std::vector<std::string>> rows = {
        {"path","order","name"},
        {"/games/a","0","SimpleName"},
        {"/games/b","1","Name;With;Semicolons"},
        {"/games/c","2","Quote\"Inside"}
    };

    if (!r.save(path, rows)) {
        std::cerr << "[FAIL] save returned false\n";
        unlink(path.c_str());
        return 1;
    }

    CSVReader r2(';', true);
    if (!r2.load(path)) {
        std::cerr << "[FAIL] reload failed\n";
        unlink(path.c_str());
        return 2;
    }
    auto out = r2.rows();
    if (out.size() != rows.size()) {
        std::cerr << "[FAIL] reload row count mismatch\n";
        unlink(path.c_str());
        return 3;
    }
    // check the semicolons and quotes preserved
    if (out[2][2] != "Name;With;Semicolons") {
        std::cerr << "[FAIL] semicolons lost: got '" << out[2][2] << "'\n";
        unlink(path.c_str());
        return 4;
    }
    if (out[3][2] != "Quote\"Inside") {
        std::cerr << "[FAIL] quote lost: got '" << out[3][2] << "'\n";
        unlink(path.c_str());
        return 5;
    }

    unlink(path.c_str());
    return 0;
}

int main() {
    int fails = 0;
    std::cout << "[test] csv_parser: running tests\n";
    fails += test_quoted_semicolons();
    fails += test_save_and_reload();

    if (fails == 0) {
        std::cout << "[OK] csv_parser tests passed\n";
    } else {
        std::cout << "[FAIL] csv_parser tests failed (" << fails << ")\n";
    }
    return fails;
}
