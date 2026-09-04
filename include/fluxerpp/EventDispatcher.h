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
#include "fluxerpp/models/Guild.h"
#include "fluxerpp/models/Message.h"

namespace fluxerpp {

class EventDispatcher {
public:
    // Registration
    void on_ready(const std::function<void()>& cb);
    // Hands back a fully parsed Message, not the raw gateway payload —
    // every handler was otherwise repeating
    // `models::Message::from_data(raw, &client.api())` as its first two
    // lines. See dispatch_message_create's comment on the lifetime caveat
    // this doesn't solve.
    void on_message_create(const std::function<void(const models::Message&)>& cb);
    void on_guild_create(const std::function<void(const models::Guild&)>& cb);
    // Fired on every HEARTBEAT ACK with the measured round-trip in ms.
    void on_latency(const std::function<void(int)>& cb);
    // Fired on every HEARTBEAT ACK, no payload — for callers that just want
    // a liveness signal without caring about the exact latency value.
    void on_heartbeat_ack(const std::function<void()>& cb);

    // Invoked by GatewayClient when the corresponding DISPATCH arrives.
    // Exceptions thrown by user callbacks are caught and logged so one
    // misbehaving handler can't kill the receive loop / whole connection.
    void dispatch_ready();
    // NOTE on Message's lifetime: Message::rest_ (bound in from_data) is a
    // raw, non-owning RestClient*. Nothing here — or in Message itself —
    // stops a caller from moving this Message out of the callback and using
    // it after the owning FluxerClient is destroyed, which would dereference
    // a dangling pointer. A real fix needs an ownership change (RestClient
    // held via shared_ptr, Message holding a weak_ptr) — flagged, not fixed,
    // here.
    void dispatch_message_create(const models::Message& message);
    void dispatch_guild_create(const models::Guild& guild);
    void dispatch_latency(int ms);
    void dispatch_heartbeat_ack();

private:
    std::mutex mutex_;
    std::vector<std::function<void()>> ready_callbacks_;
    std::vector<std::function<void(const models::Message&)>> message_callbacks_;
    std::vector<std::function<void(const models::Guild&)>> guild_create_callbacks_;
    std::vector<std::function<void(int)>> latency_callbacks_;
    std::vector<std::function<void()>> heartbeat_ack_callbacks_;
};

} // namespace fluxerpp