#pragma once
// fluxerpp/models/Member.h
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "fluxerpp/models/User.h"
#include "fluxerpp/util/Json.h"

namespace fluxerpp {

class RestClient;

namespace models {

class Member {
public:
    User user;
    std::optional<std::string> nick;
    std::vector<std::uint64_t> role_ids;
    std::string joined_at;
    bool deaf{false};
    bool mute{false};

    Member() = default;

    static Member from_data(const util::Json& data, RestClient* rest);
};

} // namespace models
} // namespace fluxerpp