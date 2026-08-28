#include <iostream>
#include <fluxerpp/FluxerClient.h>

int main() {
    fluxerpp::FluxerConfig cfg;
    cfg.token = std::getenv("FLUXER_BOT_TOKEN");

    if (!cfg.token.size()) {
        std::cerr << "Missing FLUXER_BOT_TOKEN environment variable\n";
        return 1;
    }

    fluxerpp::FluxerClient client(cfg);

    try {
        auto result = client.api().get("/users/@me");
        std::cout << "Bot info:\n" << result.dump(4) << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "REST error: " << e.what() << "\n";
    }

    return 0;
}
