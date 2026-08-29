#include "fluxerpp/RestClient.h"
#include <curl/curl.h>
#include <stdexcept>

namespace fluxerpp {

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

RestClient::RestClient(const FluxerConfig& cfg)
    : config(cfg) {}

nlohmann::json RestClient::request(const std::string& method,
                                   const std::string& path,
                                   const nlohmann::json* body) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");

    std::string url = config.restBase + path;
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth = "Authorization: Bot " + config.token;
    headers = curl_slist_append(headers, auth.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (body) {
            auto payload = body->dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        }
    } else if (method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (body) {
            auto payload = body->dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        }
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error("HTTP error: " + std::string(curl_easy_strerror(res)));

    return nlohmann::json::parse(response);
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

} // namespace fluxerpp
