// src/GatewayClient.cpp
#include "fluxerpp/GatewayClient.h"
#include "fluxerpp/RestClient.h"
#include "fluxerpp/models/Guild.h"
#include "fluxerpp/models/Message.h"
#include "fluxerpp/util/Logger.h"
#include <windows.h>
#include <winhttp.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>

#pragma comment(lib, "winhttp.lib")

namespace fluxerpp {

using util::Logger;

static std::wstring to_wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    if (!w.empty() && w.back() == L'\0') w.pop_back(); // drop the extra null MultiByteToWideChar wrote
    return w;
}

// Splits a "wss://host[:port]/path?query" URL into host and path+query.
// Path defaults to "/" if the URL has none.
struct ParsedWsUrl {
    std::string host;
    std::string path;
};

static ParsedWsUrl parse_ws_url(const std::string& url) {
    ParsedWsUrl out;
    std::string rest = url;
    auto schemePos = rest.find("://");
    if (schemePos != std::string::npos) rest = rest.substr(schemePos + 3);

    auto slashPos = rest.find('/');
    if (slashPos == std::string::npos) {
        out.host = rest;
        out.path = "/";
    } else {
        out.host = rest.substr(0, slashPos);
        out.path = rest.substr(slashPos);
        if (out.path.empty()) out.path = "/";
    }
    return out;
}

// --- RAII for WinHTTP handles -----------------------------------------
//
// Previously every failure branch in connect() manually called
// WinHttpCloseHandle() in the right order for whichever handles had been
// opened so far, repeated across ~6 branches. Any C++ exception thrown
// between acquiring a handle and reaching that branch's cleanup (e.g. from
// nlohmann::json parsing a malformed payload) would skip the cleanup
// entirely and leak the handle. Single-owner, move-only wrapper — closes
// automatically on any scope exit, including exception unwinding.
class WinHttpHandle {
public:
    WinHttpHandle() = default;
    explicit WinHttpHandle(HINTERNET h) : h_(h) {}
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    WinHttpHandle(WinHttpHandle&& other) noexcept : h_(other.h_) { other.h_ = nullptr; }
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept {
        if (this != &other) { close(); h_ = other.h_; other.h_ = nullptr; }
        return *this;
    }
    ~WinHttpHandle() { close(); }

    HINTERNET get() const { return h_; }
    explicit operator bool() const { return h_ != nullptr; }
    void close() { if (h_) { WinHttpCloseHandle(h_); h_ = nullptr; } }

private:
    HINTERNET h_{nullptr};
};

// --- RAII for the heartbeat thread -------------------------------------
//
// Previously heartbeat cleanup (stop flag + join) only ran at the bottom of
// each connection attempt's happy path. If any exception escaped between
// starting the thread and reaching that code — e.g. an unexpected JSON
// field type deep in dispatch handling — stack unwinding would destroy a
// still-joinable std::thread and call std::terminate() per its destructor
// contract. This guard's destructor runs on every exit path (normal break
// *or* exception unwinding) because C++ destroys stack locals in reverse
// declaration order regardless of how the scope is left.
class HeartbeatGuard {
public:
    HeartbeatGuard(std::thread& t, std::atomic<bool>& stopFlag, std::atomic<bool>& runningFlag)
        : thread_(t), stop_(stopFlag), running_(runningFlag) {}
    ~HeartbeatGuard() {
        stop_.store(true);
        if (thread_.joinable()) thread_.join();
        running_.store(false);
    }
    HeartbeatGuard(const HeartbeatGuard&) = delete;
    HeartbeatGuard& operator=(const HeartbeatGuard&) = delete;

private:
    std::thread& thread_;
    std::atomic<bool>& stop_;
    std::atomic<bool>& running_;
};

// Helper: send a UTF-8 text message over WinHTTP WebSocket
static DWORD send_websocket_message(HINTERNET hWebSocket, const std::string& payload) {
    return WinHttpWebSocketSend(
        hWebSocket,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        (void*)payload.data(),
        static_cast<DWORD>(payload.size())
    );
}

