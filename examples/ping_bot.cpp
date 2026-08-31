#include <iostream>
#include <string>
#include <cstdlib>

#include "fluxerpp/env.h"
#include "fluxerpp/FluxerClient.h"
#include "fluxerpp/FluxerConfig.h"

// Your WinHTTP sender
bool send_message_winhttp(const std::string& token,
                          const std::string& channel_id,
                          const std::string& content);

int main() {
    // Load .env
    fluxerpp::load_env(".env");

    const char* token = std::getenv("TOKEN");
    if (!token || std::string(token).empty()) {
        std::cerr << "[Fluxer++] ERROR: TOKEN missing in .env\n";
        return 1;
    }

    fluxerpp::FluxerConfig cfg;
    cfg.token = token;

    fluxerpp::FluxerClient client(cfg);

    // READY event
    client.gateway().on_ready([&]() {
        std::cout << "[Fluxer++] READY — sending alive message\n";

        std::string channel_id = "123456789012345678"; // replace

        bool ok = send_message_winhttp(token, channel_id, "I am alive");
        if (!ok) {
            std::cerr << "[Fluxer++] Failed to send message\n";
        }
    });

    client.login();
    return 0;
}
