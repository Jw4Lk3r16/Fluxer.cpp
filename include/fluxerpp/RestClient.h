#pragma once

#include "fluxerpp/FluxerConfig.h"
#include <nlohmann/json.hpp>
#include <string>

namespace fluxerpp {

class RestClient {
public:
    explicit RestClient(const FluxerConfig& cfg);
    ~RestClient() = default;

    // Generic request helper (throws on CURL error or JSON parse error)
    nlohmann::json request(const std::string& method,
                           const std::string& path,
                           const nlohmann::json* body = nullptr);

    nlohmann::json get(const std::string& path);
    nlohmann::json post(const std::string& path, const nlohmann::json& body);
    nlohmann::json patch(const std::string& path, const nlohmann::json& body);
    nlohmann::json del(const std::string& path);

private:
    FluxerConfig config;
};

} // namespace fluxerpp
