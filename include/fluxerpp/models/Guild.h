#pragma once
// fluxerpp/models/Guild.h
//
// Deliberately minimal: Message only needs Guild::id and a pointer to bind
// against (see Message::_cache_guild). A full guild model — roles, members,
// channels — is a separate, larger piece of work and shouldn't block
// getting Message.cpp compiling.
#include <cstdint>
#include <string>

namespace fluxerpp {
namespace models {

class Guild {
public:
    std::uint64_t id{};
    std::string name;
};

} // namespace models
} // namespace fluxerpp