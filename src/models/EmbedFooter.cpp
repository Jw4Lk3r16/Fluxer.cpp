// fluxerpp/models/EmbedFooter.cpp

#include "fluxerpp/models/EmbedFooter.h"

namespace fluxerpp {
namespace models {

using util::Json;

EmbedFooter EmbedFooter::from_data(const Json& data) {
    EmbedFooter f;

    if (data.contains("text") && !data["text"].is_null())
        f.text = data["text"].get<std::string>();

    if (data.contains("icon_url") && !data["icon_url"].is_null())
        f.icon_url = data["icon_url"].get<std::string>();

    if (data.contains("proxy_icon_url") && !data["proxy_icon_url"].is_null())
        f.proxy_icon_url = data["proxy_icon_url"].get<std::string>();

    return f;
}

Json EmbedFooter::to_dict() const {
    Json j = Json::object();

    if (text) j["text"] = *text;
    if (icon_url) j["icon_url"] = *icon_url;
    if (proxy_icon_url) j["proxy_icon_url"] = *proxy_icon_url;

    return j;
}

} // namespace models
} // namespace fluxerpp
