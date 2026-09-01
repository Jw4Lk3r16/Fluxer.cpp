#pragma once
#include "fluxerpp/RestClient.h"
#include "fluxerpp/GatewayClient.h"
#include "fluxerpp/FluxerConfig.h"
#include <nlohmann/json.hpp>

namespace fluxerpp {

class FluxerClient {
public:
    explicit FluxerClient(const FluxerConfig& cfg);

    void login();
    RestClient& api();
    GatewayClient& gateway();

private:
    RestClient rest;
    GatewayClient gate;
};

} // namespace fluxerpp