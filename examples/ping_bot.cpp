#include <iostream>
#include "fluxerpp/FluxerClient.h"
#include "fluxerpp/util/Json.h"

// Simple .env loader (see below)
std::string load_env(const std::string& key);

int main() {
    // Load token from .env
    std::string token = load_env("FLUXER_BOT_TOKEN");
    if (token.empty()) {
        std::cerr << "[Fluxer++] ERROR: TOKEN not found in .env" << std::endl;
        return 1;
    }

    fluxerpp::FluxerClient client(token);

    client.on_ready([&]() {
        std::cout << "[Fluxer++] Gateway READY — sending alive message" << std::endl;

        std::uint64_t channel_id = 123456789012345678; // replace with your channel ID

        client.rest().send_message(channel_id, {
            {"content", "I am alive"}
        });
    });

    client.run();
    return 0;
}
