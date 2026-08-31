#include "fluxerpp/EventDispatcher.h"

namespace fluxerpp {

// REGISTER CALLBACKS
void EventDispatcher::on_ready(const std::function<void()>& cb) {
    ready_callbacks.push_back(cb);
}

void EventDispatcher::on_message_create(const std::function<void(const nlohmann::json&)>& cb) {
    message_callbacks.push_back(cb);
}

// DISPATCH EVENTS
void EventDispatcher::dispatch_ready() {
    for (auto& cb : ready_callbacks) {
        cb();
    }
}

void EventDispatcher::dispatch_message_create(const nlohmann::json& data) {
    for (auto& cb : message_callbacks) {
        cb(data);
    }
}

}
