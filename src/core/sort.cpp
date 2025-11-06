#include "core/sort.h"
#include "core/config_manager.h" // only inside .cpp

#include <algorithm>
#include <string>
#include <cctype>
#include <sstream>

using core::Game;
using core::SortMode;

namespace core {

// Helper: basename without extension from path
static std::string basename_no_ext(const std::string &path) {
    if (path.empty()) return std::string();
    std::string p = path;
    while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();
    size_t sep = p.find_last_of("/\\");
    std::string fname = (sep == std::string::npos) ? p : p.substr(sep + 1);
    size_t dot = fname.find_last_of('.');
    if (dot == std::string::npos) return fname;
    if (dot == 0) return fname;
    return fname.substr(0, dot);
}

// Helper: ASCII-lowercase copy (works fine for common Latin names)
static std::string as_lower(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}

// Date struct for release comparison
struct Date {
    int y = 0;
    int m = 0;
    int d = 0;
    bool valid = false;
};

// Parse YYYY, YYYY-MM, YYYY-MM-DD (numeric) into Date
static Date parse_date(const std::string &s) {
    Date dt;
    if (s.empty()) return dt;
    std::istringstream ss(s);
    std::string part;
    if (!std::getline(ss, part, '-')) return dt;
    try {
        if (part.size() != 4) return dt;
        dt.y = std::stoi(part);
    } catch (...) { return dt; }
    if (!std::getline(ss, part, '-')) { dt.valid = true; dt.m = 0; dt.d = 0; return dt; }
    try {
        if (part.empty() || part.size() > 2) return dt;
        dt.m = std::stoi(part);
        if (dt.m < 1 || dt.m > 12) return dt;
    } catch (...) { return dt; }
    if (!std::getline(ss, part, '-')) { dt.valid = true; dt.d = 0; return dt; }
    try {
        if (part.empty() || part.size() > 2) return dt;
        dt.d = std::stoi(part);
        if (dt.d < 1 || dt.d > 31) return dt;
    } catch (...) { return dt; }
    dt.valid = true;
    return dt;
}

// Compare two dates: return -1 if a<b (older), 0 if equal, +1 if a>b (newer)
static int cmp_date(const Date &a, const Date &b) {
    if (!a.valid && !b.valid) return 0;
    if (!a.valid) return -1;
    if (!b.valid) return 1;
    if (a.y != b.y) return (a.y < b.y) ? -1 : 1;
    if (a.m != b.m) return (a.m < b.m) ? -1 : 1;
    if (a.d != b.d) return (a.d < b.d) ? -1 : 1;
    return 0;
}

static bool release_descending_from_cfg(const ConfigManager *cfg) {
    if (!cfg) return false; // default ascending
    std::string val = cfg->get<std::string>("behavior.release_order", std::string("ascending"));
    // accept "descending" (case-insensitive)
    std::string vlow;
    vlow.resize(val.size());
    std::transform(val.begin(), val.end(), vlow.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return (vlow == "descending" || vlow == "desc");
}

void sort_games(std::vector<Game> &g, SortMode mode) {
    sort_games(g, mode, nullptr);
}

void sort_games(std::vector<Game> &g, SortMode mode, const ConfigManager *cfg) {
    bool release_descending = release_descending_from_cfg(cfg);

    switch (mode) {
        case SortMode::ALPHA: {
            std::stable_sort(g.begin(), g.end(), [](const Game &a, const Game &b) {
                std::string aa = !a.name.empty() ? a.name : basename_no_ext(a.path);
                std::string bb = !b.name.empty() ? b.name : basename_no_ext(b.path);
                auto la = as_lower(aa);
                auto lb = as_lower(bb);
                if (la != lb) return la < lb;
                return a.path < b.path;
            });
            break;
        }
        case SortMode::RELEASE: {
            std::stable_sort(g.begin(), g.end(), [&](const Game &a, const Game &b) {
                Date da = a.release_iso ? parse_date(*a.release_iso) : Date{};
                Date db = b.release_iso ? parse_date(*b.release_iso) : Date{};
                // valid dates come before invalid in either ordering, but
                // direction of newest/oldest depends on release_descending
                if (da.valid != db.valid) return da.valid > db.valid;
                if (!da.valid && !db.valid) {
                    std::string aa = !a.name.empty() ? a.name : basename_no_ext(a.path);
                    std::string bb = !b.name.empty() ? b.name : basename_no_ext(b.path);
                    return as_lower(aa) < as_lower(bb);
                }
                int cmp = cmp_date(da, db); // -1 if a older than b
                if (cmp != 0) {
                    if (release_descending) {
                        // newest-first
                        return cmp > 0;
                    } else {
                        // ascending (oldest-first)
                        return cmp < 0;
                    }
                }
                return a.path < b.path;
            });
            break;
        }
        case SortMode::CUSTOM: {
            std::stable_sort(g.begin(), g.end(), [](const Game &a, const Game &b) {
                if (a.order != b.order) return a.order < b.order;
                return a.path < b.path;
            });
            break;
        }
    }
}

} // namespace core
