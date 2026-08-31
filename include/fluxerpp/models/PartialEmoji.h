#pragma once
#include <string>

namespace fluxerpp::models {
class PartialEmoji {
public:
    std::string name;

    std::string to_string() const { return name; }
};
}
