// examples/ping_bot.cpp
//
// Minimal ping bot demonstrating GatewayClient usage, including the
// verbose per-frame debug tracing (RAW FRAME / DISPATCH t=/op=) that
// previously lived in a second, conflicting copy of GatewayClient in this
// file. That's gone now — set_debug_logging(true) turns on the same
// tracing inside the real GatewayClient implementation.

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

    fluxerpp::FluxerConfig cfg;
    cfg.token = tokenEnv;

    fluxerpp::FluxerClient client(cfg);

    // Turn on RAW FRAME / DISPATCH tracing for debugging. Off by default.
    client.gateway().set_debug_logging(true);
    fluxerpp::util::Logger::instance().set_level(fluxerpp::util::LogLevel::Debug);

    client.gateway().on_ready([]() {
        std::cout << "Ping bot is ready.\n";
    });

    client.gateway().on_message_create([&client](const nlohmann::json& msg) {
        if (msg.value("content", "") == "!ping") {
            auto channel_id = std::stoull(msg.at("channel_id").get<std::string>());
            try {
                client.api().post(
                    "/channels/" + std::to_string(channel_id) + "/messages",
                    nlohmann::json{{"content", "Pong!"}}
                );
            } catch (const std::exception& ex) {
                std::cerr << "Failed to send pong: " << ex.what() << "\n";
            }
        }
    });

    client.login(); // blocks
    return 0;
}