#pragma once
// fluxerpp/FluxerConfig.h
#include <string>

namespace fluxerpp {

struct FluxerConfig {
    std::string token;
    std::string restBase = "https://api.fluxer.app/v1";
    std::string gatewayUrl = "wss://gateway.fluxer.app/?v=1&encoding=json";
};

} // namespace fluxerpp