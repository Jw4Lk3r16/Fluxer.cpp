#pragma once
// fluxerpp/models/Channel.h
#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

struct PermissionOverwrite {
    std::uint64_t id{};  // role or member id, depending on `type`
    int type{0};         // 0 = role, 1 = member
    std::string allow;   // permission bitfield — kept as a string; it can
    std::string deny;    // exceed a safely-representable 64-bit value.

    static PermissionOverwrite from_data(const util::Json& data);
};

class Channel {
public:
    std::uint64_t id{};
    std::optional<std::uint64_t> guild_id;
    std::optional<std::uint64_t> parent_id;
    std::string name;
    int type{0};
    int position{0};
    std::optional<std::string> topic;
    bool nsfw{false};
    std::optional<std::uint64_t> last_message_id;
    std::optional<int> rate_limit_per_user;
    std::vector<PermissionOverwrite> permission_overwrites;

    // Voice-channel-specific — only meaningfully set for voice channel types.
    std::optional<int> bitrate;
    std::optional<int> user_limit;
    std::optional<std::string> rtc_region;

    // Link-style channels (type 998 was seen on the wire pointing at an
    // external URL rather than holding messages).
    std::optional<std::string> url;

    Channel() = default;

    static Channel from_data(const util::Json& data);
};

} // namespace models
} // namespace fluxerpp