#include <iostream>
#include <string>
#include <cstdlib>
#include <curl/curl.h>                 // add
#include "fluxerpp/env.h"
#include "fluxerpp/FluxerClient.h"
#include "fluxerpp/FluxerConfig.h"
#include "fluxerpp/RestClient.h"       // add

int main() {
    std::cout << "=== Fluxer++ Ping Bot Debugger ===\n";

    // Initialize libcurl once at program start
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        std::cerr << "[REST] curl_global_init failed\n";
        return 1;
    }

    // Load .env
    std::cout << "[DEBUG] Loading .env...\n";
    fluxerpp::load_env(".env");

    const char* raw = std::getenv("TOKEN");
    std::cout << "[DEBUG] getenv(\"TOKEN\") returned: "
              << (raw ? raw : "NULL") << "\n";

    if (!raw || std::string(raw).empty()) {
        std::cerr << "[Fluxer++] ERROR: TOKEN missing in .env\n";
        curl_global_cleanup();
        return 1;
    }

    fluxerpp::FluxerConfig cfg;
    cfg.token = raw;
    cfg.restBase = "https://api.fluxer.app"; // set your REST base

    std::cout << "[DEBUG] FluxerConfig.token = " << cfg.token << "\n";

    fluxerpp::FluxerClient client(cfg);

    // READY event: send a message when the gateway is ready
    client.gateway().on_ready([&]() {
        std::cout << "[Fluxer++] on_ready() invoked\n";
        std::string channel_id = "1538095086615658496";

        try {
            fluxerpp::RestClient rc(cfg);
            nlohmann::json body;
            body["content"] = "I am alive";
            auto resp = rc.post("/api/channels/" + channel_id + "/messages", body);
            std::cout << "[REST] Message post response: " << resp.dump() << "\n";
        } catch (const std::exception& ex) {
            std::cerr << "[REST] Failed to post message: " << ex.what() << "\n";
        }
    });

    client.gateway().on_message_create([&](const nlohmann::json& msg) {
        try {
            std::string author = msg.value("author", nlohmann::json::object()).value("username", std::string("unknown"));
            std::string content = msg.value("content", std::string());
            std::cout << "[MessageCreate] " << author << ": " << content << "\n";
        } catch (...) {
            std::cout << "[MessageCreate] (failed to parse message)\n";
        }
    });

    std::cout << "[DEBUG] Calling client.login()...\n";
    client.login();

    std::cout << "[DEBUG] login() returned — bot may be running async.\n";

    // Program lifetime continues; when exiting, cleanup curl
    // If your program never returns from login loop, you may not reach this line.
    curl_global_cleanup();
    return 0;
}
