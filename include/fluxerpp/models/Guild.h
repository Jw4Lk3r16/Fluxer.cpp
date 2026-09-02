#pragma once
// fluxerpp/models/Guild.h
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "fluxerpp/models/Channel.h"
#include "fluxerpp/models/Role.h"
#include "fluxerpp/models/Member.h"
#include "fluxerpp/util/Json.h"

namespace fluxerpp {

class RestClient;

namespace models {

class Guild {
public:
    std::uint64_t id{};
    std::string name;
    std::optional<std::string> icon;
    std::optional<std::string> banner;
    std::optional<std::string> splash;
    std::uint64_t owner_id{};
    std::optional<std::uint64_t> afk_channel_id;
    int afk_timeout{0};
    int verification_level{0};
    int mfa_level{0};
    int nsfw_level{0};
    std::optional<std::uint64_t> system_channel_id;
    std::vector<std::string> features;

    std::string joined_at;
    std::optional<int> member_count;

    std::vector<Channel> channels;
    std::vector<Role> roles;
    std::vector<Member> members;

    Guild() = default;

    // rest is threaded through to Member::from_data -> User::from_data for
    // each member; optional since a caller without a RestClient handy
    // (e.g. a unit test) can still parse a guild, just without any member
    // bound to a client for REST operations.
    static Guild from_data(const util::Json& data, RestClient* rest = nullptr);
};

} // namespace models
} // namespace fluxerpp