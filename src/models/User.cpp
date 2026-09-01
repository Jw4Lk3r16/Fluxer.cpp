// User.cpp
#include "fluxerpp/models/User.h"

namespace fluxerpp {
namespace models {

using util::Json;
using util::parse_snowflake;

User User::from_data(const Json& data, RestClient* rest) {
    User user;
    user.id = parse_snowflake(data, "id");
    user.username = data.value("username", "");
    user.discriminator = data.value("discriminator", "0");

    if (data.contains("global_name") && !data["global_name"].is_null()) {
        user.global_name = data["global_name"].get<std::string>();
    }
    if (data.contains("avatar") && !data["avatar"].is_null()) {
        user.avatar = data["avatar"].get<std::string>();
    }
    user.bot = data.value("bot", false);
    user.rest_ = rest;
    return user;
}

} // namespace models
} // namespace fluxerpp