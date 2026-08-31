#include "fluxerpp/models/Message.h"
#include "fluxerpp/RestClient.h"
#include "fluxerpp/util/Json.h"

namespace fluxerpp {
namespace models {

using util::Json;

// optional guild_id
std::optional<std::uint64_t> Message::guild_id() const {
    if (guild_) return guild_->id;
    return std::nullopt;
}

// static constructor from JSON
Message Message::from_data(const Json& data, RestClient* rest) {
    Message msg;

    msg.id = std::stoull(data.at("id").get<std::string>());
    msg.channel_id = std::stoull(data.at("channel_id").get<std::string>());
    msg.content = data.value("content", "");
    msg.timestamp = data.at("timestamp").get<std::string>();

    if (data.contains("edited_timestamp") && !data["edited_timestamp"].is_null()) {
        msg.edited_timestamp = data["edited_timestamp"].get<std::string>();
    }

    // author
    msg.author = User::from_data(data.at("author"), rest);

    // mentions
    if (data.contains("mentions")) {
        for (const auto& u : data["mentions"]) {
            msg.mentions.push_back(User::from_data(u, rest));
        }
    }

    // attachments
    if (data.contains("attachments")) {
        for (const auto& a : data["attachments"]) {
            msg.attachments.push_back(Attachment::from_data(a));
        }
    }

    // embeds
    if (data.contains("embeds")) {
        for (const auto& e : data["embeds"]) {
            msg.embeds.push_back(Embed::from_data(e));
        }
    }

    msg.pinned = data.value("pinned", false);

    // bind rest
    msg.rest_ = rest;

    // referenced_message
    if (data.contains("referenced_message") && !data["referenced_message"].is_null()) {
        auto ref = std::make_shared<Message>(
            Message::from_data(data["referenced_message"], rest)
        );
        msg.referenced_message = ref;
    }

    // reactions
    if (data.contains("reactions")) {
        for (const auto& r : data["reactions"]) {
            msg.reactions.push_back(
                Reaction::from_data(r, rest, &msg)
            );
        }
    }

    return msg;
}

// helper: build file list JSON
static std::optional<std::vector<Json>> build_file_list(
    const std::optional<File>& file,
    const std::optional<std::vector<File>>& files
) {
    if (file.has_value()) {
        return std::vector<Json>{ file->to_dict() };
    }
    if (files.has_value()) {
        std::vector<Json> out;
        out.reserve(files->size());
        for (const auto& f : *files) {
            out.push_back(f.to_dict());
        }
        return out;
    }
    return std::nullopt;
}

// helper: convert embeds to JSON
static std::optional<std::vector<Json>> build_embed_list(
    const std::optional<Embed>& embed,
    const std::optional<std::vector<Embed>>& embeds
) {
    if (embed.has_value()) {
        return std::vector<Json>{ embed->to_dict() };
    }
    if (embeds.has_value()) {
        std::vector<Json> out;
        out.reserve(embeds->size());
        for (const auto& e : *embeds) {
            out.push_back(e.to_dict());
        }
        return out;
    }
    return std::nullopt;
}

// --- send (same channel, no reply) ---
Message Message::send(
    const std::optional<std::string>& content,
    const std::optional<Embed>& embed,
    const std::optional<std::vector<Embed>>& embeds,
    const std::optional<File>& file,
    const std::optional<std::vector<File>>& files,
    const Json& extra
) {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }

    Json payload = extra;

    // content
    if (content.has_value()) {
        payload["content"] = *content;
    }

    // embeds
    if (auto embed_list = build_embed_list(embed, embeds)) {
        payload["embeds"] = *embed_list;
    }

    // files
    if (auto file_list = build_file_list(file, files)) {
        payload["files"] = *file_list;
    }

    // perform REST call
    Json data = rest_->send_message(channel_id, payload);

    Message msg = Message::from_data(data, rest_);
    msg.channel_ = channel_;
    msg._cache_guild(guild_);
    return msg;
}

// --- reply (message_reference) ---
Message Message::reply(
    const std::optional<std::string>& content,
    const std::optional<Embed>& embed,
    const std::optional<std::vector<Embed>>& embeds,
    const std::optional<File>& file,
    const std::optional<std::vector<File>>& files,
    const Json& extra
) {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }

    Json payload = extra;

    if (content.has_value()) {
        payload["content"] = *content;
    }

    if (auto embed_list = build_embed_list(embed, embeds)) {
        payload["embeds"] = *embed_list;
    }

    if (auto file_list = build_file_list(file, files)) {
        payload["files"] = *file_list;
    }

    // message_reference
    Json message_reference;
    message_reference["message_id"] = std::to_string(id);
    message_reference["channel_id"] = std::to_string(channel_id);
    if (auto gid = guild_id()) {
        message_reference["guild_id"] = std::to_string(*gid);
    }
    payload["message_reference"] = message_reference;

