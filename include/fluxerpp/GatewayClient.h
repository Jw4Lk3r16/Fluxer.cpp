#pragma once
// fluxerpp/GatewayClient.h
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include "fluxerpp/EventDispatcher.h"

namespace fluxerpp {

class GatewayClient {
public:
    std::string token;

    explicit GatewayClient(const std::string& token);

    // Blocks the calling thread, running the connect/heartbeat/reconnect
    // loop until a non-resumable close or max reconnect attempts is hit.
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

    int max_reconnect_attempts = 6;

private:
    bool debug_logging_ = false;
};

} // namespace fluxerpp