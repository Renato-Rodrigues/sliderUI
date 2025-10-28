#ifndef DAT_CACHE_H
#define DAT_CACHE_H

#include <string>
#include <map>

struct DatMetadata {
    std::string description;
    int year = 0;
};

class DatCache {
public:
    DatCache(const std::string& cachePath);
    bool load();
    void save() const;
    DatMetadata getMetadata(const std::string& systemCode, const std::string& gameBasename, const std::string& datPath);

private:
    struct SystemCache {
        long long mtime = 0;
        std::map<std::string, DatMetadata> entries;
    };

    std::string cacheFilePath;
    std::map<std::string, SystemCache> systems;

    bool parseDatFile(const std::string& systemCode, const std::string& datPath);
};

#endif // DAT_CACHE_H