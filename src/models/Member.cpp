// Member.cpp
#include "fluxerpp/models/Member.h"

namespace fluxerpp {
namespace models {

using util::Json;

Member Member::from_data(const Json& data, RestClient* rest) {
    Member m;

    if (data.contains("user") && !data["user"].is_null()) {
        m.user = User::from_data(data["user"], rest);
    }
    if (data.contains("nick") && !data["nick"].is_null()) {
        m.nick = data["nick"].get<std::string>();
    }
    if (data.contains("roles")) {
        for (const auto& r : data["roles"]) {
            m.role_ids.push_back(std::stoull(r.get<std::string>()));
        }
    }
    m.joined_at = data.value("joined_at", "");
    m.deaf = data.value("deaf", false);
    m.mute = data.value("mute", false);

    return m;
}

} // namespace models
} // namespace fluxerpp