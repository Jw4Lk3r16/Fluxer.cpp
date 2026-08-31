// src/GatewayClient.cpp
#include "fluxerpp/GatewayClient.h"
#include <windows.h>
#include <winhttp.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>

#pragma comment(lib, "winhttp.lib")

namespace fluxerpp {

// Helper: send a UTF-8 text message over WinHTTP WebSocket
static DWORD send_websocket_message(HINTERNET hWebSocket, const std::string& payload) {
    // WinHttpWebSocketSend expects a void* pointer (non-const), so cast away constness safely here.
    return WinHttpWebSocketSend(
        hWebSocket,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        (void*)payload.data(),
        static_cast<DWORD>(payload.size())
    );
}

// Build IDENTIFY payload (sent after HELLO)
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

// Build RESUME payload
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

// Utility: extract complete JSON objects from a buffer string.
// This scans for balanced braces while respecting string quoting and escapes.
// Returns vector of JSON strings and leaves any trailing partial data in 'acc'.
static std::vector<std::string> extract_json_objects(std::string& acc) {
    std::vector<std::string> out;
    size_t i = 0;
    const size_t n = acc.size();
    while (i < n) {
        // find first '{'
        size_t start = acc.find('{', i);
        if (start == std::string::npos) break;

        int depth = 0;
        bool in_string = false;
        bool escape = false;
        size_t j = start;
        for (; j < n; ++j) {
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
                        // complete JSON object from start..j
                        out.emplace_back(acc.substr(start, j - start + 1));
                        i = j + 1;
                        break;
                    }
                }
            }
        }
        if (j >= n) break; // incomplete
        // continue scanning from i
    }

    // Remove consumed prefix from acc
    if (!out.empty()) {
        // find the first '{' position
        size_t first_brace = acc.find('{');
        if (first_brace != std::string::npos) {
            // compute consumed length by summing sizes of extracted objects
            size_t consumed_len = 0;
            for (const auto& s : out) consumed_len += s.size();
            acc.erase(0, first_brace + consumed_len);
        } else {
            acc.clear();
        }
    }
    return out;
}

GatewayClient::GatewayClient(const std::string& t)
    : token(t) { }

void GatewayClient::on_ready(const std::function<void()>& cb) {
    ready_callbacks.push_back(cb);
}

void GatewayClient::on_message_create(const std::function<void(const nlohmann::json&)>& cb) {
    message_callbacks.push_back(cb);
}

void GatewayClient::dispatch_ready() {
    for (auto& cb : ready_callbacks) cb();
}

void GatewayClient::dispatch_message_create(const nlohmann::json& data) {
    for (auto& cb : message_callbacks) cb(data);
}

