#include "fluxerpp/FluxerClient.h"

namespace fluxerpp {

FluxerClient::FluxerClient(const FluxerConfig& cfg)
    : rest(cfg), gate(cfg.token) {
    // Lets GatewayClient::connect() resolve GET /gateway/bot instead of
    // dialing a hardcoded host. `rest` is declared before `gate` in
    // FluxerClient.h, so it's already fully constructed here.
    gate.bind_rest(&rest);
}

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