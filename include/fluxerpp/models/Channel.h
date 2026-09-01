#pragma once
// fluxerpp/models/Channel.h
//
// Deliberately minimal — see Guild.h for the same rationale. Message only
// needs a pointer to bind against.
#include <cstdint>
#include <string>

namespace fluxerpp {
namespace models {

class Channel {
public:
    std::uint64_t id{};
    std::string name;
};

} // namespace models
} // namespace fluxerpp