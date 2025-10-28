#include "dat_cache.h"
#include <fstream>
#include <filesystem>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

DatCache::DatCache(const std::string& cachePath) : cacheFilePath(cachePath) {}

bool DatCache::load() {
    std::ifstream in(cacheFilePath);
    if (!in.is_open()) return false;
    systems.clear();
    std::string line;
    std::string currentSys;
    while (std::getline(in, line)) {
        if (line.rfind("#SYS:", 0) == 0) {
            currentSys = line.substr(5);
            systems[currentSys] = SystemCache{};
        } else if (line.rfind("#MTIME:", 0) == 0 && !currentSys.empty()) {
            systems[currentSys].mtime = std::stoll(line.substr(7));
        } else if (!currentSys.empty()) {
            size_t s1 = line.find('|');
            size_t s2 = line.find('|', s1+1);
            if (s1!=std::string::npos && s2!=std::string::npos) {
                std::string name = line.substr(0,s1);
                std::string desc = line.substr(s1+1, s2-s1-1);
                int year = std::stoi(line.substr(s2+1));
                systems[currentSys].entries[name] = {desc, year};
            }
        }
    }
    return true;
}

void DatCache::save() const {
    std::ofstream out(cacheFilePath, std::ios::trunc);
    if (!out.is_open()) return;
    for (const auto& kv : systems) {
        out << "#SYS:" << kv.first << "\n";
        out << "#MTIME:" << kv.second.mtime << "\n";
        for (const auto& e : kv.second.entries) {
            out << e.first << "|" << e.second.description << "|" << e.second.year << "\n";
        }
    }
}

bool DatCache::parseDatFile(const std::string& systemCode, const std::string& datPath) {
    if (!fs::exists(datPath)) return false;
    long long mtime = (long long)fs::last_write_time(datPath).time_since_epoch().count();
    auto &sc = systems[systemCode];
    if (sc.mtime == mtime && !sc.entries.empty()) return true;
    sc.entries.clear();
    sc.mtime = mtime;

    std::ifstream in(datPath);
    if (!in.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    // Define regex patterns once
    std::regex gameRe(R"(<game\s+name\s*=\s*"([^"]+)")", std::regex::icase);
    std::regex descRe(R"(<description>([^<]+)</description>)", std::regex::icase);
    std::regex yearRe(R"(<year>(\d{4})</year>)", std::regex::icase);
    
    std::smatch m;
    auto start = content.cbegin();
    while (std::regex_search(start, content.cend(), m, gameRe)) {
        std::string gameName = m[1];
        size_t pos = m.position(0);
        size_t blockStart = pos;
        size_t nextPos = content.find("<game", pos+5);
        std::string block = content.substr(blockStart, nextPos - blockStart);
        std::smatch desc, yr;
        DatMetadata meta;
        if (std::regex_search(block, desc, descRe)) meta.description = desc[1];
        if (std::regex_search(block, yr, yearRe)) meta.year = std::stoi(yr[1]);
        sc.entries[gameName] = meta;
        start = m.suffix().first;
    }
    return true;
}

DatMetadata DatCache::getMetadata(const std::string& systemCode, const std::string& gameBasename, const std::string& datPath) {
    parseDatFile(systemCode, datPath);
    auto itSys = systems.find(systemCode);
    if (itSys == systems.end()) return {};
    auto it = itSys->second.entries.find(gameBasename);
    if (it != itSys->second.entries.end()) return it->second;
    for (const auto& kv : itSys->second.entries) {
        if (kv.first.find(gameBasename) != std::string::npos) return kv.second;
    }
    return {};
}