#pragma once
#include <functional>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace fluxerpp {

class EventDispatcher {
public:
    // READY
    void on_ready(const std::function<void()>& cb);

    // MESSAGE_CREATE
    void on_message_create(const std::function<void(const nlohmann::json&)>& cb);

    // Dispatchers called by GatewayClient
    void dispatch_ready();
    void dispatch_message_create(const nlohmann::json& data);

private:
    std::vector<std::function<void()>> ready_callbacks;
    std::vector<std::function<void(const nlohmann::json&)>> message_callbacks;
};

}
