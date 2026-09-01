#pragma once
// fluxerpp/GatewayClient.h
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "fluxerpp/EventDispatcher.h"

namespace fluxerpp {

class RestClient;

class GatewayClient {
public:
    std::string token;

    explicit GatewayClient(const std::string& token);

    // Blocks the calling thread, running the connect/heartbeat/reconnect
    // loop until a non-resumable close or max reconnect attempts is hit.
    //
    // Resolves the real gateway URL via GET /gateway/bot before connecting
    // (see bind_rest) instead of dialing a hardcoded host — mirroring what
    // WebSocketManager.connect() does in the JS client. If no RestClient is
    // bound, or the request fails, falls back to fallback_host below.
    void connect();

    // The single source of truth for callbacks — see EventDispatcher.h.
    EventDispatcher dispatcher;

    // Convenience wrappers so call sites can keep writing
    // gateway.on_ready(...) instead of gateway.dispatcher.on_ready(...).
    void on_ready(const std::function<void()>& cb);
    void on_message_create(const std::function<void(const nlohmann::json&)>& cb);

    // Enables verbose per-frame logging (RAW FRAME / DISPATCH tracer).
    // Off by default — this used to be unconditional std::cout spam.
    void set_debug_logging(bool enabled) { debug_logging_ = enabled; }

    // Required for connect() to resolve /gateway/bot. FluxerClient wires
    // this up automatically; set it manually if constructing GatewayClient
    // standalone.
    void bind_rest(RestClient* rest) { rest_ = rest; }

    int max_reconnect_attempts = 6;
    std::string gateway_version = "1";

    // Used only if /gateway/bot can't be resolved (no RestClient bound, or
    // the request failed) — last-resort fallback, not the primary path.
    std::string fallback_host = "gateway.fluxer.app";

private:
    bool debug_logging_ = false;
    RestClient* rest_{nullptr};
};

} // namespace fluxerpp