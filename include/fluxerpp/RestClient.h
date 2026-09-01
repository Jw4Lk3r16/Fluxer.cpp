#pragma once

#include "fluxerpp/FluxerConfig.h"
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>
#include <optional>
#include <mutex>

namespace fluxerpp {

class RestClient {
public:
    explicit RestClient(const FluxerConfig& cfg);
    ~RestClient() = default;

    // Generic request helper (throws on CURL error or unrecoverable HTTP error).
    // Automatically retries once on HTTP 429, honoring Retry-After if present.
    nlohmann::json request(const std::string& method,
                           const std::string& path,
                           const nlohmann::json* body = nullptr);

    nlohmann::json get(const std::string& path);
    nlohmann::json post(const std::string& path, const nlohmann::json& body);
    nlohmann::json patch(const std::string& path, const nlohmann::json& body);
    nlohmann::json del(const std::string& path);

    // --- Message-level helpers used by models::Message ---
    // These were called from Message.cpp but never declared here — that's
    // why the model API didn't compile. channel_id/message_id are snowflake
    // IDs (see util::parse_snowflake for the inverse direction).
    nlohmann::json send_message(std::uint64_t channel_id, const nlohmann::json& payload);
    nlohmann::json edit_message(std::uint64_t channel_id, std::uint64_t message_id, const nlohmann::json& payload);
    void delete_message(std::uint64_t channel_id, std::uint64_t message_id);

    void add_reaction(std::uint64_t channel_id, std::uint64_t message_id, const std::string& emoji);
    void delete_reaction(std::uint64_t channel_id, std::uint64_t message_id,
                         const std::string& emoji, const std::string& user = "@me");
    void delete_all_reactions(std::uint64_t channel_id, std::uint64_t message_id);
    void delete_all_reactions_for_emoji(std::uint64_t channel_id, std::uint64_t message_id, const std::string& emoji);

    void pin_message(std::uint64_t channel_id, std::uint64_t message_id);
    void unpin_message(std::uint64_t channel_id, std::uint64_t message_id);

    // Returns the authenticated bot's own user ID. Fetched via GET /users/@me
    // on first call and cached thereafter (mutex-guarded — GatewayClient's
    // receive thread and caller threads can both reach this through Message
    // methods). Message::_add_reaction/_remove_reaction use this to tell
    // whether a reaction update was caused by this bot itself.
    std::uint64_t user_id();

private:
    FluxerConfig config;

    struct RawResponse {
        long http_code;
        std::string body;
        std::string headers;
    };
    RawResponse perform(const std::string& method, const std::string& path, const nlohmann::json* body);
    static double parse_retry_after(const std::string& headers);
    static std::string url_encode_component(const std::string& s);

    std::optional<std::uint64_t> cached_user_id_;
    std::mutex user_id_mutex_;
};

} // namespace fluxerpp