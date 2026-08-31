#pragma once
#include <fstream>
#include <string>
#include <cstdlib>
#include <iostream>

namespace fluxerpp {

inline void load_env(const std::string& path = ".env") {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[DEBUG] Could not open .env file\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {

        // Strip BOM if present
        if (line.rfind("\xEF\xBB\xBF", 0) == 0) {
            std::cout << "[DEBUG] BOM detected, stripping...\n";
            line.erase(0, 3);
        }

        std::cout << "[DEBUG] LINE: [" << line << "]\n";

        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        // Trim whitespace
        while (!key.empty() && isspace(key.back())) key.pop_back();
        while (!val.empty() && isspace(val.front())) val.erase(0, 1);

        std::cout << "[DEBUG] Setting ENV: " << key << "=" << val << "\n";
        _putenv_s(key.c_str(), val.c_str());
    }
}

}