void GatewayClient::connect() {
    const int maxReconnectAttempts = 6;
    int reconnectAttempt = 0;

    std::string session_id;
    int last_seq = -1;

    while (true) {
        HINTERNET hSession = WinHttpOpen(
            L"FluxerPP/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (!hSession) {
            std::cout << "[Gateway] WinHttpOpen failed\n";
            return;
        }

        HINTERNET hConnect = WinHttpConnect(
            hSession,
            L"gateway.fluxer.app",
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        );

        if (!hConnect) {
            std::cout << "[Gateway] WinHttpConnect failed\n";
            WinHttpCloseHandle(hSession);
            return;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            L"/?v=1&encoding=json",
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (!hRequest) {
            std::cout << "[Gateway] WinHttpOpenRequest failed\n";
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        BOOL opt = WinHttpSetOption(
            hRequest,
            WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
            NULL,
            0
        );

        if (!opt) {
            std::cout << "[Gateway] Failed to set WebSocket upgrade option\n";
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(hRequest, NULL)) {
            std::cout << "[Gateway] WebSocket handshake failed\n";
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        HINTERNET hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, (DWORD_PTR)NULL);
        WinHttpCloseHandle(hRequest);

        if (!hWebSocket) {
            std::cout << "[Gateway] WebSocket upgrade failed\n";
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        std::cout << "[Gateway] Connected via WinHTTP WebSocket\n";

        // Receive state
        std::string acc; // accumulator for text frames
        const size_t BUF_SZ = 16 * 1024;
        std::vector<char> buffer(BUF_SZ);
        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType = WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;

        // Heartbeat state
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

        // If we have a session_id, we'll attempt RESUME after HELLO (not before)
        bool want_resume = !session_id.empty();

        // Receive loop
        while (true) {
            DWORD hr = WinHttpWebSocketReceive(
                hWebSocket,
                reinterpret_cast<BYTE*>(buffer.data()),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                &bufferType
            );

            if (hr != NO_ERROR) {
                std::cout << "[Gateway] Receive failed, code=" << hr << "\n";

                // Query close status if available
                closeStatus = 0;
                closeReasonLength = sizeof(closeReason);
                DWORD queryHr = WinHttpWebSocketQueryCloseStatus(
                    hWebSocket,
                    &closeStatus,
                    closeReason,
                    closeReasonLength,
                    &closeReasonLength
                );

                std::cout << "[Gateway] Close status=" << closeStatus << "\n";
                if (queryHr == NO_ERROR) {
                    std::wcout << L"[Gateway] Close reason=" << closeReason << L"\n";
                } else {
                    std::cout << "[Gateway] Close reason unavailable (queryHr=" << queryHr << ")\n";
                }

                connectionClosed = true;
                break;
            }

            // Handle control frames
            if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                closeStatus = 0;
                closeReasonLength = sizeof(closeReason);
                DWORD queryHr = WinHttpWebSocketQueryCloseStatus(
                    hWebSocket,
                    &closeStatus,
                    closeReason,
                    closeReasonLength,
                    &closeReasonLength
                );
                std::cout << "[Gateway] Received CLOSE frame, status=" << closeStatus << "\n";
                if (queryHr == NO_ERROR) {
                    std::wcout << L"[Gateway] Close reason=" << closeReason << L"\n";
                }
                connectionClosed = true;
                break;
            }

            if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
                // Append received bytes to accumulator
                acc.append(buffer.data(), bytesRead);

                // Extract complete JSON objects (handles fragmentation and multiple objects)
                auto jsonStrings = extract_json_objects(acc);
                for (const auto& js : jsonStrings) {
                    nlohmann::json data;
                    try {
                        data = nlohmann::json::parse(js);
                    } catch (const std::exception& ex) {
                        std::cout << "[Gateway] JSON parse error: " << ex.what() << "\n";
                        continue;
                    }

                    // Update sequence if present
                    if (data.contains("s") && !data["s"].is_null()) {
                        std::lock_guard<std::mutex> lk(seq_mutex);
                        try {
                            last_seq = data.value("s", last_seq);
                        } catch (...) {}
                    }

                    int op = data.value("op", -1);

                    if (op == 10) { // HELLO
                        int interval = 0;
                        try {
                            interval = data["d"].value("heartbeat_interval", 0);
                        } catch (...) {}
                        heartbeat_interval_ms.store(interval);
                        std::cout << "[Gateway] HELLO interval=" << interval << "\n";

                        // Start heartbeat thread now (but do not send heartbeats until IDENTIFY/RESUME is sent)
                        if (!heartbeat_running.load()) {
                            heartbeat_running.store(true);
                            stop_heartbeat.store(false);
                            heartbeatThread = std::thread([&]() {
                                using namespace std::chrono;
                                int local_interval = heartbeat_interval_ms.load();
                                if (local_interval <= 0) local_interval = 41250;
                                // initial sleep half interval to stagger
                                std::this_thread::sleep_for(milliseconds(local_interval / 2));
                                while (!stop_heartbeat.load()) {
                                    // Build heartbeat payload with last_seq (may be -1)
                                    int seq_snapshot = -1;
                                    {
                                        std::lock_guard<std::mutex> lk(seq_mutex);
                                        seq_snapshot = last_seq;
                                    }
                                    nlohmann::json hb;
                                    hb["op"] = 1;
                                    if (seq_snapshot == -1) hb["d"] = nullptr;
                                    else hb["d"] = seq_snapshot;

                                    std::string payload = hb.dump();
                                    DWORD sendHr = send_websocket_message(hWebSocket, payload);
                                    if (sendHr != NO_ERROR) {
                                        std::cout << "[Gateway] Heartbeat send failed, code=" << sendHr << "\n";
                                        break;
                                    }
                                    std::cout << "[Gateway] Sent heartbeat\n";
                                    int sleep_ms = heartbeat_interval_ms.load();
                                    if (sleep_ms <= 0) sleep_ms = local_interval;
                                    std::this_thread::sleep_for(milliseconds(sleep_ms));
                                }
                            });
                        }

                        // After HELLO: attempt RESUME if we have session info, otherwise IDENTIFY
                        if (want_resume && !identified_or_resumed) {
                            std::string resumePayload = build_resume(this->token, session_id, last_seq);
                            DWORD sendHr = send_websocket_message(hWebSocket, resumePayload);
                            if (sendHr == NO_ERROR) {
                                std::cout << "[Gateway] Sent RESUME (session_id=" << session_id << ", seq=" << last_seq << ")\n";
                                identified_or_resumed = true;
                            } else {
                                std::cout << "[Gateway] RESUME send failed, code=" << sendHr << " — will IDENTIFY\n";
                                // fall through to IDENTIFY
                            }
                        }

                        if (!identified_or_resumed) {
                            std::string identifyPayload = build_identify(this->token);
                            DWORD sendHr = send_websocket_message(hWebSocket, identifyPayload);
                            if (sendHr != NO_ERROR) {
                                std::cout << "[Gateway] IDENTIFY send failed, code=" << sendHr << "\n";
                                connectionClosed = true;
                                break;
                            }
                            std::cout << "[Gateway] Sent IDENTIFY\n";
                            identified_or_resumed = true;
                        }

                    } else if (op == 0) { // DISPATCH
                        std::string t = data.value("t", "");
                        std::cout << "[Gateway] DISPATCH: " << t << "\n";

                        if (t == "READY") {
                            try {
                                session_id = data["d"].value("session_id", session_id);
                            } catch (...) {}
                            std::cout << "[Gateway] READY received; session_id=" << session_id << "\n";
                            dispatch_ready();
                        } else if (t == "MESSAGE_CREATE") {
                            std::cout << "[Gateway] MESSAGE_CREATE received\n";
                            try {
                                dispatch_message_create(data["d"]);
                            } catch (...) {}
                        } else {
                            // other dispatch events can be handled here
                        }
                    } else if (op == 1) { // Server heartbeat request
                        // Respond with heartbeat (include last_seq)
                        int seq_snapshot = -1;
                        {
                            std::lock_guard<std::mutex> lk(seq_mutex);
                            seq_snapshot = last_seq;
                        }
                        nlohmann::json hb;
                        hb["op"] = 1;
                        if (seq_snapshot == -1) hb["d"] = nullptr;
                        else hb["d"] = seq_snapshot;

                        std::string payload = hb.dump();
                        DWORD sendHr = send_websocket_message(hWebSocket, payload);
                        if (sendHr != NO_ERROR) {
                            std::cout << "[Gateway] Heartbeat (response) send failed, code=" << sendHr << "\n";
                            connectionClosed = true;
                            break;
                        }
                        std::cout << "[Gateway] Responded to server heartbeat request\n";
                    } else if (op == 7) { // RECONNECT
                        std::cout << "[Gateway] Server requested reconnect (OP 7)\n";
                        connectionClosed = true;
                        break;
                    } else if (op == 9) { // INVALID SESSION
                        bool resumable = false;
                        try {
                            resumable = data["d"].get<bool>();
                        } catch (...) {}
                        std::cout << "[Gateway] INVALID SESSION (resumable=" << (resumable ? "true" : "false") << ")\n";
                        if (!resumable) {
                            // clear session info so next connect will IDENTIFY
                            session_id.clear();
                            last_seq = -1;
                        }
                        // Close and reconnect
                        connectionClosed = true;
                        break;
                    } else if (op == 11) { // HEARTBEAT ACK
                        std::cout << "[Gateway] Heartbeat ACK\n";
                        // Could update heartbeat monitoring state here
                    } else {
                        // Unhandled op
                    }
                } // end for each json string
            } else if (bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
                // ignore binary frames but do not append to text accumulator
                continue;
            } else {
                // other control frames: ignore
                continue;
            }
        } // end receive loop

        // Stop heartbeat thread if running
        if (heartbeat_running.load()) {
            stop_heartbeat.store(true);
            if (heartbeatThread.joinable()) heartbeatThread.join();
            heartbeat_running.store(false);
        }

        // Close handles
        WinHttpCloseHandle(hWebSocket);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (!connectionClosed) {
            std::cout << "[Gateway] Connection ended without close status; stopping.\n";
            break;
        }

        // Decide reconnect behavior
        bool resumable = false;
        if (closeStatus == 1000) {
            std::cout << "[Gateway] Normal close (1000). Not reconnecting.\n";
            break;
        } else {
            if (closeStatus == 4009) resumable = true;
        }

        reconnectAttempt++;
        if (reconnectAttempt > maxReconnectAttempts) {
            std::cout << "[Gateway] Max reconnect attempts reached (" << maxReconnectAttempts << "). Giving up.\n";
            break;
        }

        int backoffSeconds = (1 << (reconnectAttempt - 1));
        if (backoffSeconds > 30) backoffSeconds = 30;
        std::cout << "[Gateway] Reconnecting in " << backoffSeconds << "s (attempt " << reconnectAttempt << ")\n";
        std::this_thread::sleep_for(std::chrono::seconds(backoffSeconds));

        if (!resumable) {
            session_id.clear();
            last_seq = -1;
        } else {
            std::cout << "[Gateway] Attempting resume on reconnect (session_id=" << session_id << ", seq=" << last_seq << ")\n";
        }
    } // end outer reconnect loop
}

} // namespace fluxerpp
