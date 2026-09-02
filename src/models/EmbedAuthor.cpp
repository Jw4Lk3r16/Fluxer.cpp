#include "fluxerpp/models/EmbedAuthor.h"

namespace fluxerpp {
namespace models {

using util::Json;

EmbedAuthor EmbedAuthor::from_data(const Json& data) {
    EmbedAuthor a;

    if (data.contains("name") && !data["name"].is_null())
        a.name = data["name"].get<std::string>();

    if (data.contains("url") && !data["url"].is_null())
        a.url = data["url"].get<std::string>();

    if (data.contains("icon_url") && !data["icon_url"].is_null())
        a.icon_url = data["icon_url"].get<std::string>();

    if (data.contains("proxy_icon_url") && !data["proxy_icon_url"].is_null())
        a.proxy_icon_url = data["proxy_icon_url"].get<std::string>();

    return a;
}

Json EmbedAuthor::to_dict() const {
    Json j = Json::object();

    if (name) j["name"] = *name;
    if (url) j["url"] = *url;
    if (icon_url) j["icon_url"] = *icon_url;
    if (proxy_icon_url) j["proxy_icon_url"] = *proxy_icon_url;

    return j;
}

} // namespace models
} // namespace fluxerpp
