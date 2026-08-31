#pragma once
#include <string>

namespace fluxerpp::models {

class PartialEmoji {
public:
    std::string name;

    // Convert PartialEmoji → std::string
    operator std::string() const {
        return name;
    }

    std::string to_string() const {
        return name;
    }
};

}
