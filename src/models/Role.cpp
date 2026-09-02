// Role.cpp
#include "fluxerpp/models/Role.h"

namespace fluxerpp {
namespace models {

using util::Json;
using util::parse_snowflake;

Role Role::from_data(const Json& data) {
    Role r;
    r.id = parse_snowflake(data, "id");
    r.name = data.value("name", "");
    r.color = data.value("color", 0);
    r.hoist = data.value("hoist", false);
    r.position = data.value("position", 0);
    r.permissions = data.value("permissions", "0");
    r.mentionable = data.value("mentionable", false);
    return r;
}

} // namespace models
} // namespace fluxerpp