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

#pragma comment(lib, "winhttp.lib")

namespace fluxerpp {

GatewayClient::GatewayClient(const std::string& t)
    : token(t) {}

static DWORD send_websocket_message(HINTERNET hWebSocket, const std::string& payload) {
    return WinHttpWebSocketSend(
        hWebSocket,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        (BYTE*)payload.c_str(),
        (DWORD)payload.size()
    );
}

// Helper: build IDENTIFY payload
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

// Helper: build RESUME payload
// NOTE: Confirm the RESUME op code and payload shape with Fluxer docs.
// Discord uses op 6 for RESUME; if Fluxer differs, change accordingly.
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

void GatewayClient::connect() {
    // We'll allow reconnect attempts on non-normal closes
    const int maxReconnectAttempts = 6;
    int reconnectAttempt = 0;

    // Persisted session state for resume
    std::string session_id;
    int last_seq = -1;

    // Outer reconnect loop
    while (true) {
        // Create WinHTTP session/connect/request/upgrade
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

        BOOL result = WinHttpSetOption(
            hRequest,
            WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
            NULL,
            0
        );

        if (!result) {
            std::cout << "[Gateway] Failed to set WebSocket upgrade option\n";
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        result = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0
        );

        if (!result || !WinHttpReceiveResponse(hRequest, NULL)) {
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

        // Receive buffer and state
        char buffer[8192];
        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

        // JSON accumulator for fragmented frames
        std::string jsonBuffer;

        // Heartbeat state
        std::atomic<int> heartbeat_interval_ms{0};
        std::atomic<bool> heartbeat_running{false};
        std::atomic<bool> stop_heartbeat{false};
        std::mutex hb_mutex;
        std::thread heartbeatThread;

        // Whether we've sent IDENTIFY/RESUME successfully for this connection
        bool identified_or_resumed = false;

        // If we have a session_id from previous run, attempt RESUME first
        if (!session_id.empty()) {
            std::string resumePayload = build_resume(this->token, session_id, last_seq);
            DWORD sendHr = send_websocket_message(hWebSocket, resumePayload);
            if (sendHr == NO_ERROR) {
                std::cout << "[Gateway] Sent RESUME (session_id=" << session_id << ", seq=" << last_seq << ")\n";
                identified_or_resumed = true; // we'll still wait for server confirmation
            } else {
                std::cout << "[Gateway] RESUME send failed, code=" << sendHr << " — will IDENTIFY instead\n";
            }
        }

        // Main receive loop
        bool connectionClosed = false;
        USHORT closeStatus = 0;
        WCHAR closeReason[512] = {};
        DWORD closeReasonLength = 0;

        while (true) {
            DWORD hr = WinHttpWebSocketReceive(
                hWebSocket,
                (BYTE*)buffer,
                sizeof(buffer),
                &bytesRead,
                &bufferType
            );

            if (hr != NO_ERROR) {
                std::cout << "[Gateway] Receive failed, code=" << hr << "\n";

                // Query close status (if available)
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

            // Control frames
            if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                // Query close status and break
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
                // Append fragment to accumulator
                jsonBuffer.append(buffer, bytesRead);

                // Try to parse the accumulated JSON; if incomplete, wait for more frames
                nlohmann::json data;
                try {
                    data = nlohmann::json::parse(jsonBuffer);
                    // Successfully parsed — clear accumulator
                    jsonBuffer.clear();
                } catch (const std::exception&) {
                    // Not a complete JSON document yet — continue receiving
                    continue;
                }

                // Update sequence if present
                if (data.contains("s") && !data["s"].is_null()) {
                    try {
                        last_seq = data.value("s", last_seq);
                    } catch (...) { /* ignore */ }
                }

                int op = data.value("op", -1);

                if (op == 10) { // HELLO
                    int interval = data["d"].value("heartbeat_interval", 0);
                    heartbeat_interval_ms.store(interval);
                    std::cout << "[Gateway] HELLO interval=" << interval << "\n";

                    // Start heartbeat thread if not already running
                    if (!heartbeat_running.load()) {
                        heartbeat_running.store(true);
                        stop_heartbeat.store(false);

                        // Start heartbeat thread
                        heartbeatThread = std::thread([&]() {
                            using namespace std::chrono;
                            // small initial delay (half interval) to send first heartbeat earlier
                            int interval_local = heartbeat_interval_ms.load();
                            if (interval_local <= 0) interval_local = 41250; // fallback
                            std::this_thread::sleep_for(milliseconds(interval_local / 2));

                            while (!stop_heartbeat.load()) {
                                nlohmann::json hb = { {"op", 1}, {"d", nullptr} };
                                std::string payload = hb.dump();
                                DWORD sendHr = send_websocket_message(hWebSocket, payload);
                                if (sendHr != NO_ERROR) {
                                    std::cout << "[Gateway] Heartbeat send failed, code=" << sendHr << "\n";
                                    break;
                                }
                                std::cout << "[Gateway] Sent heartbeat\n";

                                // Sleep for the interval (re-check interval each loop)
                                int sleep_ms = heartbeat_interval_ms.load();
                                if (sleep_ms <= 0) sleep_ms = interval_local;
                                std::this_thread::sleep_for(milliseconds(sleep_ms));
                            }
                        });
                    }

                    // If we haven't identified/resumed yet, send IDENTIFY now (if resume wasn't sent or failed)
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
                        // Save session_id for resume
                        try {
                            session_id = data["d"].value("session_id", session_id);
                        } catch (...) {}
                        std::cout << "[Gateway] READY received; session_id=" << session_id << "\n";
                    } else if (t == "GUILD_CREATE") {
                        std::cout << "[Gateway] GUILD_CREATE received\n";
                    } else if (t == "MESSAGE_CREATE") {
                        std::cout << "[Gateway] MESSAGE_CREATE received\n";
                    }
                    // Add more event handling here
                } else if (op == 1) { // Server heartbeat request
                    // Respond immediately
                    nlohmann::json hb = { {"op", 1}, {"d", nullptr} };
                    std::string payload = hb.dump();
                    DWORD sendHr = send_websocket_message(hWebSocket, payload);
                    if (sendHr != NO_ERROR) {
                        std::cout << "[Gateway] Heartbeat (response) send failed, code=" << sendHr << "\n";
                        connectionClosed = true;
                        break;
                    }
                    std::cout << "[Gateway] Responded to server heartbeat request\n";
                } else if (op == 11) { // HEARTBEAT ACK (if used)
                    std::cout << "[Gateway] Heartbeat ACK\n";
                } else {
                    // Unhandled op
                }
            } else if (bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE) {
                // Ignore binary frames for now
                continue;
            } else {
                // Other control frames (PING/PONG) — ignore
                continue;
            }
        } // end receive loop

        // Stop heartbeat thread if running
        if (heartbeat_running.load()) {
            stop_heartbeat.store(true);
            if (heartbeatThread.joinable()) heartbeatThread.join();
            heartbeat_running.store(false);
        }

        // Close handles for this connection
        WinHttpCloseHandle(hWebSocket);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        // If connection closed normally (1000) or we explicitly broke with no closeStatus, stop reconnecting
        if (!connectionClosed) {
            std::cout << "[Gateway] Connection ended without close status; stopping.\n";
            break;
        }

        // Decide whether to attempt resume/reconnect
        // Treat 1000 as normal close -> stop
        // Treat 4009 or other resumable codes as candidate for resume
        // If server gave a reason like "Press Enter to exit..." it's suspicious; still attempt reconnect a few times
        bool resumable = false;
        if (closeStatus == 1000) {
            std::cout << "[Gateway] Normal close (1000). Not reconnecting.\n";
            break;
        } else {
            // Heuristic: treat 4009 as resumable; you can adjust based on Fluxer docs
            if (closeStatus == 4009) resumable = true;
            // Also allow reconnect attempts for other non-1000 codes up to a limit
        }

        reconnectAttempt++;
        if (reconnectAttempt > maxReconnectAttempts) {
            std::cout << "[Gateway] Max reconnect attempts reached (" << maxReconnectAttempts << "). Giving up.\n";
            break;
        }

        // Backoff before reconnecting
        int backoffSeconds = (1 << (reconnectAttempt - 1));
        if (backoffSeconds > 30) backoffSeconds = 30;
        std::cout << "[Gateway] Reconnecting in " << backoffSeconds << "s (attempt " << reconnectAttempt << ")\n";
        std::this_thread::sleep_for(std::chrono::seconds(backoffSeconds));

        // If not resumable, clear session state so next connect will IDENTIFY
        if (!resumable) {
            session_id.clear();
            last_seq = -1;
        } else {
            std::cout << "[Gateway] Attempting resume on reconnect (session_id=" << session_id << ", seq=" << last_seq << ")\n";
        }

        // Loop will attempt to reconnect
    } // end outer reconnect loop
}

} // namespace fluxerpp
