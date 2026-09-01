// src/RestClient.cpp
#include "fluxerpp/RestClient.h"
#include "fluxerpp/util/Logger.h"
#include <curl/curl.h>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstdlib>

namespace fluxerpp {

using util::Logger;

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    if (!out) return 0;
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

static size_t headerCallback(char* buffer, size_t size, size_t nitems, std::string* out) {
    if (!out) return 0;
    out->append(buffer, size * nitems);
    return size * nitems;
}

RestClient::RestClient(const FluxerConfig& cfg)
    : config(cfg) { }

double RestClient::parse_retry_after(const std::string& headers) {
    // Look for a "Retry-After: <seconds>" header (case-insensitive-ish; curl
    // typically returns them as sent, so this covers the common case).
    auto pos = headers.find("Retry-After:");
    if (pos == std::string::npos) pos = headers.find("retry-after:");
    if (pos == std::string::npos) return 1.0; // sane default if header absent

    pos += std::string("Retry-After:").size();
    auto end = headers.find_first_of("\r\n", pos);
    std::string val = headers.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    try {
        return std::stod(val);
    } catch (...) {
        return 1.0;
    }
}

RestClient::RawResponse RestClient::perform(const std::string& method,
                                            const std::string& path,
                                            const nlohmann::json* body) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");

    std::string base = config.restBase;
    if (base.empty()) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Rest base URL is empty (config.restBase)");
    }
    if (base.back() == '/') base.pop_back();
    std::string p = path;
    if (!p.empty() && p.front() != '/') p = "/" + p;
    std::string fullUrl = base + p;

    std::string response;
    std::string responseHeaders;
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "FluxerPP-REST/1.0");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bot " + config.token;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // NOTE: intentionally not logging fullUrl+method with the auth header attached.
    Logger::instance().debug("Request: " + fullUrl + " method=" + method);

    std::string payload;
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (body) {
            payload = body->dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
    } else if (method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (body) {
            payload = body->dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
        }
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method != "GET") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        if (body) {
            payload = body->dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
        }
    }

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }

    Logger::instance().debug("Response status=" + std::to_string(http_code));
    return RawResponse{http_code, response, responseHeaders};
}

nlohmann::json RestClient::request(const std::string& method,
                                   const std::string& path,
                                   const nlohmann::json* body) {
    RawResponse resp = perform(method, path, body);

    // A route-level 429 is routine under load, not exceptional — retry once
    // after the server-specified Retry-After instead of throwing straight
    // to the caller. If it 429s again, surface it; a caller-visible retry
    // loop across every call site isn't a fix, it's a symptom.
    if (resp.http_code == 429) {
        double waitSeconds = parse_retry_after(resp.headers);
        Logger::instance().warn("Rate limited on " + path + ", retrying in " +
                                 std::to_string(waitSeconds) + "s");
        std::this_thread::sleep_for(std::chrono::duration<double>(waitSeconds));
        resp = perform(method, path, body);
    }

    if (resp.http_code < 200 || resp.http_code >= 300) {
        throw std::runtime_error("HTTP error " + std::to_string(resp.http_code) + ": " + resp.body);
    }

    try {
        if (resp.body.empty()) return nlohmann::json::object();
        return nlohmann::json::parse(resp.body);
    } catch (const std::exception& ex) {
        throw std::runtime_error(std::string("JSON parse error: ") + ex.what() + " raw=" + resp.body);
    }
}

nlohmann::json RestClient::get(const std::string& path) {
    return request("GET", path, nullptr);
}

nlohmann::json RestClient::post(const std::string& path, const nlohmann::json& body) {
    return request("POST", path, &body);
}

nlohmann::json RestClient::patch(const std::string& path, const nlohmann::json& body) {
    return request("PATCH", path, &body);
}

nlohmann::json RestClient::del(const std::string& path) {
    return request("DELETE", path, nullptr);
}

std::string RestClient::url_encode_component(const std::string& s) {
    CURL* curl = curl_easy_init();
    if (!curl) return s; // best-effort fallback; shouldn't happen in practice
    char* escaped = curl_easy_escape(curl, s.c_str(), static_cast<int>(s.size()));
    std::string out = escaped ? escaped : s;
    if (escaped) curl_free(escaped);
    curl_easy_cleanup(curl);
    return out;
}

nlohmann::json RestClient::send_message(std::uint64_t channel_id, const nlohmann::json& payload) {
    return post("/channels/" + std::to_string(channel_id) + "/messages", payload);
}

nlohmann::json RestClient::edit_message(std::uint64_t channel_id, std::uint64_t message_id,
                                        const nlohmann::json& payload) {
    return patch("/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(message_id), payload);
}

void RestClient::delete_message(std::uint64_t channel_id, std::uint64_t message_id) {
    del("/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(message_id));
}

void RestClient::add_reaction(std::uint64_t channel_id, std::uint64_t message_id, const std::string& emoji) {
    request("PUT",
            "/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(message_id) +
                "/reactions/" + url_encode_component(emoji) + "/@me",
            nullptr);
}

void RestClient::delete_reaction(std::uint64_t channel_id, std::uint64_t message_id,
                                 const std::string& emoji, const std::string& user) {
    request("DELETE",
            "/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(message_id) +
                "/reactions/" + url_encode_component(emoji) + "/" + user,
            nullptr);
}

void RestClient::delete_all_reactions(std::uint64_t channel_id, std::uint64_t message_id) {
    del("/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(message_id) + "/reactions");
}

void RestClient::delete_all_reactions_for_emoji(std::uint64_t channel_id, std::uint64_t message_id,
                                                const std::string& emoji) {
    del("/channels/" + std::to_string(channel_id) + "/messages/" + std::to_string(message_id) +
        "/reactions/" + url_encode_component(emoji));
}

void RestClient::pin_message(std::uint64_t channel_id, std::uint64_t message_id) {
    request("PUT", "/channels/" + std::to_string(channel_id) + "/pins/" + std::to_string(message_id), nullptr);
}

void RestClient::unpin_message(std::uint64_t channel_id, std::uint64_t message_id) {
    del("/channels/" + std::to_string(channel_id) + "/pins/" + std::to_string(message_id));
}

std::uint64_t RestClient::user_id() {
    std::lock_guard<std::mutex> lk(user_id_mutex_);
    if (!cached_user_id_.has_value()) {
        nlohmann::json me = get("/users/@me");
        cached_user_id_ = std::stoull(me.at("id").get<std::string>());
    }
    return *cached_user_id_;
}

} // namespace fluxerpp