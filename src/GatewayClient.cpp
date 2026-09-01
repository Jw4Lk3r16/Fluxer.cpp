// src/GatewayClient.cpp
#include "fluxerpp/GatewayClient.h"
#include "fluxerpp/RestClient.h"
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

// Helper: send a UTF-8 text message over WinHTTP WebSocket
static DWORD send_websocket_message(HINTERNET hWebSocket, const std::string& payload) {
    return WinHttpWebSocketSend(
        hWebSocket,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        (void*)payload.data(),
        static_cast<DWORD>(payload.size())
    );
}

// Build IDENTIFY payload (sent after HELLO). Never logged in full — see
// debug_handle_raw_frame below, which only logs t/op, not payload bodies.
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
// final chunk as WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE. A previous
// version of this loop only appended bytes to `acc` when it saw the
// _MESSAGE_ type and used a brace-matching scanner to pull JSON objects out
// of whatever survived — which silently dropped every FRAGMENT chunk. For
// small payloads (HELLO) that never showed up; for a large READY payload
// (guild roles/regions/member profile all nested under "d") most of the
// message was fragmented and got thrown away, leaving only stray bytes that
// happened to still parse as small, unrelated-looking JSON objects.
//
// aiohttp (and browsers) do this reassembly for you transparently, which is
// why the Python/JS reference clients just do `json.loads(msg.data)` on one
// complete message with no manual scanning. We now do the same: accumulate
// every chunk regardless of FRAGMENT vs MESSAGE type, and only parse+clear
// `acc` once WinHTTP reports the MESSAGE type, meaning the message is done.

GatewayClient::GatewayClient(const std::string& t)
    : token(t) { }

void GatewayClient::on_ready(const std::function<void()>& cb) {
    dispatcher.on_ready(cb);
}

void GatewayClient::on_message_create(const std::function<void(const nlohmann::json&)>& cb) {
    dispatcher.on_message_create(cb);
}

