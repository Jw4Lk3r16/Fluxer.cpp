#include "fluxerpp/GatewayClient.h"
#include <iostream>

namespace fluxerpp {

GatewayClient::GatewayClient(const FluxerConfig& cfg)
    : config(cfg) {}

void GatewayClient::connect() {
    std::cout << "Connecting to gateway..." << std::endl;
}

void GatewayClient::disconnect() {
    std::cout << "Disconnecting..." << std::endl;
}

} // namespace fluxerpp
