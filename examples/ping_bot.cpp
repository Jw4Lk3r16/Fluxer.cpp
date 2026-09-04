// examples/ping_bot.cpp
//
// Ping bot demonstrating:
// - READY event
// - MESSAGE_CREATE event (now hands back a parsed fluxerpp::models::Message
//   directly — see EventDispatcher's change)
// - Gateway latency (heartbeat ACK -> now)
// - API latency (REST send time)
// - Embeds + fields using the model/builder system

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
    // std::atomic even though on_latency and on_message_create currently
    // both fire on the same thread (GatewayClient's receive loop never
    // hands dispatch off to the heartbeat sender thread) — so there's no
    // *live* race today. Making it atomic costs nothing and stops being a
    // correct assumption the moment that internal detail ever changes.
    std::atomic<int> last_gateway_latency{0};

    client.gateway().on_latency([&](int ms) {
        last_gateway_latency.store(ms);
    });
    // ============================================================

    client.gateway().on_ready([&client, channel_id]() {
        std::cout << "Ping bot is ready.\n";
        try {
            client.api().send_message(channel_id, "I'm online!");
        } catch (const std::exception& ex) {
            std::cerr << "Failed to send ready message: " << ex.what() << "\n";
        }
    });

    client.gateway().on_message_create([&](const fluxerpp::models::Message& msg) {

        // Without this, the bot answers its own "!ping" reply forever —
        // matters more here than in real code, since examples get copied
        // by people who don't know to add it yet.
        if (msg.author.bot) return;

        // ============================================================
        // !ping command
        // ============================================================
        if (msg.content == "!ping") {

            auto reply_channel_id = msg.channel_id;

            // API latency measurement
            auto start = std::chrono::steady_clock::now();

            try {
                client.api().send_message(reply_channel_id, "Pong!");
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
                    "Pong! Gateway: " + std::to_string(last_gateway_latency.load()) +
                    "ms | API: " + std::to_string(api_latency) + "ms"
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

            try {
                client.api().send_message(reply_channel_id, "Hello " + mention);
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send hello: " << ex.what() << "\n";
            }

        // ============================================================
        // !embed command — latency embed
        // ============================================================
        } else if (msg.content == "!embed") {

            using namespace fluxerpp::builders;

            auto embed = EmbedBuilder()
                .set_title("Current Spotlight")
                .set_description("Every week I will choose a spotlight how you get on here is up to you.")
                .set_color(0x5865F2)
                .set_author("PixelMan")
                .add_field("Current Spotlight: ", "<@1542684459030032384>", true)
                .build();

            try {
                msg.reply(embed);
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send embed: " << ex.what() << "\n";
            }
        }
    });

    client.login(); // blocks
    return 0;
}