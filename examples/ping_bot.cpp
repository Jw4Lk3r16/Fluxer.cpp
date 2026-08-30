#include <iostream>
#include <fluxerpp/FluxerClient.h>
#include <fluxerpp/env.h>

int main() {
    // Load .env file
    fluxerpp::load_env(".env");

    fluxerpp::FluxerConfig cfg;

    const char* token = std::getenv("FLUXER_BOT_TOKEN");
    if (!token || std::string(token).empty()) {
        std::cerr << "Missing FLUXER_BOT_TOKEN in .env or environment\n";
        return 1;
    }

    cfg.token = token;

    fluxerpp::FluxerClient client(cfg);

    try {
        auto result = client.api().get("/users/@me");
        std::cout << "Bot info:\n" << result.dump(4) << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "REST error: " << e.what() << "\n";
    }

    std::cout << "Press Enter to exit...";
    std::cin.get();

    return 0;
}