    Json data = rest_->send_message(channel_id, payload);

    Message msg = Message::from_data(data, rest_);
    msg.channel_ = channel_;
    msg._cache_guild(guild_);
    return msg;
}

// --- send_to_channel (different channel) ---
Message Message::send_to_channel(
    std::uint64_t target_channel_id,
    const std::optional<std::string>& content,
    const std::optional<Embed>& embed,
    const std::optional<std::vector<Embed>>& embeds,
    const std::optional<File>& file,
    const std::optional<std::vector<File>>& files,
    const Json& extra
) {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }

    Json payload = extra;

    if (content.has_value()) {
        payload["content"] = *content;
    }

    if (auto embed_list = build_embed_list(embed, embeds)) {
        payload["embeds"] = *embed_list;
    }

    if (auto file_list = build_file_list(file, files)) {
        payload["files"] = *file_list;
    }

    Json data = rest_->send_message(target_channel_id, payload);

    Message msg = Message::from_data(data, rest_);
    msg._cache_guild(guild_);
    return msg;
}

// --- edit ---
Message Message::edit(
    const std::optional<std::string>& content,
    const Json& extra
) {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }

    Json payload = extra;
    if (content.has_value()) {
        payload["content"] = *content;
    }

    Json data = rest_->edit_message(channel_id, id, payload);

    Message msg = Message::from_data(data, rest_);
    msg.channel_ = channel_;
    msg._cache_guild(guild_);
    return msg;
}

// --- delete ---
void Message::delete_message() {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }
    rest_->delete_message(channel_id, id);
}

// --- reactions (public) ---
void Message::add_reaction(const std::string& emoji) {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }
    rest_->add_reaction(channel_id, id, emoji);
}

void Message::add_reaction(const PartialEmoji& emoji) {
    add_reaction(emoji.to_string());
}

void Message::remove_reaction(const std::string& emoji, const std::string& user) {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }
    rest_->delete_reaction(channel_id, id, emoji, user);
}

void Message::remove_reaction(const PartialEmoji& emoji, const std::string& user) {
    remove_reaction(emoji.to_string(), user);
}

void Message::clear_reactions() {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }
    rest_->delete_all_reactions(channel_id, id);
}

void Message::clear_reaction(const std::string& emoji) {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }
    rest_->delete_all_reactions_for_emoji(channel_id, id, emoji);
}

void Message::clear_reaction(const PartialEmoji& emoji) {
    clear_reaction(emoji.to_string());
}

// --- pin / unpin ---
void Message::pin() {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }
    rest_->pin_message(channel_id, id);
    pinned = true;
}

void Message::unpin() {
    if (!rest_) {
        throw std::runtime_error("Message is not bound to a RestClient");
    }
    rest_->unpin_message(channel_id, id);
    pinned = false;
}

// --- internal reaction helpers ---
Reaction Message::_add_reaction(const Json& /*data*/,
                                const PartialEmoji& emoji,
                                std::uint64_t user_id) {
    // find existing
    for (auto& reaction : reactions) {
        if (reaction.emoji == emoji.to_string()) {
            reaction.count += 1;

            if (rest_ && user_id == rest_->user_id()) {
                reaction.me = true;
            }

            return reaction;
        }
    }


    // new reaction
    Reaction reaction;
    reaction.emoji = emoji;
    reaction.count = 1;
    reaction.me = (rest_ && user_id == rest_->user_id());
    reaction.bind_message(this);
    reaction.bind_rest(rest_);

    reactions.push_back(reaction);
    return reactions.back();
}

Reaction Message::_remove_reaction(const Json& /*data*/,
                                   const PartialEmoji& emoji,
                                   std::uint64_t user_id) {
    for (std::size_t i = 0; i < reactions.size(); ++i) {
        auto& reaction = reactions[i];

        if (reaction.emoji == emoji.to_string()) {
            reaction.count -= 1;

            if (rest_ && user_id == rest_->user_id()) {
                reaction.me = false;
            }

            if (reaction.count <= 0) {
                Reaction removed = reaction;
                reactions.erase(reactions.begin() + i);
                return removed;
            }

            return reaction;
        }
    }
    throw std::runtime_error("Reaction not found on message");
}

// --- cache guild (for replies) ---
void Message::_cache_guild(Guild* g) {
    guild_ = g;
    if (referenced_message) {
        referenced_message->guild_ = g;
    }
}

// --- clear emoji internal ---
std::optional<Reaction> Message::_clear_emoji(const PartialEmoji& emoji) {
    for (std::size_t i = 0; i < reactions.size(); ++i) {
        if (reactions[i].emoji == emoji.to_string()) {
            Reaction removed = reactions[i];
            reactions.erase(reactions.begin() + i);
            return removed;
        }
    }
    return std::nullopt;
}

} // namespace models
} // namespace fluxerpp
