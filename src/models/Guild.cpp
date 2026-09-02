// Guild.cpp
#include "fluxerpp/models/Guild.h"

namespace fluxerpp {
namespace models {

using util::Json;
using util::parse_snowflake;

Guild Guild::from_data(const Json& data, RestClient* rest) {
    Guild g;

    // Most identifying fields (name, icon, owner_id, verification_level,
    // etc.) live under a nested "properties" object in the payload this
    // targets, not flat on "d" — confirmed against a real GUILD_CREATE
    // capture, not guessed. Top-level "id" is also present and used as a
    // fallback if "properties" is ever missing.
    const Json* props = data.contains("properties") ? &data["properties"] : nullptr;

    if (data.contains("id") && !data["id"].is_null()) {
        g.id = parse_snowflake(data, "id");
    } else if (props) {
        g.id = parse_snowflake(*props, "id");
    }

    if (props) {
        g.name = props->value("name", "");

        if (props->contains("icon") && !(*props)["icon"].is_null()) {
            g.icon = (*props)["icon"].get<std::string>();
        }
        if (props->contains("banner") && !(*props)["banner"].is_null()) {
            g.banner = (*props)["banner"].get<std::string>();
        }
        if (props->contains("splash") && !(*props)["splash"].is_null()) {
            g.splash = (*props)["splash"].get<std::string>();
        }
        if (props->contains("owner_id") && !(*props)["owner_id"].is_null()) {
            g.owner_id = parse_snowflake(*props, "owner_id");
        }
        if (props->contains("afk_channel_id") && !(*props)["afk_channel_id"].is_null()) {
            g.afk_channel_id = parse_snowflake(*props, "afk_channel_id");
        }
        g.afk_timeout = props->value("afk_timeout", 0);
        g.verification_level = props->value("verification_level", 0);
        g.mfa_level = props->value("mfa_level", 0);
        g.nsfw_level = props->value("nsfw_level", 0);

        if (props->contains("system_channel_id") && !(*props)["system_channel_id"].is_null()) {
            g.system_channel_id = parse_snowflake(*props, "system_channel_id");
        }
        if (props->contains("features")) {
            for (const auto& f : (*props)["features"]) {
                g.features.push_back(f.get<std::string>());
            }
        }
    }

    g.joined_at = data.value("joined_at", "");
    if (data.contains("member_count") && !data["member_count"].is_null()) {
        g.member_count = data["member_count"].get<int>();
    }

    if (data.contains("channels")) {
        for (const auto& c : data["channels"]) {
            g.channels.push_back(Channel::from_data(c));
        }
    }
    if (data.contains("roles")) {
        for (const auto& r : data["roles"]) {
            g.roles.push_back(Role::from_data(r));
        }
    }
    if (data.contains("members")) {
        for (const auto& m : data["members"]) {
            g.members.push_back(Member::from_data(m, rest));
        }
    }

    return g;
}

} // namespace models
} // namespace fluxerpp