// Build IDENTIFY payload (sent after HELLO). Never logged in full — see the
// IDENTIFY-sent log line below, which only logs a redacted token.
static std::string build_identify(const std::string& token) {
    nlohmann::json identify = {
        {"op", 2},
        {"d", {
            {"token", token},
            {"intents", 0},
            {"properties", {
                {"os", "windows"},
                {"browser", "fluxerpp"},
                {"device", "fluxerpp"}
            }}
        }}
    };
    return identify.dump();
}

static std::string build_resume(const std::string& token, const std::string& session_id, int seq) {
    nlohmann::json resume = {
        {"op", 6},
        {"d", {
            {"token", token},
            {"session_id", session_id},
            {"seq", seq}
        }}
    };
    return resume.dump();
}

// NOTE on message reassembly:
// WinHTTP's WebSocket API can split one logical message across several
// WinHttpWebSocketReceive calls. It marks every chunk of a message except
// the last as WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE, and only the
// final chunk as WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE. We accumulate
// every chunk regardless of type and only parse+clear `acc` once WinHTTP
// reports the MESSAGE type, meaning the message is fully reassembled —
// matching what aiohttp/browsers do transparently for you.

GatewayClient::GatewayClient(const std::string& t)
    : token(t) { }

void GatewayClient::on_ready(const std::function<void()>& cb) {
    dispatcher.on_ready(cb);
}

void GatewayClient::on_message_create(const std::function<void(const models::Message&)>& cb) {
    dispatcher.on_message_create(cb);
}

void GatewayClient::on_guild_create(const std::function<void(const models::Guild&)>& cb) {
    dispatcher.on_guild_create(cb);
}

void GatewayClient::on_latency(const std::function<void(int)>& cb) {
    dispatcher.on_latency(cb);
}

void GatewayClient::on_heartbeat_ack(const std::function<void()>& cb) {
    dispatcher.on_heartbeat_ack(cb);
}

void GatewayClient::stop() {
    stop_requested_.store(true);
    // Force-cancel a blocking receive so connect() notices promptly rather
    // than waiting for the next server message. Same idempotent pattern
    // the heartbeat thread uses on a missed ACK — safe if both race to
    // close the same handle.
    void* h = active_ws_handle_.exchange(nullptr);
    if (h) WinHttpCloseHandle(static_cast<HINTERNET>(h));
}

