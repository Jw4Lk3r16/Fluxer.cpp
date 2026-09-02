// fluxerpp/models/EmbedThumbnail.cpp

#include "fluxerpp/models/EmbedThumbnail.h"

namespace fluxerpp {
namespace models {

using util::Json;

EmbedThumbnail EmbedThumbnail::from_data(const Json& data) {
    EmbedThumbnail t;

    if (data.contains("url") && !data["url"].is_null())
        t.url = data["url"].get<std::string>();

    if (data.contains("proxy_url") && !data["proxy_url"].is_null())
        t.proxy_url = data["proxy_url"].get<std::string>();

    if (data.contains("height") && !data["height"].is_null())
        t.height = data["height"].get<int>();

    if (data.contains("width") && !data["width"].is_null())
        t.width = data["width"].get<int>();

    return t;
}

Json EmbedThumbnail::to_dict() const {
    Json j = Json::object();

    if (url) j["url"] = *url;
    if (proxy_url) j["proxy_url"] = *proxy_url;
    if (height) j["height"] = *height;
    if (width) j["width"] = *width;

    return j;
}

} // namespace models
} // namespace fluxerpp
