#pragma once
// fluxerpp/env.h
#include <fstream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include "fluxerpp/util/Logger.h"

namespace fluxerpp {

namespace detail {
inline bool looks_like_secret_key(const std::string& key) {
    std::string upper = key;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                    [](unsigned char c) { return std::toupper(c); });
    static const char* markers[] = {"TOKEN", "SECRET", "KEY", "PASSWORD", "PASS"};
    for (auto* m : markers) {
        if (upper.find(m) != std::string::npos) return true;
    }
    return false;
}
} // namespace detail

inline void load_env(const std::string& path = ".env") {
    using util::Logger;
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::instance().debug("Could not open .env file at " + path);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Strip BOM if present
        if (line.rfind("\xEF\xBB\xBF", 0) == 0) {
            line.erase(0, 3);
        }

        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back()))) key.pop_back();
        while (!val.empty() && std::isspace(static_cast<unsigned char>(val.front()))) val.erase(0, 1);

        // Never print the value for anything that looks like a secret.
        Logger::instance().debug("Setting ENV: " + key + "=" +
            (detail::looks_like_secret_key(key) ? util::Logger::redact(val) : val));

#ifdef _WIN32
        _putenv_s(key.c_str(), val.c_str());
#else
        setenv(key.c_str(), val.c_str(), 1);
#endif
    }
}

} // namespace fluxerpp