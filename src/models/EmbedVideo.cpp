// fluxerpp/models/EmbedVideo.cpp

#include "fluxerpp/models/EmbedVideo.h"

namespace fluxerpp {
namespace models {

using util::Json;

EmbedVideo EmbedVideo::from_data(const Json& data) {
    EmbedVideo v;

    if (data.contains("url") && !data["url"].is_null())
        v.url = data["url"].get<std::string>();

    if (data.contains("proxy_url") && !data["proxy_url"].is_null())
        v.proxy_url = data["proxy_url"].get<std::string>();

    if (data.contains("height") && !data["height"].is_null())
        v.height = data["height"].get<int>();

    if (data.contains("width") && !data["width"].is_null())
        v.width = data["width"].get<int>();

    return v;
}

Json EmbedVideo::to_dict() const {
    Json j = Json::object();

    if (url)       j["url"] = *url;
    if (proxy_url) j["proxy_url"] = *proxy_url;
    if (height)    j["height"] = *height;
    if (width)     j["width"] = *width;

    return j;
}

} // namespace models
} // namespace fluxerpp
