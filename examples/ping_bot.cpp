// examples/ping_bot.cpp
//
// Minimal ping bot demonstrating GatewayClient usage: connects, sends a
// message to a configured channel as soon as READY fires, and replies to
// "!ping" in that channel. Also shows the verbose per-frame debug tracing
// (RAW FRAME / DISPATCH t=/op=) via set_debug_logging(true).

#include "fluxerpp/FluxerClient.h"
#include "fluxerpp/env.h"
#include "fluxerpp/util/Logger.h"
#include <cstdlib>
#include <iostream>

int main() {
    fluxerpp::load_env();

    const char* tokenEnv = std::getenv("FLUXER_TOKEN");
    if (!tokenEnv) {
        std::cerr << "FLUXER_TOKEN not set (check your .env)\n";
        return 1;
    }

    // Where to send the on-ready message. Hardcoding this is a deliberate
    // shortcut for this example — GUILD_CREATE isn't routed into any guild
    // /channel cache yet (that's separate, bigger follow-up work), so there
    // is currently no way to discover "a channel in my server" at runtime.
    // Grab a channel ID from your server (right-click a channel -> Copy ID,
    // with developer mode on) and put it in .env as FLUXER_CHANNEL_ID.
    const char* channelEnv = std::getenv("FLUXER_CHANNEL_ID");
    if (!channelEnv) {
        std::cerr << "FLUXER_CHANNEL_ID not set (check your .env)\n";
        return 1;
    }
    std::uint64_t channel_id;
    try {
        channel_id = std::stoull(channelEnv);
    } catch (const std::exception& ex) {
        std::cerr << "FLUXER_CHANNEL_ID is not a valid snowflake: " << ex.what() << "\n";
        return 1;
    }

    fluxerpp::FluxerConfig cfg;
    cfg.token = tokenEnv;

    fluxerpp::FluxerClient client(cfg);

    // Turn on RAW FRAME / DISPATCH tracing for debugging. Off by default.
    client.gateway().set_debug_logging(true);
    fluxerpp::util::Logger::instance().set_level(fluxerpp::util::LogLevel::Debug);

    client.gateway().on_ready([&client, channel_id]() {
        std::cout << "Ping bot is ready.\n";
        // NOTE: this runs synchronously on the gateway's receive thread, so
        // the WebSocket receive loop is paused for the duration of this one
        // REST call — fine for a single quick send like this, but a slow or
        // long-running on_ready handler would delay heartbeats and message
        // processing. Move real work to its own thread if that ever matters.
        try {
            client.api().send_message(channel_id, nlohmann::json{{"content", "I'm online!"}});
        } catch (const std::exception& ex) {
            std::cerr << "Failed to send ready message: " << ex.what() << "\n";
        }
    });

    client.gateway().on_message_create([&client](const nlohmann::json& msg) {
        if (msg.value("content", "") == "!ping") {
            auto reply_channel_id = std::stoull(msg.at("channel_id").get<std::string>());
            try {
                client.api().send_message(reply_channel_id, nlohmann::json{{"content", "Pong!"}});
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send pong: " << ex.what() << "\n";
            }
        }
    });

    client.login(); // blocks
    return 0;
}