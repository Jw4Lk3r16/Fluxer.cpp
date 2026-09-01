// Attachment.cpp
#include "fluxerpp/models/Attachment.h"

namespace fluxerpp {
namespace models {

using util::Json;
using util::parse_snowflake;

Attachment Attachment::from_data(const Json& data) {
    Attachment a;
    a.id = parse_snowflake(data, "id");
    a.filename = data.value("filename", "");
    a.size = data.value("size", 0ull);
    a.url = data.value("url", "");
    a.proxy_url = data.value("proxy_url", "");

    if (data.contains("content_type") && !data["content_type"].is_null()) {
        a.content_type = data["content_type"].get<std::string>();
    }
    if (data.contains("width") && !data["width"].is_null()) {
        a.width = data["width"].get<int>();
    }
    if (data.contains("height") && !data["height"].is_null()) {
        a.height = data["height"].get<int>();
    }
    return a;
}

} // namespace models
} // namespace fluxerpp