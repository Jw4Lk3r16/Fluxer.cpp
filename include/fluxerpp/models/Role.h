#pragma once
// fluxerpp/models/Role.h
#include <cstdint>
#include <string>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

class Role {
public:
    std::uint64_t id{};
    std::string name;
    int color{0};
    bool hoist{false};
    int position{0};
    std::string permissions; // bitfield, kept as a string — same reasoning as PermissionOverwrite
    bool mentionable{false};

    Role() = default;

    static Role from_data(const util::Json& data);
};

} // namespace models
} // namespace fluxerpp