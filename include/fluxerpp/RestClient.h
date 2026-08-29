#pragma once
#include <string>
#include "fluxerpp/nlohmann/json.hpp"
#include "FluxerConfig.h"

namespace fluxerpp {

class RestClient {
public:
    explicit RestClient(const FluxerConfig& cfg);

    nlohmann::json get(const std::string& path);
    nlohmann::json post(const std::string& path, const nlohmann::json& body);
    nlohmann::json patch(const std::string& path, const nlohmann::json& body);
    nlohmann::json del(const std::string& path);

private:
    FluxerConfig config;
    nlohmann::json request(const std::string& method,
                           const std::string& path,
                           const nlohmann::json* body);
};

}
