// fluxerpp/models/Embed.cpp

#include "fluxerpp/models/Embed.h"

namespace fluxerpp {
namespace models {

using util::Json;

Embed Embed::from_data(const Json& data) {
    Embed e;

    if (data.contains("title") && !data["title"].is_null())
        e.title = data["title"].get<std::string>();

    if (data.contains("description") && !data["description"].is_null())
        e.description = data["description"].get<std::string>();

    if (data.contains("url") && !data["url"].is_null())
        e.url = data["url"].get<std::string>();

    if (data.contains("color") && !data["color"].is_null())
        e.color = data["color"].get<int>();

    if (data.contains("timestamp") && !data["timestamp"].is_null())
        e.timestamp = data["timestamp"].get<std::string>();

    // ⭐ Correct field parsing
    if (data.contains("fields")) {
        for (const auto& f : data["fields"]) {
            e.fields.push_back(EmbedField::from_data(f));
        }
    }

    if (data.contains("video") && !data["video"].is_null()) {
        e.video = EmbedVideo::from_data(data["video"]);
    }

    if (data.contains("author") && !data["author"].is_null())
        e.author = EmbedAuthor::from_data(data["author"]);

    if (data.contains("footer") && !data["footer"].is_null())
        e.footer = EmbedFooter::from_data(data["footer"]);

    if (data.contains("thumbnail") && !data["thumbnail"].is_null())
        e.thumbnail = EmbedThumbnail::from_data(data["thumbnail"]);

    if (data.contains("image") && !data["image"].is_null())
        e.image = EmbedImage::from_data(data["image"]);



    return e;
}

Json Embed::to_dict() const {
    Json j = Json::object();

    if (title)      j["title"] = *title;
    if (description) j["description"] = *description;
    if (url)        j["url"] = *url;
    if (color)      j["color"] = *color;
    if (timestamp)  j["timestamp"] = *timestamp;

    // ⭐ Correct field serialization
    if (!fields.empty()) {
        j["fields"] = Json::array();
        for (const auto& f : fields)
            j["fields"].push_back(f.to_dict());
    }

    if (video.has_value()) {
        j["video"] = video->to_dict();
    }

    if (author.has_value())
        j["author"] = author->to_dict();

    if (footer.has_value())
        j["footer"] = footer->to_dict();

    if (thumbnail.has_value())
        j["thumbnail"] = thumbnail->to_dict();

    if (image.has_value())
        j["image"] = image->to_dict();


    return j;
}

} // namespace models
} // namespace fluxerpp
