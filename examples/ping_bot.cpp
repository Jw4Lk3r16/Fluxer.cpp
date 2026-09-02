// examples/ping_bot.cpp
//
// Ping bot demonstrating:
// - READY event
// - MESSAGE_CREATE event
// - Gateway latency (heartbeat ACK → now)
// - API latency (REST send time)
// - Embeds + fields using your model system

#include "fluxerpp/FluxerClient.h"
#include "fluxerpp/env.h"
#include "fluxerpp/util/Logger.h"
#include "fluxerpp/models/Embed.h"
#include "fluxerpp/models/EmbedField.h"
#include "fluxerpp/models/Message.h"

#include "fluxerpp/builders/EmbedBuilder.h"
#include "fluxerpp/builders/EmbedAuthorBuilder.h"
#include "fluxerpp/builders/EmbedFooterBuilder.h"

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

    client.gateway().on_message_create([&](const nlohmann::json& raw) {

        // Convert raw JSON → your Message model
        fluxerpp::models::Message msg =
            fluxerpp::models::Message::from_data(raw, &client.api());

        // ============================================================
        // !ping command
        // ============================================================
        if (msg.content == "!ping") {

            auto reply_channel_id = msg.channel_id;

            // API latency measurement
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
            int api_latency =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

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

        // ============================================================
        // !Hello command
        // ============================================================
        } else if (msg.content == "!Hello") {

            auto reply_channel_id = msg.channel_id;
            std::string mention = "<@" + std::to_string(msg.author.id) + ">";

            client.api().send_message(
                reply_channel_id,
                nlohmann::json{{"content", "Hello " + mention}}
            );

        // ============================================================
        // !embed command — latency embed
        // ============================================================
        } else if (msg.content == "!embed") {

            using namespace fluxerpp::builders;

            auto embed = EmbedBuilder()
                .set_title("Fluxer++ Embed")
                .set_description("Cinematic embed builder activated.")
                .set_color(0x5865F2)
                .set_author("PixelMan", "https://fluxerusercontent.com/attachments/1538056519919079424/1543056191670718464/image.png")
                .set_footer("Generated by Fluxer++", "https://fluxerusercontent.com/attachments/1538056519919079424/1543056191670718464/image.png")
                .set_thumbnail("https://fluxerusercontent.com/attachments/1538056519919079424/1543056191670718464/image.png")
                .set_image("https://fluxerusercontent.com/attachments/1538056519919079424/1543056191670718464/image.png")
                .add_field("Latency", "42ms", true)
                .add_field("Gateway", "Connected", true)
                .build();

            msg.reply(std::nullopt, embed);


        }
    });

    client.login(); // blocks
    return 0;
}