void GatewayClient::connect() {
    int reconnectAttempt = 0;

    std::string session_id;
    int last_seq = -1;
    // Tracks whether the *server* told us the session is resumable (via
    // op 9 INVALID_SESSION's `d` field, or a 4009 close code). The
    // reconnect decision below uses this directly instead of a second,
    // separately-initialized local that used to shadow and discard it.
    bool session_resumable = true;

    // Resolve the real gateway URL via GET /gateway/bot before dialing
    // anything, matching WebSocketManager.connect() in the JS client.
    ParsedWsUrl resolved{fallback_host, "/"};
    if (rest_) {
        try {
            nlohmann::json gw = rest_->get("/gateway/bot");
            std::string url = gw.at("url").get<std::string>();
            resolved = parse_ws_url(url);
            Logger::instance().info("Resolved gateway via /gateway/bot: " + url);
        } catch (const std::exception& ex) {
            Logger::instance().error(std::string("GET /gateway/bot failed: ") + ex.what() +
                                      " — falling back to " + fallback_host);
        }
    } else {
        Logger::instance().warn("No RestClient bound (call bind_rest()) — using fallback_host " +
                                 fallback_host + " instead of resolving /gateway/bot");
    }

    std::wstring wHost = to_wide(resolved.host);
    std::string queryChar = (resolved.path.find('?') == std::string::npos) ? "?" : "&";
    std::wstring wPath = to_wide(resolved.path + queryChar + "v=" + gateway_version + "&encoding=json");

    while (true) {
        if (stop_requested_.load()) {
            Logger::instance().info("stop() was called — not (re)connecting.");
            break;
        }

        WinHttpHandle hSession(WinHttpOpen(
            L"FluxerPP/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        ));

        if (!hSession) {
            Logger::instance().error("WinHttpOpen failed");
            return;
        }

        WinHttpHandle hConnect(WinHttpConnect(
            hSession.get(),
            wHost.c_str(),
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        ));

        if (!hConnect) {
            Logger::instance().error("WinHttpConnect failed");
            return; // hSession closes automatically
        }

        WinHttpHandle hRequest(WinHttpOpenRequest(
            hConnect.get(),
            L"GET",
            wPath.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        ));

        if (!hRequest) {
            Logger::instance().error("WinHttpOpenRequest failed");
            return; // hConnect, hSession close automatically
        }

        BOOL opt = WinHttpSetOption(hRequest.get(), WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);
        if (!opt) {
            Logger::instance().error("Failed to set WebSocket upgrade option");
            return;
        }

        if (!WinHttpSendRequest(hRequest.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(hRequest.get(), NULL)) {
            Logger::instance().error("WebSocket handshake failed");
            return;
        }

        HINTERNET rawWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest.get(), (DWORD_PTR)NULL);
        hRequest.close(); // WinHTTP's documented pattern: close the request handle right after upgrade, win or lose

        if (!rawWebSocket) {
            Logger::instance().error("WebSocket upgrade failed");
            return;
        }

        // active_ws_handle_ mirrors this connection attempt's live socket so
        // stop() (any thread) and the heartbeat thread's missed-ACK path can
        // both force-cancel a blocking receive via the same atomic exchange
        // — whichever gets there first wins, the other sees nullptr and
        // no-ops, so it's safe even if both race.
        active_ws_handle_.store(static_cast<void*>(rawWebSocket));
        struct ActiveHandleGuard {
            std::atomic<void*>& slot;
            HINTERNET h;
            ~ActiveHandleGuard() {
                // Only clear/close if nobody else (stop()/missed-ACK) already did.
                void* expected = h;
                if (slot.compare_exchange_strong(expected, nullptr)) {
                    WinHttpCloseHandle(h);
                }
            }
        } activeHandleGuard{active_ws_handle_, rawWebSocket};

        Logger::instance().info("Connected via WinHTTP WebSocket");

        std::string acc;
        const size_t BUF_SZ = 16 * 1024;
        std::vector<char> buffer(BUF_SZ);
        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType = WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;

        std::atomic<int> heartbeat_interval_ms{0};
        std::atomic<bool> heartbeat_running{false};
        std::atomic<bool> stop_heartbeat{false};
        // Set true right after we send any heartbeat (scheduled or
        // server-requested), cleared on op 11 (HEARTBEAT ACK). If it's
        // still true when the heartbeat thread wakes up to send the next
        // one, the previous ACK never arrived — previously this was never
        // checked at all, so a half-dead connection with no ACKs could sit
        // blocked in WinHttpWebSocketReceive indefinitely.
        std::atomic<bool> awaiting_ack{false};
        std::mutex seq_mutex;
        std::thread heartbeatThread;
        // Declared *after* the thread/atomics it references so it destructs
        // *before* them (reverse declaration order) on every exit path —
        // see the class comment above.
        HeartbeatGuard heartbeatGuard(heartbeatThread, stop_heartbeat, heartbeat_running);

        bool identified_or_resumed = false;
        bool connectionClosed = false;
        USHORT closeStatus = 0;
        WCHAR closeReason[512] = {};
        DWORD closeReasonLength = 0;

        bool want_resume = !session_id.empty();

        while (true) {
            DWORD hr = WinHttpWebSocketReceive(
                rawWebSocket,
                reinterpret_cast<BYTE*>(buffer.data()),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                &bufferType
            );

            if (hr != NO_ERROR) {
                if (stop_requested_.load()) {
                    Logger::instance().info("Receive canceled by stop().");
                } else {
                    Logger::instance().warn("Receive failed, code=" + std::to_string(hr));
                }

                closeStatus = 0;
                closeReasonLength = sizeof(closeReason);
                DWORD queryHr = WinHttpWebSocketQueryCloseStatus(
                    rawWebSocket, &closeStatus, closeReason, closeReasonLength, &closeReasonLength
                );
                if (queryHr == NO_ERROR) {
                    Logger::instance().warn("Close status=" + std::to_string(closeStatus));
                }

                connectionClosed = true;
                break;
            }

            if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                closeStatus = 0;
                closeReasonLength = sizeof(closeReason);
                WinHttpWebSocketQueryCloseStatus(
                    rawWebSocket, &closeStatus, closeReason, closeReasonLength, &closeReasonLength
                );
                Logger::instance().info("Received CLOSE frame, status=" + std::to_string(closeStatus));
                connectionClosed = true;
                break;
            }

            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE ||
                bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) {
                acc.append(buffer.data(), bytesRead);

                if (bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) {
                    continue; // more of this message is still coming
                }

                std::string js = std::move(acc);
                acc.clear();

                if (debug_logging_) {
                    Logger::instance().debug("RAW FRAME: " + js);
                }

                // Everything from here through dispatch used to only have
                // json::parse guarded by try/catch. data.value("op", -1)
                // and friends can themselves throw nlohmann::json::type_error
                // if a field exists with an unexpected type — e.g. "op" sent
                // as a string. That exception used to propagate out of this
                // whole block uncaught, unwinding past a still-joinable
                // heartbeatThread and calling std::terminate(). Wrapping the
                // full parse-through-dispatch sequence closes that gap: a
                // malformed message is now logged and skipped, not fatal.
                try {
                    nlohmann::json data = nlohmann::json::parse(js);

                    if (data.contains("s") && !data["s"].is_null()) {
                        std::lock_guard<std::mutex> lk(seq_mutex);
                        last_seq = data.value("s", last_seq);
                    }

                    int op = data.value("op", -1);

                    if (debug_logging_) {
                        std::string t = data.value("t", std::string());
                        Logger::instance().debug("DISPATCH t=" + (t.empty() ? "<none>" : t) +
                                                  " op=" + std::to_string(op));
                    }

                    if (op == 10) { // HELLO
                        int interval = 0;
                        try { interval = data["d"].value("heartbeat_interval", 0); } catch (...) {}
                        heartbeat_interval_ms.store(interval);
                        Logger::instance().info("HELLO interval=" + std::to_string(interval));

                        if (!heartbeat_running.load()) {
                            heartbeat_running.store(true);
                            stop_heartbeat.store(false);
                            heartbeatThread = std::thread([&]() {
                                using namespace std::chrono;
                                int local_interval = heartbeat_interval_ms.load();
                                if (local_interval <= 0) local_interval = 41250;
                                std::this_thread::sleep_for(milliseconds(local_interval / 2));
                                while (!stop_heartbeat.load()) {
                                    if (awaiting_ack.load()) {
                                        Logger::instance().warn(
                                            "Missed heartbeat ACK — forcing reconnect");
                                        void* h = active_ws_handle_.exchange(nullptr);
                                        if (h) WinHttpCloseHandle(static_cast<HINTERNET>(h));
                                        break;
                                    }

                                    int seq_snapshot;
                                    {
                                        std::lock_guard<std::mutex> lk(seq_mutex);
                                        seq_snapshot = last_seq;
                                    }
                                    nlohmann::json hb;
                                    hb["op"] = 1;
                                    hb["d"] = (seq_snapshot == -1) ? nlohmann::json(nullptr) : nlohmann::json(seq_snapshot);

                                    void* sockRaw = active_ws_handle_.load();
                                    if (!sockRaw) break; // already closed elsewhere
                                    DWORD sendHr = send_websocket_message(static_cast<HINTERNET>(sockRaw), hb.dump());
                                    if (sendHr != NO_ERROR) {
                                        Logger::instance().warn("Heartbeat send failed, code=" + std::to_string(sendHr));
                                        break;
                                    }
                                    // Timestamped right after the confirmed send, not before —
                                    // hb.dump()'s serialization time shouldn't count toward
                                    // the latency measured when the ACK comes back.
                                    last_hb_sent_.store(std::chrono::steady_clock::now());
                                    awaiting_ack.store(true);

                                    int sleep_ms = heartbeat_interval_ms.load();
                                    if (sleep_ms <= 0) sleep_ms = local_interval;
                                    std::this_thread::sleep_for(milliseconds(sleep_ms));
                                }
                            });
                        }

                        if (want_resume && !identified_or_resumed) {
                            std::string resumePayload = build_resume(this->token, session_id, last_seq);
                            DWORD sendHr = send_websocket_message(rawWebSocket, resumePayload);
                            if (sendHr == NO_ERROR) {
                                Logger::instance().info("Sent RESUME (session_id=" + session_id + ", seq=" + std::to_string(last_seq) + ")");
                                identified_or_resumed = true;
                            } else {
                                Logger::instance().warn("RESUME send failed, code=" + std::to_string(sendHr) + " — will IDENTIFY");
                            }
                        }

                        if (!identified_or_resumed) {
                            std::string identifyPayload = build_identify(this->token);
                            DWORD sendHr = send_websocket_message(rawWebSocket, identifyPayload);
                            if (sendHr != NO_ERROR) {
                                Logger::instance().error("IDENTIFY send failed, code=" + std::to_string(sendHr));
                                connectionClosed = true;
                            } else {
                                Logger::instance().info("Sent IDENTIFY (token=" + Logger::redact(this->token) + ")");
                                identified_or_resumed = true;
                            }
                        }

                    } else if (op == 0) { // DISPATCH
                        std::string t = data.value("t", "");

                        if (t == "READY") {
                            try { session_id = data["d"].value("session_id", session_id); } catch (...) {}
                            // A successful READY means this connection attempt
                            // worked end to end — reset the backoff counter so
                            // an unrelated disconnect much later in the
                            // process's life doesn't inherit a nearly-exhausted
                            // budget from a rocky start hours ago.
                            reconnectAttempt = 0;
                            session_resumable = true;
                            Logger::instance().info("READY received; session_id=" + session_id);
                            dispatcher.dispatch_ready();
                        } else if (t == "MESSAGE_CREATE") {
                            try {
                                models::Message msg = models::Message::from_data(data["d"], rest_);
                                dispatcher.dispatch_message_create(msg);
                            } catch (const std::exception& ex) {
                                Logger::instance().error(std::string("MESSAGE_CREATE handling failed: ") + ex.what());
                            }
                        } else if (t == "GUILD_CREATE") {
                            try {
                                models::Guild guild = models::Guild::from_data(data["d"], rest_);
                                dispatcher.dispatch_guild_create(guild);
                            } catch (const std::exception& ex) {
                                Logger::instance().error(std::string("GUILD_CREATE handling failed: ") + ex.what());
                            }
                        }
                        // other dispatch events: add routing here as needed

                    } else if (op == 1) { // Server heartbeat request
                        int seq_snapshot;
                        {
                            std::lock_guard<std::mutex> lk(seq_mutex);
                            seq_snapshot = last_seq;
                        }
                        nlohmann::json hb;
                        hb["op"] = 1;
                        hb["d"] = (seq_snapshot == -1) ? nlohmann::json(nullptr) : nlohmann::json(seq_snapshot);

                        DWORD sendHr = send_websocket_message(rawWebSocket, hb.dump());
                        if (sendHr != NO_ERROR) {
                            Logger::instance().warn("Heartbeat (response) send failed, code=" + std::to_string(sendHr));
                            connectionClosed = true;
                        } else {
                            // This is the fix for the bug where a
                            // server-requested heartbeat's ACK would measure
                            // latency against whenever the *previous*
                            // scheduled heartbeat went out, not this one —
                            // last_hb_sent_ needs updating here too, same as
                            // the scheduled loop above.
                            last_hb_sent_.store(std::chrono::steady_clock::now());
                            awaiting_ack.store(true);
                        }

                    } else if (op == 7) { // RECONNECT
                        Logger::instance().info("Server requested reconnect (OP 7)");
                        // Deliberately does NOT touch session_resumable —
                        // RECONNECT always implies "come back and resume",
                        // unlike INVALID SESSION which explicitly tells us
                        // whether resuming is possible.
                        connectionClosed = true;

                    } else if (op == 9) { // INVALID SESSION
                        bool resumable = false;
                        try { resumable = data["d"].get<bool>(); } catch (...) {}
                        Logger::instance().warn(std::string("INVALID SESSION (resumable=") + (resumable ? "true" : "false") + ")");
                        session_resumable = resumable;
                        if (!resumable) {
                            session_id.clear();
                            last_seq = -1;
                        }
                        connectionClosed = true;

                    } else if (op == 11) { // HEARTBEAT ACK
                        awaiting_ack.store(false);
                        auto now = std::chrono::steady_clock::now();
                        auto sent = last_hb_sent_.load();
                        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - sent).count();
                        // Routed through EventDispatcher (mutex-guarded,
                        // per-callback try/catch) rather than invoking raw
                        // std::function members directly — see the header
                        // comment on why the previous version was a data race.
                        dispatcher.dispatch_latency(static_cast<int>(ms));
                        dispatcher.dispatch_heartbeat_ack();
                        if (debug_logging_) Logger::instance().debug("Heartbeat ACK");
                    }

                } catch (const std::exception& ex) {
                    Logger::instance().warn(std::string("Failed to handle message: ") + ex.what());
                    continue;
                } catch (...) {
                    Logger::instance().warn("Failed to handle message: unknown exception");
                    continue;
                }

                if (connectionClosed) break;

            } else if (bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE ||
                       bufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) {
                continue; // binary frames aren't used by this JSON gateway
            } else {
                continue;
            }
        } // end receive loop

        // heartbeatGuard, activeHandleGuard, hRequest/hConnect/hSession all
        // clean up automatically here via RAII as this scope ends (reverse
        // declaration order), including on the stop_requested_ exit path.

        if (stop_requested_.load()) {
            break;
        }

        if (!connectionClosed) {
            Logger::instance().info("Connection ended without close status; stopping.");
            break;
        }

        if (closeStatus == 1000) {
            Logger::instance().info("Normal close (1000). Not reconnecting.");
            break;
        }
        if (closeStatus == 4009) {
            session_resumable = true; // server-side signal, in case op 9 didn't already tell us
        }

        reconnectAttempt++;
        if (reconnectAttempt > max_reconnect_attempts) {
            Logger::instance().error("Max reconnect attempts reached (" + std::to_string(max_reconnect_attempts) + "). Giving up.");
            break;
        }

        int backoffSeconds = (1 << (reconnectAttempt - 1));
        if (backoffSeconds > 30) backoffSeconds = 30;
        Logger::instance().info("Reconnecting in " + std::to_string(backoffSeconds) + "s (attempt " + std::to_string(reconnectAttempt) + ")");
        std::this_thread::sleep_for(std::chrono::seconds(backoffSeconds));

        if (!session_resumable) {
            session_id.clear();
            last_seq = -1;
        } else if (!session_id.empty()) {
            Logger::instance().info("Attempting resume on reconnect (session_id=" + session_id + ", seq=" + std::to_string(last_seq) + ")");
        }
    } // end outer reconnect loop
}

} // namespace fluxerpp