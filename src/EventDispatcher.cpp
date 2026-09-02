#include "fluxerpp/EventDispatcher.h"
#include "fluxerpp/util/Logger.h"

namespace fluxerpp {

using util::Logger;

void EventDispatcher::on_ready(const std::function<void()>& cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    ready_callbacks_.push_back(cb);
}

void EventDispatcher::on_message_create(const std::function<void(const nlohmann::json&)>& cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    message_callbacks_.push_back(cb);
}

void EventDispatcher::on_guild_create(const std::function<void(const models::Guild&)>& cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    guild_create_callbacks_.push_back(cb);
}

void EventDispatcher::on_latency(const std::function<void(int)>& cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    latency_callbacks_.push_back(cb);
}

void EventDispatcher::on_heartbeat_ack(const std::function<void()>& cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    heartbeat_ack_callbacks_.push_back(cb);
}

void EventDispatcher::dispatch_ready() {
    std::vector<std::function<void()>> callbacks;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        callbacks = ready_callbacks_; // copy so callbacks can register more callbacks safely
    }
    for (auto& cb : callbacks) {
        try {
            cb();
        } catch (const std::exception& ex) {
            Logger::instance().error(std::string("on_ready callback threw: ") + ex.what());
        } catch (...) {
            Logger::instance().error("on_ready callback threw an unknown exception");
        }
    }
}

void EventDispatcher::dispatch_message_create(const nlohmann::json& data) {
    std::vector<std::function<void(const nlohmann::json&)>> callbacks;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        callbacks = message_callbacks_;
    }
    for (auto& cb : callbacks) {
        try {
            cb(data);
        } catch (const std::exception& ex) {
            Logger::instance().error(std::string("on_message_create callback threw: ") + ex.what());
        } catch (...) {
            Logger::instance().error("on_message_create callback threw an unknown exception");
        }
    }
}

void EventDispatcher::dispatch_guild_create(const models::Guild& guild) {
    std::vector<std::function<void(const models::Guild&)>> callbacks;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        callbacks = guild_create_callbacks_;
    }
    for (auto& cb : callbacks) {
        try {
            cb(guild);
        } catch (const std::exception& ex) {
            Logger::instance().error(std::string("on_guild_create callback threw: ") + ex.what());
        } catch (...) {
            Logger::instance().error("on_guild_create callback threw an unknown exception");
        }
    }
}

void EventDispatcher::dispatch_latency(int ms) {
    std::vector<std::function<void(int)>> callbacks;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        callbacks = latency_callbacks_;
    }
    for (auto& cb : callbacks) {
        try {
            cb(ms);
        } catch (const std::exception& ex) {
            Logger::instance().error(std::string("on_latency callback threw: ") + ex.what());
        } catch (...) {
            Logger::instance().error("on_latency callback threw an unknown exception");
        }
    }
}

void EventDispatcher::dispatch_heartbeat_ack() {
    std::vector<std::function<void()>> callbacks;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        callbacks = heartbeat_ack_callbacks_;
    }
    for (auto& cb : callbacks) {
        try {
            cb();
        } catch (const std::exception& ex) {
            Logger::instance().error(std::string("on_heartbeat_ack callback threw: ") + ex.what());
        } catch (...) {
            Logger::instance().error("on_heartbeat_ack callback threw an unknown exception");
        }
    }
}

} // namespace fluxerpp