// Channel.cpp
#include "fluxerpp/models/Channel.h"

namespace fluxerpp {
namespace models {

using util::Json;
using util::parse_snowflake;

PermissionOverwrite PermissionOverwrite::from_data(const Json& data) {
    PermissionOverwrite po;
    po.id = parse_snowflake(data, "id");
    po.type = data.value("type", 0);
    po.allow = data.value("allow", "0");
    po.deny = data.value("deny", "0");
    return po;
}

Channel Channel::from_data(const Json& data) {
    Channel c;
    c.id = parse_snowflake(data, "id");

    if (data.contains("guild_id") && !data["guild_id"].is_null()) {
        c.guild_id = parse_snowflake(data, "guild_id");
    }
    if (data.contains("parent_id") && !data["parent_id"].is_null()) {
        c.parent_id = parse_snowflake(data, "parent_id");
    }

    c.name = data.value("name", "");
    c.type = data.value("type", 0);
    c.position = data.value("position", 0);

    if (data.contains("topic") && !data["topic"].is_null()) {
        c.topic = data["topic"].get<std::string>();
    }
    c.nsfw = data.value("nsfw", false);

    if (data.contains("last_message_id") && !data["last_message_id"].is_null()) {
        c.last_message_id = parse_snowflake(data, "last_message_id");
    }
    if (data.contains("rate_limit_per_user") && !data["rate_limit_per_user"].is_null()) {
        c.rate_limit_per_user = data["rate_limit_per_user"].get<int>();
    }
    if (data.contains("permission_overwrites")) {
        for (const auto& po : data["permission_overwrites"]) {
            c.permission_overwrites.push_back(PermissionOverwrite::from_data(po));
        }
    }

    if (data.contains("bitrate") && !data["bitrate"].is_null()) {
        c.bitrate = data["bitrate"].get<int>();
    }
    if (data.contains("user_limit") && !data["user_limit"].is_null()) {
        c.user_limit = data["user_limit"].get<int>();
    }
    if (data.contains("rtc_region") && !data["rtc_region"].is_null()) {
        c.rtc_region = data["rtc_region"].get<std::string>();
    }
    if (data.contains("url") && !data["url"].is_null()) {
        c.url = data["url"].get<std::string>();
    }

    return c;
}

} // namespace models
} // namespace fluxerpp