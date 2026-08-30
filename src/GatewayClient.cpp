#include "fluxerpp/GatewayClient.h"
#include <libwebsockets.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <chrono>

namespace fluxerpp {

static int heartbeat_interval = 0;
static std::chrono::steady_clock::time_point last_heartbeat;

static int callback_gateway(struct lws *wsi,
                            enum lws_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    switch (reason) {

    case LWS_CALLBACK_CLIENT_RECEIVE: {
        auto data = nlohmann::json::parse(std::string((char*)in, len));

        if (data["op"] == 10) { // HELLO
            heartbeat_interval = data["d"]["heartbeat_interval"];
            last_heartbeat = std::chrono::steady_clock::now();

            GatewayClient* client =
                (GatewayClient*)lws_context_user(lws_get_context(wsi));

            nlohmann::json identify = {
                {"op", 2},
                {"d", {
                    {"token", client->token},
                    {"intents", 513},
                    {"properties", {
                        {"os", "windows"},
                        {"browser", "fluxerpp"},
                        {"device", "fluxerpp"}
                    }}
                }}
            };

            std::string payload = identify.dump();
            std::vector<unsigned char> buf(LWS_PRE + payload.size());
            std::memcpy(buf.data() + LWS_PRE, payload.c_str(), payload.size());

            lws_write(wsi, buf.data() + LWS_PRE, payload.size(), LWS_WRITE_TEXT);

            lws_callback_on_writable(wsi);
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
        if (heartbeat_interval > 0) {
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_heartbeat).count();

            if (ms >= heartbeat_interval) {
                last_heartbeat = now;

                nlohmann::json heartbeat = {
                    {"op", 1},
                    {"d", nullptr}
                };

                std::string payload = heartbeat.dump();
                std::vector<unsigned char> buf(LWS_PRE + payload.size());
                std::memcpy(buf.data() + LWS_PRE, payload.c_str(), payload.size());

                lws_write(wsi, buf.data() + LWS_PRE, payload.size(), LWS_WRITE_TEXT);
            }

            lws_callback_on_writable(wsi);
        }
        break;
    }

    default:
        break;
    }

    return 0;
}

GatewayClient::GatewayClient(const std::string& t)
    : token(t) {}

void GatewayClient::connect() {
    lws_context_creation_info info{};
    info.port = CONTEXT_PORT_NO_LISTEN;

    static lws_protocols protocols[] = {
        {"fluxer", callback_gateway, 0, 4096},
        {nullptr, nullptr, 0, 0}
    };

    info.protocols = protocols;
    info.user = this;

    lws_context* context = lws_create_context(&info);
    if (!context) {
        std::cerr << "Failed to create WebSocket context\n";
        return;
    }

    lws_client_connect_info ccinfo{};
    ccinfo.context = context;
    ccinfo.address = "gateway.fluxer.app";
    ccinfo.port = 443;
    ccinfo.path = "/?v=1&encoding=json";
    ccinfo.ssl_connection = LCCSCF_USE_SSL;
    ccinfo.protocol = "fluxer";

    if (!lws_client_connect_via_info(&ccinfo)) {
        std::cerr << "Failed to connect to Fluxer Gateway\n";
        lws_context_destroy(context);
        return;
    }

    while (lws_service(context, 50) >= 0) {
        lws_callback_on_writable_all_protocol(context, protocols);
    }

    lws_context_destroy(context);
}

} // namespace fluxerpp
