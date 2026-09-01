#include "fluxerpp/FluxerClient.h"

namespace fluxerpp {

FluxerClient::FluxerClient(const FluxerConfig& cfg)
    : rest(cfg), gate(cfg.token) {}

RestClient& FluxerClient::api() {
    return rest;
}

GatewayClient& FluxerClient::gateway() {
    return gate;
}

void FluxerClient::login() {
    gate.connect();
}

} // namespace fluxerpp