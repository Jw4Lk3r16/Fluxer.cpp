#pragma once
#include <fstream>
#include <string>
#include <cstdlib>

namespace fluxerpp {

inline void load_env(const std::string& path = ".env") {
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        // Set environment variable
        _putenv_s(key.c_str(), val.c_str());
    }
}

}
