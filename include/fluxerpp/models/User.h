#pragma once
// fluxerpp/models/User.h
#include <cstdint>
#include <string>
#include <optional>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {

class RestClient;

namespace models {

class User {
public:
    std::uint64_t id{};
    std::string username;
    std::string discriminator;
    std::optional<std::string> global_name;
    std::optional<std::string> avatar;
    bool bot{false};

    User() = default;

    static User from_data(const util::Json& data, RestClient* rest);

    void bind_rest(RestClient* rest) { rest_ = rest; }
    RestClient* rest() const { return rest_; }

private:
    RestClient* rest_{nullptr};
};

} // namespace models
} // namespace fluxerpp