void GatewayClient::connect() {
    int reconnectAttempt = 0;

    std::string session_id;
    int last_seq = -1;

    // Resolve the real gateway URL via GET /gateway/bot before dialing
    // anything, matching WebSocketManager.connect() in the JS client. This
    // used to hardcode "gateway.fluxer.app" with path "/", which may not be
    // the actual event gateway — hence receiving bare, unenveloped objects
    // instead of a proper op/t/s/d dispatch stream.
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
        HINTERNET hSession = WinHttpOpen(
            L"FluxerPP/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (!hSession) {
            Logger::instance().error("WinHttpOpen failed");
            return;
        }

        HINTERNET hConnect = WinHttpConnect(
            hSession,
            wHost.c_str(),
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        );

        if (!hConnect) {
            Logger::instance().error("WinHttpConnect failed");
            WinHttpCloseHandle(hSession);
            return;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            wPath.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (!hRequest) {
            Logger::instance().error("WinHttpOpenRequest failed");
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        BOOL opt = WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);
        if (!opt) {
            Logger::instance().error("Failed to set WebSocket upgrade option");
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(hRequest, NULL)) {
            Logger::instance().error("WebSocket handshake failed");
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        HINTERNET hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, (DWORD_PTR)NULL);
        WinHttpCloseHandle(hRequest);

        if (!hWebSocket) {
            Logger::instance().error("WebSocket upgrade failed");
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        Logger::instance().info("Connected via WinHTTP WebSocket");

        std::string acc;
        const size_t BUF_SZ = 16 * 1024;
        std::vector<char> buffer(BUF_SZ);
        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType = WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;

        std::atomic<int> heartbeat_interval_ms{0};
        std::atomic<bool> heartbeat_running{false};
        std::atomic<bool> stop_heartbeat{false};
        std::mutex seq_mutex;
        std::thread heartbeatThread;

        bool identified_or_resumed = false;
        bool connectionClosed = false;
        USHORT closeStatus = 0;
        WCHAR closeReason[512] = {};
        DWORD closeReasonLength = 0;

        bool want_resume = !session_id.empty();

        while (true) {
            DWORD hr = WinHttpWebSocketReceive(
                hWebSocket,
                reinterpret_cast<BYTE*>(buffer.data()),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                &bufferType
            );

            if (hr != NO_ERROR) {
                Logger::instance().warn("Receive failed, code=" + std::to_string(hr));

                closeStatus = 0;
                closeReasonLength = sizeof(closeReason);
                DWORD queryHr = WinHttpWebSocketQueryCloseStatus(
                    hWebSocket, &closeStatus, closeReason, closeReasonLength, &closeReasonLength
                );
                Logger::instance().warn("Close status=" + std::to_string(closeStatus));
                if (queryHr != NO_ERROR) {
                    Logger::instance().warn("Close reason unavailable (queryHr=" + std::to_string(queryHr) + ")");
                }

                connectionClosed = true;
                break;
            }

            if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                closeStatus = 0;
                closeReasonLength = sizeof(closeReason);
                WinHttpWebSocketQueryCloseStatus(
                    hWebSocket, &closeStatus, closeReason, closeReasonLength, &closeReasonLength
                );
                Logger::instance().info("Received CLOSE frame, status=" + std::to_string(closeStatus));
                connectionClosed = true;
                break;
            }

            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE ||
                bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) {
                acc.append(buffer.data(), bytesRead);

                if (bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) {
                    // Only part of this message has arrived — keep accumulating
                    // and wait for the chunk WinHTTP marks as _MESSAGE_ type.
                    continue;
                }

                // _MESSAGE_ type: this was the final chunk, so `acc` now holds
                // exactly one complete JSON envelope. Take ownership and reset
                // the accumulator for the next message.
                std::string js = std::move(acc);
                acc.clear();

                if (debug_logging_) {
                    Logger::instance().debug("RAW FRAME: " + js);
                }

                nlohmann::json data;
                try {
                    data = nlohmann::json::parse(js);
                } catch (const std::exception& ex) {
                    Logger::instance().warn(std::string("JSON parse error: ") + ex.what());
                    continue;
                }

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
                                int seq_snapshot = -1;
                                {
                                    std::lock_guard<std::mutex> lk(seq_mutex);
                                    seq_snapshot = last_seq;
                                }
                                nlohmann::json hb;
                                hb["op"] = 1;
                                hb["d"] = (seq_snapshot == -1) ? nlohmann::json(nullptr) : nlohmann::json(seq_snapshot);

                                DWORD sendHr = send_websocket_message(hWebSocket, hb.dump());
                                if (sendHr != NO_ERROR) {
                                    Logger::instance().warn("Heartbeat send failed, code=" + std::to_string(sendHr));
                                    break;
                                }
                                int sleep_ms = heartbeat_interval_ms.load();
                                if (sleep_ms <= 0) sleep_ms = local_interval;
                                std::this_thread::sleep_for(milliseconds(sleep_ms));
                            }
                        });
                    }

                    if (want_resume && !identified_or_resumed) {
                        std::string resumePayload = build_resume(this->token, session_id, last_seq);
                        DWORD sendHr = send_websocket_message(hWebSocket, resumePayload);
                        if (sendHr == NO_ERROR) {
                            Logger::instance().info("Sent RESUME (session_id=" + session_id + ", seq=" + std::to_string(last_seq) + ")");
                            identified_or_resumed = true;
                        } else {
                            Logger::instance().warn("RESUME send failed, code=" + std::to_string(sendHr) + " — will IDENTIFY");
                        }
                    }

                    if (!identified_or_resumed) {
                        std::string identifyPayload = build_identify(this->token);
                        DWORD sendHr = send_websocket_message(hWebSocket, identifyPayload);
                        if (sendHr != NO_ERROR) {
                            Logger::instance().error("IDENTIFY send failed, code=" + std::to_string(sendHr));
                            connectionClosed = true;
                            break;
                        }
                        Logger::instance().info("Sent IDENTIFY (token=" + Logger::redact(this->token) + ")");
                        identified_or_resumed = true;
                    }

                } else if (op == 0) { // DISPATCH
                    std::string t = data.value("t", "");

                    if (t == "READY") {
                        try { session_id = data["d"].value("session_id", session_id); } catch (...) {}
                        Logger::instance().info("READY received; session_id=" + session_id);
                        dispatcher.dispatch_ready();
                    } else if (t == "MESSAGE_CREATE") {
                        try {
                            dispatcher.dispatch_message_create(data["d"]);
                        } catch (const std::exception& ex) {
                            Logger::instance().error(std::string("MESSAGE_CREATE handling failed: ") + ex.what());
                        }
                    }
                    // other dispatch events: add routing here as needed

                } else if (op == 1) { // Server heartbeat request
                    int seq_snapshot = -1;
                    {
                        std::lock_guard<std::mutex> lk(seq_mutex);
                        seq_snapshot = last_seq;
                    }
                    nlohmann::json hb;
                    hb["op"] = 1;
                    hb["d"] = (seq_snapshot == -1) ? nlohmann::json(nullptr) : nlohmann::json(seq_snapshot);

                    DWORD sendHr = send_websocket_message(hWebSocket, hb.dump());
                    if (sendHr != NO_ERROR) {
                        Logger::instance().warn("Heartbeat (response) send failed, code=" + std::to_string(sendHr));
                        connectionClosed = true;
                        break;
                    }

                } else if (op == 7) { // RECONNECT
                    Logger::instance().info("Server requested reconnect (OP 7)");
                    connectionClosed = true;
                    break;

                } else if (op == 9) { // INVALID SESSION
                    bool resumable = false;
                    try { resumable = data["d"].get<bool>(); } catch (...) {}
                    Logger::instance().warn(std::string("INVALID SESSION (resumable=") + (resumable ? "true" : "false") + ")");
                    if (!resumable) {
                        session_id.clear();
                        last_seq = -1;
                    }
                    connectionClosed = true;
                    break;

                } else if (op == 11) { // HEARTBEAT ACK
                    if (debug_logging_) Logger::instance().debug("Heartbeat ACK");
                }

            } else if (bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE ||
                       bufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE) {
                continue; // binary frames aren't used by this JSON gateway
            } else {
                continue;
            }
        } // end receive loop

        if (heartbeat_running.load()) {
            stop_heartbeat.store(true);
            if (heartbeatThread.joinable()) heartbeatThread.join();
            heartbeat_running.store(false);
        }

        WinHttpCloseHandle(hWebSocket);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (!connectionClosed) {
            Logger::instance().info("Connection ended without close status; stopping.");
            break;
        }

        bool resumable = false;
        if (closeStatus == 1000) {
            Logger::instance().info("Normal close (1000). Not reconnecting.");
            break;
        } else if (closeStatus == 4009) {
            resumable = true;
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

        if (!resumable) {
            session_id.clear();
            last_seq = -1;
        } else {
            Logger::instance().info("Attempting resume on reconnect (session_id=" + session_id + ", seq=" + std::to_string(last_seq) + ")");
        }
    } // end outer reconnect loop
}

} // namespace fluxerpp