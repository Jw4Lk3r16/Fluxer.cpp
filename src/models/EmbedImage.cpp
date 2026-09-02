// fluxerpp/models/EmbedImage.cpp

#include "fluxerpp/models/EmbedImage.h"

namespace fluxerpp {
namespace models {

using util::Json;

EmbedImage EmbedImage::from_data(const Json& data) {
    EmbedImage img;

    if (data.contains("url") && !data["url"].is_null())
        img.url = data["url"].get<std::string>();

    if (data.contains("proxy_url") && !data["proxy_url"].is_null())
        img.proxy_url = data["proxy_url"].get<std::string>();

    if (data.contains("height") && !data["height"].is_null())
        img.height = data["height"].get<int>();

    if (data.contains("width") && !data["width"].is_null())
        img.width = data["width"].get<int>();

    return img;
}

Json EmbedImage::to_dict() const {
    Json j = Json::object();

    if (url) j["url"] = *url;
    if (proxy_url) j["proxy_url"] = *proxy_url;
    if (height) j["height"] = *height;
    if (width) j["width"] = *width;

    return j;
}

} // namespace models
} // namespace fluxerpp
