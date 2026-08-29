#pragma once
#include "FluxerConfig.h"

namespace fluxerpp {

class GatewayClient {
public:
    GatewayClient(const FluxerConfig& cfg);

    void connect();
    void disconnect();

private:
    FluxerConfig config;
};

} // namespace fluxerpp
