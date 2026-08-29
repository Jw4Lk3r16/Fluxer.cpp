#pragma once
#include "RestClient.h"
#include "GatewayClient.h"
#include "FluxerConfig.h"
#include <fluxerpp/nlohmann/json.hpp>

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

}
