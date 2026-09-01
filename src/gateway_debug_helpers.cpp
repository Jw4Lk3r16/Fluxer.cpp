// src/gateway_debug_helpers.cpp
// Helpers to integrate into your existing GatewayClient receive/dispatch code.
// Do NOT compile this as a second GatewayClient definition.
// Instead: include these functions in your existing GatewayClient.cpp
// and call them from the receive loop and dispatch_ready implementation.

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>

// ----------------------------- JSON extraction -----------------------------
// Safely extract complete JSON objects from an accumulator string.
// Returns vector of JSON text fragments and erases consumed prefix from acc.
static std::vector<std::string> extract_json_objects(std::string& acc) {
    std::vector<std::string> out;
    size_t search_from = 0;
    size_t last_consumed_end = 0;

    while (true) {
        // find next opening brace
        size_t start = acc.find('{', search_from);
        if (start == std::string::npos) break;

        int depth = 0;
        bool in_string = false;
        bool escape = false;
        size_t j = start;
        bool complete = false;

        for (; j < acc.size(); ++j) {
            char c = acc[j];
            if (in_string) {
                if (escape) {
                    escape = false;
                } else if (c == '\\') {
                    escape = true;
                } else if (c == '"') {
                    in_string = false;
                }
            } else {
                if (c == '"') {
                    in_string = true;
                } else if (c == '{') {
                    ++depth;
                } else if (c == '}') {
                    --depth;
                    if (depth == 0) {
                        complete = true;
                        break;
                    }
                }
            }
        }

        if (!complete) break; // incomplete JSON tail; wait for more data

        // j points at the closing '}' of a complete object
        out.emplace_back(acc.substr(start, j - start + 1));
        last_consumed_end = j + 1;
        search_from = last_consumed_end;
    }

    if (last_consumed_end > 0) {
        acc.erase(0, last_consumed_end);
    }
    return out;
}

// ----------------------------- Logging helpers -----------------------------
static inline void log_raw_frame(const std::string& raw) {
    std::cout << "[Gateway] RAW FRAME len=" << raw.size() << " : " << raw << "\n";
}

static inline void trace_dispatch(const nlohmann::json& data, size_t raw_len) {
    try {
        std::string t = data.value("t", std::string());
        int op = data.value("op", -1);
        std::cout << "[Gateway] DISPATCH t=" << (t.empty() ? "<none>" : t)
                  << " op=" << op << " raw_len=" << raw_len << "\n";
    } catch (...) {
        std::cout << "[Gateway] DISPATCH (failed to read t/op)\n";
    }
}

// ----------------------------- Protected ready dispatch -----------------------------
// Call this from your existing dispatch_ready() implementation.
// - ready_callbacks: vector of callbacks you already maintain
// - ready_received_flag: atomic<bool> you should store in your GatewayClient
static inline void dispatch_ready_protected(std::vector<std::function<void()>>& ready_callbacks,
                                            std::atomic<bool>& ready_received_flag,
                                            std::mutex& cb_mutex) {
    ready_received_flag.store(true, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lk(cb_mutex);
    std::cout << "[Gateway] dispatch_ready() invoking " << ready_callbacks.size() << " callbacks\n";
    for (auto& cb : ready_callbacks) {
        try {
            cb();
        } catch (const std::exception& ex) {
            std::cerr << "[Gateway] ready callback threw: " << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[Gateway] ready callback threw unknown exception\n";
        }
    }
}

// ----------------------------- READY watchdog -----------------------------
// Start this after you send IDENTIFY. It will reconnect if READY not seen.
class ReadyWatchdog {
public:
    ReadyWatchdog(std::atomic<bool>& ready_flag,
                  std::function<void()> reconnect_fn,
                  std::chrono::seconds timeout = std::chrono::seconds(90))
        : ready_received(ready_flag), reconnect(std::move(reconnect_fn)), timeout_s(timeout) {}

    void start() {
        bool expected = false;
        if (!running.compare_exchange_strong(expected, true)) return;
        ready_received.store(false, std::memory_order_relaxed);

        std::thread([this]() {
            std::cout << "[Gateway] READY watchdog started; timeout=" << timeout_s.count() << "s\n";
            auto start = std::chrono::steady_clock::now();
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (ready_received.load(std::memory_order_relaxed)) {
                    std::cout << "[Gateway] READY received; watchdog stopping\n";
                    running.store(false);
                    return;
                }
                if (std::chrono::steady_clock::now() - start >= timeout_s) {
                    std::cerr << "[Gateway] READY watchdog fired: no READY within " << timeout_s.count() << "s\n";
                    try {
                        reconnect();
                    } catch (const std::exception& ex) {
                        std::cerr << "[Gateway] reconnect() threw: " << ex.what() << "\n";
                    } catch (...) {
                        std::cerr << "[Gateway] reconnect() threw unknown exception\n";
                    }
                    running.store(false);
                    return;
                }
            }
        }).detach();
    }

private:
    std::atomic<bool>& ready_received;
    std::function<void()> reconnect;
    std::chrono::seconds timeout_s;
    std::atomic<bool> running{false};
};

