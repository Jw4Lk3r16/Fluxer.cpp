// Embed.cpp
#include "fluxerpp/models/Embed.h"

namespace fluxerpp {
namespace models {

using util::Json;

Embed Embed::from_data(const Json& data) {
    Embed e;
    if (data.contains("title") && !data["title"].is_null()) e.title = data["title"].get<std::string>();
    if (data.contains("description") && !data["description"].is_null()) e.description = data["description"].get<std::string>();
    if (data.contains("url") && !data["url"].is_null()) e.url = data["url"].get<std::string>();
    if (data.contains("color") && !data["color"].is_null()) e.color = data["color"].get<int>();
    if (data.contains("timestamp") && !data["timestamp"].is_null()) e.timestamp = data["timestamp"].get<std::string>();
    return e;
}

Json Embed::to_dict() const {
    Json j = Json::object();
    if (title) j["title"] = *title;
    if (description) j["description"] = *description;
    if (url) j["url"] = *url;
    if (color) j["color"] = *color;
    if (timestamp) j["timestamp"] = *timestamp;
    return j;
}

} // namespace models
} // namespace fluxerpp