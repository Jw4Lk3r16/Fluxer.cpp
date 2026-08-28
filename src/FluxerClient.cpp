#include "fluxerpp/FluxerClient.h"

namespace fluxerpp {

FluxerClient::FluxerClient(const FluxerConfig& cfg)
    : rest(cfg), gate(cfg) {}

void FluxerClient::login() {
    gate.connect();
}

RestClient& FluxerClient::api() {
    return rest;
}

GatewayClient& FluxerClient::gateway() {
    return gate;
}

}
