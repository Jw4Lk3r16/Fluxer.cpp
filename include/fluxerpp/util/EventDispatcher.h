#pragma once
// fluxerpp/EventDispatcher.h
//
// Single source of truth for event callbacks. Previously GatewayClient also
// kept its own private ready_callbacks/message_callbacks vectors alongside
// this class, so callbacks registered via GatewayClient::on_ready() and
// callbacks registered via GatewayClient::dispatcher.on_ready() went to two
// different lists — only the former ever actually fired. GatewayClient now
// holds one EventDispatcher and forwards to it; nothing else should keep a
// second copy of these vectors.

#include <functional>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

namespace fluxerpp {

class EventDispatcher {
public:
    // Registration
    void on_ready(const std::function<void()>& cb);
    void on_message_create(const std::function<void(const nlohmann::json&)>& cb);

    // Invoked by GatewayClient when the corresponding DISPATCH arrives.
    // Exceptions thrown by user callbacks are caught and logged so one
    // misbehaving handler can't kill the receive loop / whole connection.
    void dispatch_ready();
    void dispatch_message_create(const nlohmann::json& data);

private:
    std::mutex mutex_;
    std::vector<std::function<void()>> ready_callbacks_;
    std::vector<std::function<void(const nlohmann::json&)>> message_callbacks_;
};

} // namespace fluxerpp