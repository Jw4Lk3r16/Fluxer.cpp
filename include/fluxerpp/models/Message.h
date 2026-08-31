#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

#include "fluxerpp/models/User.h"
#include "fluxerpp/models/Embed.h"
#include "fluxerpp/models/Reaction.h"
#include "fluxerpp/models/Attachment.h"
#include "fluxerpp/models/PartialEmoji.h"
#include "fluxerpp/models/File.h"
#include "fluxerpp/models/Channel.h"
#include "fluxerpp/models/Guild.h"
#include "fluxerpp/util/Json.h"

namespace fluxerpp {

class RestClient;

namespace models {

class Message {
public:
    // Core fields
    std::uint64_t id{};
    std::uint64_t channel_id{};
    std::string content;
    User author;
    std::string timestamp;
    std::optional<std::string> edited_timestamp;

    std::vector<Embed> embeds;
    std::vector<Attachment> attachments;
    std::vector<User> mentions;
    bool pinned{false};
    std::vector<Reaction> reactions;

    std::shared_ptr<Message> referenced_message;

private:
    RestClient* rest_{nullptr};
    Channel* channel_{nullptr};
    Guild* guild_{nullptr};

public:
    Message() = default;

    // Construct from JSON
    static Message from_data(const util::Json& data, RestClient* rest);

    // Binding helpers
    void bind_rest(RestClient* rest) { rest_ = rest; }
    void bind_channel(Channel* ch) { channel_ = ch; }
    void bind_guild(Guild* g) { guild_ = g; }

    Channel* channel() const { return channel_; }
    Guild* guild() const { return guild_; }
    std::optional<std::uint64_t> guild_id() const;

    // Message operations (REST)
    Message send(
        const std::optional<std::string>& content = std::nullopt,
        const std::optional<Embed>& embed = std::nullopt,
        const std::optional<std::vector<Embed>>& embeds = std::nullopt,
        const std::optional<File>& file = std::nullopt,
        const std::optional<std::vector<File>>& files = std::nullopt,
        const util::Json& extra = util::Json::object()
    );

    Message reply(
        const std::optional<std::string>& content = std::nullopt,
        const std::optional<Embed>& embed = std::nullopt,
        const std::optional<std::vector<Embed>>& embeds = std::nullopt,
        const std::optional<File>& file = std::nullopt,
        const std::optional<std::vector<File>>& files = std::nullopt,
        const util::Json& extra = util::Json::object()
    );

    Message send_to_channel(
        std::uint64_t target_channel_id,
        const std::optional<std::string>& content = std::nullopt,
        const std::optional<Embed>& embed = std::nullopt,
        const std::optional<std::vector<Embed>>& embeds = std::nullopt,
        const std::optional<File>& file = std::nullopt,
        const std::optional<std::vector<File>>& files = std::nullopt,
        const util::Json& extra = util::Json::object()
    );

    Message edit(
        const std::optional<std::string>& content = std::nullopt,
        const util::Json& extra = util::Json::object()
    );

    void delete_message();

    // Reactions
    void add_reaction(const std::string& emoji);
    void add_reaction(const PartialEmoji& emoji);

    void remove_reaction(const std::string& emoji, const std::string& user = "@me");
    void remove_reaction(const PartialEmoji& emoji, const std::string& user = "@me");

    void clear_reactions();
    void clear_reaction(const std::string& emoji);
    void clear_reaction(const PartialEmoji& emoji);

    // Pinning
    void pin();
    void unpin();

    // Internal helpers (gateway reaction updates)
    Reaction _add_reaction(const util::Json& data, const PartialEmoji& emoji, std::uint64_t user_id);
    Reaction _remove_reaction(const util::Json& data, const PartialEmoji& emoji, std::uint64_t user_id);
    void _cache_guild(Guild* g);
    std::optional<Reaction> _clear_emoji(const PartialEmoji& emoji);
};

} // namespace models
} // namespace fluxerpp
