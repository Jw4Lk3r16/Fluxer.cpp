// examples/ping_bot.cpp
//
// Ping bot demonstrating:
// - READY event
// - MESSAGE_CREATE event
// - Gateway latency (heartbeat ACK → now)
// - API latency (REST send time)

#include "fluxerpp/FluxerClient.h"
#include "fluxerpp/env.h"
#include "fluxerpp/util/Logger.h"
#include <cstdlib>
#include <iostream>
#include <atomic>
#include <chrono>

int main() {
    fluxerpp::load_env();

    const char* tokenEnv = std::getenv("FLUXER_TOKEN");
    if (!tokenEnv) {
        std::cerr << "FLUXER_TOKEN not set (check your .env)\n";
        return 1;
    }

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

    client.gateway().set_debug_logging(true);
    fluxerpp::util::Logger::instance().set_level(fluxerpp::util::LogLevel::Debug);

    // ============================================================
    // LATENCY TRACKING (gateway heartbeat ACK)
    // ============================================================
    int last_gateway_latency = 0;

    client.gateway().on_latency([&](int ms) {
        last_gateway_latency = ms;
    });
    // ============================================================

    client.gateway().on_ready([&client, channel_id]() {
        std::cout << "Ping bot is ready.\n";
        try {
            client.api().send_message(channel_id, nlohmann::json{{"content", "I'm online!"}});
        } catch (const std::exception& ex) {
            std::cerr << "Failed to send ready message: " << ex.what() << "\n";
        }
    });

    client.gateway().on_message_create([&client, &last_gateway_latency](const nlohmann::json& msg) {
        if (msg.value("content", "") == "!ping") {

            auto reply_channel_id = std::stoull(msg.at("channel_id").get<std::string>());

            // ============================================================
            // API LATENCY MEASUREMENT
            // ============================================================
            auto start = std::chrono::steady_clock::now();

            try {
                client.api().send_message(
                    reply_channel_id,
                    nlohmann::json{{"content", "Pong!"}}
                );
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send pong: " << ex.what() << "\n";
                return;
            }

            auto end = std::chrono::steady_clock::now();
            int api_latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            // ============================================================

            // Send combined latency message
            try {
                client.api().send_message(
                    reply_channel_id,
                    nlohmann::json{{"content",
                        "Pong! Gateway: " + std::to_string(last_gateway_latency) +
                        "ms | API: " + std::to_string(api_latency) + "ms"
                    }}
                );
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send latency message: " << ex.what() << "\n";
            }
        }
    });

    client.login(); // blocks
    return 0;
}
