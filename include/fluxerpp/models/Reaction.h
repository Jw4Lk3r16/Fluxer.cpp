#pragma once
#include <string>
#include <cstdint>

namespace fluxerpp::models {
class Message; // forward declare

class Reaction {
public:
    std::string emoji;
    int count = 0;
    bool me = false;

    void bind_message(Message*) {}
    void bind_rest(void*) {}
};
}
